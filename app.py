import os
import subprocess
from flask import Flask, render_template, request, redirect, url_for, session

app = Flask(__name__)
app.secret_key = 'some_secret_key'  # needed for session usage

@app.route('/')
def index():
    return redirect(url_for('editor'))

@app.template_filter("enumerate")
def enumerate_filter(sequence):
    return list(enumerate(sequence))


# ----------------------------------------------------------------------------
# 1. EDITOR
# ----------------------------------------------------------------------------
@app.route('/editor', methods=['GET', 'POST'])
def editor():
    if request.method == 'POST':
        # 1. Get code from form
        code = request.form.get('code', '')

        # 2. Write to input.asm
        with open('input.asm', 'w') as f:
            f.write(code)

        # 3. Run your existing executables
        subprocess.run(['./main'])
        subprocess.run(['./simulator'])

        # 4. Reset step index and logs for fresh simulation
        session['step_index'] = 0

        return redirect(url_for('simulator'))

    # On GET, load existing code if present
    existing_code = ''
    if os.path.exists('input.asm'):
        with open('input.asm', 'r') as f:
            existing_code = f.read()

    return render_template('editor.html', code=existing_code)

# ----------------------------------------------------------------------------
# 2. SIMULATOR
# ----------------------------------------------------------------------------



@app.route('/simulator', methods=['GET', 'POST'])
def simulator():
    # Ensure we have a step counter in session
    if 'step_index' not in session:
        session['step_index'] = 0

    # 1) Load instructions & data segment from output.mc (if it exists)
    instructions, data_seg = parse_output_mc('output.mc')

    # 2) Determine action if POST
    if request.method == 'POST':
        action = request.form.get('action', '')

        if action == 'run':
            # parse final_state.mc & final_data.mc for final registers/memory
            session['step_index'] = len(instructions)  # highlight "beyond last"
            registers = parse_final_state('final_state.mc')
            # final_data.mc => raw memory
            raw_memory_dict = parse_final_data('final_data.mc')  
            
        elif action == 'step':
            session['step_index'] += 1
            step_idx = session['step_index']
            registers = parse_register_log('register_log.mc', step_idx)
            raw_memory_dict = parse_memory_log('memory_log.mc', step_idx)

        elif action == 'reset':
            session['step_index'] = 0
            registers = get_default_registers()
            raw_memory_dict = get_default_memory_dict()  # e.g. {addr_int: byte_int}

        elif action == 'assemble':
            if os.path.exists('input.asm'):
                subprocess.run(['./main'])
                subprocess.run(['./simulator'])
            session['step_index'] = 0
            registers = get_default_registers()
            # Let’s say data_seg is a list of (addr_str, val_str),
            # we can convert that to a dict { addr_int : val_int } if needed
            raw_memory_dict = convert_data_list_to_dict(data_seg)
        else:
            # No recognized action
            registers = get_default_registers()
            raw_memory_dict = get_default_memory_dict()
    else:
        #  GET request => show default registers/memory if nothing else is done
        registers = get_default_registers()
        raw_memory_dict = get_default_memory_dict()

    # 3) Handle the "Jump to" logic (GET parameters: address=..., move=...)
    address_hex = request.args.get('address', None)
    move_cmd    = request.args.get('move', None)
    jump_addr   = None
    
    # Convert 'address' param from hex to int if given
    if address_hex:
        try:
            jump_addr = int(address_hex, 16)
        except ValueError:
            jump_addr = None  # ignore bad input

    # If user clicked up/down, shift jump_addr by some fixed offset
    # e.g., 10 words (40 bytes)
    if jump_addr and move_cmd:
        WORD_SIZE = 4
        SHIFT_COUNT = 10  # how many words to skip
        offset = WORD_SIZE * SHIFT_COUNT
        if move_cmd == 'up':
            jump_addr += offset
        elif move_cmd == 'down':
            jump_addr -= offset

    # 4) Convert raw_memory_dict => chunked list for the template
    #    Something like: [ (addr, [b0,b1,b2,b3]), ... ] in descending order
    memory_chunked = chunk_memory_into_words(raw_memory_dict)

    # 5) If you literally want to show *all* addresses, pass memory_chunked as is.
    #    Otherwise, you can filter around jump_addr if you only want a "window."
    # memory_chunked = slice_memory_around(memory_chunked, jump_addr, radius=10)

    # 6) Get the current step index for highlighting instructions
    current_step = session['step_index']

    return render_template(
        'simulator.html',
        instructions=instructions,
        registers=registers,
        memory=memory_chunked,
        step_index=current_step,
        jump_addr=jump_addr
    )


def chunk_memory_into_words(mem_dict):
    """
    Takes a dict { address(int): byte_val(int), ... }
    Returns list of (addr, [b0,b1,b2,b3]) sorted descending by address (like Venus).
    Each row shows 4 bytes with 'addr' as the highest byte address in that group.
    """
    if not mem_dict:
        return []
    # get all addresses, sort descending
    all_addrs = sorted(mem_dict.keys(), reverse=True)
    result = []
    i = 0
    while i < len(all_addrs):
        addr = all_addrs[i]
        # pick b0..b3 in a big-endian or little-endian manner, your choice
        b0 = mem_dict.get(addr, 0)
        b1 = mem_dict.get(addr - 1, 0)
        b2 = mem_dict.get(addr - 2, 0)
        b3 = mem_dict.get(addr - 3, 0)
        result.append((addr, [b0, b1, b2, b3]))
        i += 4  # skip next 3 addresses because they're in the same row
    return result


def slice_memory_around(memory_list, center_addr, radius=10):
    """
    Given memory_list as [(word_addr, [b0,b1,b2,b3]) ...], return a small
    slice around 'center_addr' (± radius rows). We look for the row that includes center_addr.
    """
    if center_addr is None:
        return memory_list  # no jump => show everything, or do whatever you prefer

    # find the row that covers center_addr
    center_index = None
    for i, (addr, bytes_list) in enumerate(memory_list):
        # row covers addresses [addr, addr-1, addr-2, addr-3]
        if addr >= center_addr >= addr-3:
            center_index = i
            break
    if center_index is None:
        # if not found, just return all
        return memory_list

    start = max(0, center_index - radius)
    end   = min(len(memory_list), center_index + radius + 1)
    return memory_list[start:end]


def convert_data_list_to_dict(data_seg):
    """
    If data_seg is something like [(0x10000000, 0x5), (0x10000001, 0x0), ...],
    convert to { 0x10000000: 0x05, 0x10000001: 0x00, ... }
    """
    mem = {}
    for addr_str, val_str in data_seg:
        try:
            addr_int = int(addr_str, 16)
            val_int  = int(val_str, 16)
        except ValueError:
            # skip or handle error
            continue
        mem[addr_int] = val_int
    return mem


# @app.route('/simulator', methods=['GET', 'POST'])
# def simulator():
#     # Ensure we have a step counter in session
#     if 'step_index' not in session:
#         session['step_index'] = 0

#     # Load all instructions from output.mc (if it exists)
#     instructions, data_seg = parse_output_mc('output.mc')

#     # Determine action
#     if request.method == 'POST':
#         action = request.form.get('action', '')

#         if action == 'run':
#             # "Run" -> parse final_state.mc & final_data.mc for final registers/memory
#             # Also show all instructions (highlight the last one, or none)
#             session['step_index'] = len(instructions)  # highlight "beyond last"
#             registers = parse_final_state('final_state.mc')
#             memory = parse_final_data('final_data.mc')

#         elif action == 'step':
#             # "Step" -> increment step_index by 1, parse that step from logs
#             # (We assume register_log.mc and memory_log.mc each contain a line per step.)
#             session['step_index'] += 1
#             step_idx = session['step_index']
#             registers = parse_register_log('register_log.mc', step_idx)
#             memory = parse_memory_log('memory_log.mc', step_idx)

#         elif action == 'reset':
#             # "Reset" -> reset step index, revert to default registers/memory
#             session['step_index'] = 0
#             registers = get_default_registers()
#             memory = get_default_memory()

#         elif action == 'assemble':
#             # Rerun the executables with existing input.asm
#             if os.path.exists('input.asm'):
#                 subprocess.run(['./main'])
#                 subprocess.run(['./simulator'])
#             # Optionally reset step index or parse final logs immediately
#             session['step_index'] = 0
#             registers = get_default_registers()
#             memory = data_seg  # show data segment

#         else:
#             # No recognized action
#             registers = get_default_registers()
#             memory = get_default_memory()

#     else:
#         # GET request -> show default registers/memory if nothing else done
#         registers = get_default_registers()
#         memory = get_default_memory()

#     # Now we have registers, memory, instructions, and step_index
#     current_step = session['step_index']

#     # Render simulator
#     return render_template(
#         'simulator.html',
#         registers=registers,
#         memory=memory,
#         instructions=instructions,
#         step_index=current_step
#     )

# ----------------------------------------------------------------------------
# 3. PARSING FUNCTIONS
# ----------------------------------------------------------------------------

import os

def parse_output_mc(filename):
    """
    Reads 'output.mc' lines and separates into instructions vs. data segment.
    Returns (instructions, memory) where:
      - instructions is a list of dicts
      - memory is a list (or dict) of data lines
    """
    if not os.path.exists(filename):
        return [], []  # or however you want to handle missing file

    instructions = []
    memory       = []
    found_data_segment = False

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue  # skip blank lines

            # If we see "Data Segment", switch to reading memory lines
            if line == "Data Segment":
                found_data_segment = True
                continue

            if not found_data_segment:
                #
                # Parse as instruction
                #
                # For example, line might look like:
                #   "0x00001000 0x00000513 addi x10, x0, 0 # comment"
                #
                # We'll do a split with maxsplit=3 to separate them into
                # pc, machine_code, basic_code, and anything else in 'original_code'.
                line_no_comment = line.split('#', 1)[0].strip()
                parts = line_no_comment.split(maxsplit=3)

                # fallback if not enough parts
                pc            = parts[0] if len(parts) > 0 else ''
                machine_code  = parts[1] if len(parts) > 1 else ''
                # basic_code    = parts[2] if len(parts) > 2 else ''
                original_code = parts[3] if len(parts) > 3 else ''

                # Append to instructions list
                instructions.append({
                    'pc': pc,
                    'machine_code': machine_code,
                    'original_code': original_code
                })
            else:
                #
                # Parse as memory data line
                #
                # Typically these lines are like: "0x10000000 0x5"
                # You can store them as a dict, or just keep them in a list
                #
                parts = line.split()
                if len(parts) == 2:
                    addr, val = parts
                    # Convert them if you want
                    # address_int = int(addr, 16)
                    # value_int   = int(val, 16)
                    memory.append((addr, val))
                else:
                    # Possibly skip or handle differently if the format is unexpected
                    memory.append((line, "???"))

    return instructions, memory




def parse_final_state(filename):
    """
    final_state.mc lines:
      x0 = 0x00000000
      x1 = 0x00000000
      ...
    Return list of (register_name, value)
    """
    regs = []
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if line:
                    if '=' in line:
                        reg, val = line.split('=', 1)
                        reg = reg.strip()
                        val = val.strip()
                        regs.append((reg, val))
    return regs


def parse_final_data(filename):
    """
    final_data.mc lines:
      0x10010000 0xABCD1234
    Return list of (mem_loc, value)
    """
    mem = []
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if line:
                    parts = line.split()
                    if len(parts) == 2:
                        mem.append((parts[0], parts[1]))
    return mem


def parse_register_log(filename, step_idx):
    """
    register_log.mc might contain multiple lines per step or 1 line per step. 
    For simplicity, assume 1 line per register per step, separated by steps.
    Example:
      Step 0:
        x0 = 0x00000000
        x1 = 0x00000001
      Step 1:
        x0 = 0x00000000
        x1 = 0x00000002
    We'll parse them in blocks of 32 lines (if each step has 32 registers).
    Adjust logic as needed to match your real log format.
    """
    default = get_default_registers()
    if not os.path.exists(filename):
        return default

    lines = []
    with open(filename, 'r') as f:
        lines = [ln.strip() for ln in f if ln.strip()]

    # Suppose each step has 32 lines, one per register
    regs_per_step = 32
    start = step_idx * regs_per_step
    end = start + regs_per_step

    # If step_idx is beyond the file, return the last known or default
    if start >= len(lines):
        return default

    block = lines[start:end]
    regs = []
    for line in block:
        if '=' in line:
            reg, val = line.split('=', 1)
            reg = reg.strip()
            val = val.strip()
            regs.append((reg, val))

    # If we got fewer than 32 lines, fill up with default or do something
    if len(regs) < regs_per_step:
        # Just add missing lines from default
        existing_reg_names = {r[0] for r in regs}
        for (dreg, dval) in default:
            if dreg not in existing_reg_names:
                regs.append((dreg, dval))

    return regs

def parse_memory_log(filename, step_idx):
    """
    Reads memory changes from memory_log.txt (or similar).
    Returns a dictionary { address_int: byte_val_int } representing memory state at a given step.

    Assumptions (example):
      - Each step in the log is 10 lines of the form "0xADDRESS 0xVALUE".
      - step_idx indicates which 10-line block to parse (0-based).
      - If the file doesn't exist or the step index is out of range, returns default memory.
    """
    # Use a dictionary-based default memory so we can do jump logic, chunking, etc.
    default_mem = get_default_memory_dict()
    if not os.path.exists(filename):
        return default_mem

    # Read all non-empty lines
    with open(filename, "r") as f:
        lines = [ln.strip() for ln in f if ln.strip()]

    # Suppose each step has 10 lines. (Adjust if your log differs.)
    mem_per_step = 10
    start = step_idx * mem_per_step

    # If start index is beyond total lines, just return default memory
    if start >= len(lines):
        return default_mem

    # Extract the block of lines for this step
    block = lines[start:start + mem_per_step]

    # Update our memory dictionary based on the block lines
    for line in block:
        parts = line.split()
        if len(parts) == 2:
            mem_loc_str, val_str = parts
            try:
                # Convert from hex string (e.g. "0x10000000") to int
                addr_int = int(mem_loc_str, 16)
                val_int  = int(val_str, 16)
            except ValueError:
                # If parsing fails, skip this line or handle error
                continue

            # Update memory dictionary at addr_int with val_int
            default_mem[addr_int] = val_int

    return default_mem


# def parse_memory_log(filename, step_idx):
#     """
#     Similar approach to parse_register_log, but memory might be large.
#     For demonstration, assume each step has 10 lines of memory changes, or something like that.
#     Adjust as per your real log format.
#     """
#     default_mem = get_default_memory()
#     if not os.path.exists(filename):
#         return default_mem

#     lines = []
#     with open(filename, 'r') as f:
#         lines = [ln.strip() for ln in f if ln.strip()]

#     # Suppose each step has 10 lines. Adjust to your format.
#     mem_per_step = 10
#     start = step_idx * mem_per_step
#     end = start + mem_per_step

#     if start >= len(lines):
#         return default_mem

#     block = lines[start:end]
#     mem = []
#     for line in block:
#         parts = line.split()
#         if len(parts) == 2:
#             mem_loc, val = parts
#             mem.append((mem_loc, val))

#     # If not enough lines, just fill or ignore
#     if len(mem) < mem_per_step:
#         # Possibly combine with default or do nothing
#         pass

#     return mem


# ----------------------------------------------------------------------------
# 4. DEFAULT VALUES
# ----------------------------------------------------------------------------

def get_default_registers():
    """
    Returns a dictionary of all 32 RISC-V registers (x0..x31) mapped to default values in hex.
    x0 is always 0x00000000. We give x2 (stack pointer) and x3 (global pointer) some typical defaults.
    Adjust as needed for your simulator.
    """
    # Initialize every register to "0x00000000"
    registers = {f"x{i}": "0x00000000" for i in range(32)}

    # x2 is commonly used as the stack pointer; x3 is often the global pointer
    registers["x2"] = "0x7FFFFFFC"  # example default stack pointer
    registers["x3"] = "0x10000000"  # example default global pointer

    # x0 must always be 0 in RISC-V
    registers["x0"] = "0x00000000"

    return registers


# def get_default_registers():
#     """
#     Return a list of (reg, value) for the default state before run/step.
#     Adjust these defaults as desired (like x2 might be the stack pointer).
#     """
#     # Example defaults for 32 registers:
#     defaults = [
#         ("x0 (zero)", "0x00000000"),
#         ("x1 (ra)",    "0x00000000"),
#         ("x2 (sp)",    "0x7FFFFFFC"),
#         ("x3 (gp)",    "0x10000000"),
#         ("x4 (tp)",    "0x00000000"),
#         ("x5 (t0)",    "0x00000000"),
#         ("x6 (t1)",    "0x00000000"),
#         ("x7 (t2)",    "0x00000000"),
#         ("x8 (s0/fp)", "0x00000000"),
#         ("x9 (s1)",    "0x00000000"),
#         ("x10 (a0)",   "0x00000000"),
#         ("x11 (a1)",   "0x00000000"),
#         ("x12 (a2)",   "0x00000000"),
#         ("x13 (a3)",   "0x00000000"),
#         ("x14 (a4)",   "0x00000000"),
#         ("x15 (a5)",   "0x00000000"),
#         ("x16 (a6)",   "0x00000000"),
#         ("x17 (a7)",   "0x00000000"),
#         ("x18 (s2)",   "0x00000000"),
#         ("x19 (s3)",   "0x00000000"),
#         ("x20 (s4)",   "0x00000000"),
#         ("x21 (s5)",   "0x00000000"),
#         ("x22 (s6)",   "0x00000000"),
#         ("x23 (s7)",   "0x00000000"),
#         ("x24 (s8)",   "0x00000000"),
#         ("x25 (s9)",   "0x00000000"),
#         ("x26 (s10)",  "0x00000000"),
#         ("x27 (s11)",  "0x00000000"),
#         ("x28 (t3)",   "0x00000000"),
#         ("x29 (t4)",   "0x00000000"),
#         ("x30 (t5)",   "0x00000000"),
#         ("x31 (t6)",   "0x00000000"),
#     ]
#     return defaults


def get_default_memory():
    """
    Return a small list of (address, value) if you want some default memory visible.
    Otherwise, return empty.
    """
    return [
        ("0x10000000", "0x00000000"),
        ("0x10000004", "0x00000000"),
        ("0x10000008", "0x00000000"),
        ("0x1000000C", "0x00000000"),
    ]

def get_default_memory_dict():
    """
    Returns a dictionary representing the default memory layout for the RISC-V simulator.
    
    This function initializes a contiguous memory region starting at 0x10000000 up to 0x1000FFFF.
    Each address in this range is set to 0 by default. Adjust the base address and region size as
    needed to suit the requirements of your simulator.
    
    Returns:
        dict: A dictionary where each key is an integer address and each value is an integer (0-255)
              representing a byte stored at that address.
    """
    memory = {}
    base_addr = 0x10000000
    region_size = 0x10000  # 64KB of memory
    
    # Initialize each address in the region with 0
    for addr in range(base_addr, base_addr + region_size):
        memory[addr] = 0
    
    return memory


# ----------------------------------------------------------------------------
# 5. RUN
# ----------------------------------------------------------------------------
if __name__ == '__main__':
    app.run(debug=True)
