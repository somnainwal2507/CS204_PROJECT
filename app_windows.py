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
        subprocess.run(['./main.exe'])
        subprocess.run(['./instruction_execution.exe'])

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

    step_idx = session['step_index']
    highlight_pc_int = None

    # 1) Load instructions & data segment from output.mc (if it exists)
    instructions, data_seg = parse_output_mc('output.mc')

    # 2) Determine action if POST
    if request.method == 'POST':
        action = request.form.get('action', '')

        if action == 'run':
            # Parse final state of registers/memory
            session['step_index'] = len(instructions)  # highlight "beyond last"
            registers = parse_final_state('final_state.mc')
            raw_memory_dict = parse_final_data('final_data.mc', data_seg)
            current_pc = None

        elif action == 'step':
            session['step_index'] += 1
            step_idx = session['step_index']
            registers, highlight_pc_int = parse_register_log('register_log.mc', step_idx)
            raw_memory_dict = parse_memory_log('memory_log.mc', step_idx, data_seg)

        elif action == 'reset':
            session['step_index'] = 0
            registers = get_default_registers()
            raw_memory_dict = data_seg
            current_pc = None
        else:
            registers = get_default_registers()
            raw_memory_dict = data_seg
            current_pc = None
    else:
        # GET request: show default registers/memory
        registers = get_default_registers()
        raw_memory_dict = data_seg
        current_pc = None

    # 3) Handle "Jump to" logic
    address_hex = request.args.get('address', None)
    move_cmd    = request.args.get('move', None)
    jump_addr   = None
    if address_hex:
        try:
            jump_addr = int(address_hex, 16)
        except ValueError:
            jump_addr = None
    if jump_addr and move_cmd:
        WORD_SIZE = 4
        SHIFT_COUNT = 10
        offset = WORD_SIZE * SHIFT_COUNT
        if move_cmd == 'up':
            jump_addr += offset
        elif move_cmd == 'down':
            jump_addr -= offset

    # 4) Determine memory window starting address
    # Use the jump address if provided; otherwise, default to data segment start.
    default_start_addr = 0x10000000
    window_start_addr = jump_addr if jump_addr is not None else default_start_addr

    # Clamp the window_start_addr in case the user enters a value beyond our range.
    if window_start_addr < 0:
        window_start_addr = 0
    max_mem_addr = 0x7FFFFFFC
    if window_start_addr > max_mem_addr:
        window_start_addr = max_mem_addr

    # Instead of chunking the entire memory, create a windowed view.
    memory_chunked = window_memory(raw_memory_dict, window_start_addr, num_words=40)

    # 5) Get the current step index (for other uses if needed)
    current_step = session['step_index']

    # 6) Read code from input.asm to pass to the template
    code = ''
    if os.path.exists('input.asm'):
        with open('input.asm', 'r') as f:
            code = f.read()
    
    return render_template(
        'simulator.html',
        instructions=instructions,
				registers=registers,
        memory=memory_chunked,
				step_index=step_idx,
        highlight_pc_int=highlight_pc_int,
				jump_addr=jump_addr,
        code=code
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
    for addr_str in data_seg:
        try:
            addr_int = int(addr_str, 16)
            val_int  = int(data_seg[addr_str], 16)
        except ValueError:
            # skip or handle error
            continue
        mem[addr_int] = val_int
    return mem



# ----------------------------------------------------------------------------
# 3. PARSING FUNCTIONS
# ----------------------------------------------------------------------------

def parse_output_mc(filename):
    if not os.path.exists(filename):
        return [], {}
    
    instructions = []
    memory = get_default_memory_dict()
    found_data_segment = False

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            # Check for the data segment boundary.
            if line == "Data Segment":
                found_data_segment = True
                continue

            if not found_data_segment:
                # Example: "0x00001000 0x00000513 addi x10, x0, 0 # comment"
                line_no_comment = line.split('#', 1)[0].strip()
                parts = line_no_comment.split(maxsplit=3)
                pc_str = parts[0] if len(parts) > 0 else ''
                machine_code = parts[1] if len(parts) > 1 else ''
                original_code = parts[3] if len(parts) > 3 else ''
                
                try:
                    # Convert the PC string to an integer (base 16)
                    pc_int = int(pc_str, 16)
                except ValueError:
                    pc_int = None
                
                instructions.append({
                    'pc': pc_str,       # The original string for display
                    'pc_int': pc_int,   # The integer version for comparing
                    'machine_code': machine_code,
                    'original_code': original_code
                })
            else:
                # Code for reading memory (unchanged)
                parts = line.split()
                if len(parts) == 2:
                    addr_str, val_str = parts
                    try:
                        addr_int = int(addr_str, 16)
                        val_int = int(val_str, 16)
                        memory[addr_int] = val_int
                    except ValueError:
                        pass
    
    return instructions, memory

def parse_final_state(filename):
    regs = {}
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if line and '=' in line and line.startswith("x"):
                    reg, val = line.split('=', 1)
                    regs[reg.strip()] = val.strip()
    return regs



def parse_final_data(filename, initial_state=None):
    """
    Parses final_data.mc as a diff that contains only the memory changes made after the initial state.
    
    Parameters:
        filename (str): Path to the final_data.mc file.
        initial_state (dict, optional): A dictionary with the initial memory state. If provided,
                                        the changes in final_data.mc will be merged on top of it.
    
    Returns:
        dict: A dictionary representing the updated memory state.
    """
    # Start with a copy of the initial state if provided; otherwise, an empty dict.
    if initial_state is None:
        initial_state = {}
    mem = initial_state.copy()

    if not os.path.exists(filename):
        return mem  # return the initial state (or empty dict) if file doesn't exist

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue  # skip blank lines

            # Expecting lines of the form "0xADDRESS = 0xVALUE"
            parts = line.split('=', 1)
            if len(parts) != 2:
                continue  # ignore lines that do not match the format

            addr_str = parts[0].strip()  # e.g. "0x10000000"
            val_str = parts[1].strip()   # e.g. "0xABC01234"

            try:
                # Convert hex strings to integers
                addr_int = int(addr_str, 16)
                val_int = int(val_str, 16)
            except ValueError:
                continue  # skip this line if conversion fails

            # Update the memory state with the diff
            mem[addr_int] = val_int

    return mem

def parse_register_log(filename, step_idx):
    """
    Returns (registers, highlight_pc_int) where highlight_pc_int is the PC (as an integer)
    from the last log entry up to the current step.
    """
    registers = get_default_registers()
    highlight_pc_int = None

    if not os.path.exists(filename):
        return registers, highlight_pc_int

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or ':' not in line:
                continue

            try:
                # Split timestamp and the remaining part of the log entry.
                timestamp_part, rest = line.split(":", 1)
                clock = int(timestamp_part.strip(), 16)

                # Only consider entries up to the current step index.
                if clock > step_idx:
                    continue

                # Split the remaining part by commas.
                segments = rest.split(',')
                for seg in segments:
                    seg = seg.strip()
                    if seg.startswith("PC="):
                        # Extract the PC value
                        pc_str = seg.split('=', 1)[1].strip()
                        highlight_pc_int = int(pc_str, 16)
                    else:
                        # For register updates, split on '='.
                        parts = seg.split('=', 1)
                        if len(parts) == 2:
                            reg = parts[0].strip()
                            val = parts[1].strip()
                            registers[reg] = val
            except Exception as e:
                continue

    return registers, highlight_pc_int



def parse_memory_log(filename, current_step, initial_state):
    """
    Processes the memory_log.mc file and returns the memory state after applying all updates
    whose timestamp is less than or equal to current_step.

    Parameters:
        filename (str): Path to the memory log file.
        current_step (int): The current simulation step (used to filter out future updates).
        initial_state (dict): The initial memory state (e.g. read from output.mc).
    
    Returns:
        dict: The memory state with updates merged from all log entries whose
              timestamp is less than or equal to current_step.
    """
    # Start with a copy of the initial state.
    state = initial_state.copy()
    
    if not os.path.exists(filename):
        return state

    with open(filename, 'r') as f:
        lines = [line.strip() for line in f if line.strip()]
    
    for line in lines:
        try:
            timestamp_part, rest = line.split(":", 1)
            timestamp = int(timestamp_part.strip())
        except Exception:
            continue  # Skip malformed lines
        
        # Skip updates that are for a future step.
        if timestamp > current_step:
            continue

        # Split the rest by comma; ignore the PC part and handle update parts.
        parts = rest.split(',')
        for part in parts:
            part = part.strip()
            if part.startswith("PC="):
                continue
            # Expected part format: "0x1000000C = 0x05"
            subparts = part.split('=', 1)
            if len(subparts) != 2:
                continue
            addr_str = subparts[0].strip()
            val_str = subparts[1].strip()
            try:
                addr_int = int(addr_str, 16)
                val_int = int(val_str, 16)
            except Exception:
                continue
            # Update memory at this address.
            state[addr_int] = val_int
    return state



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

    # Set stack pointer and global pointer
    registers["x2"] = "0x7FFFFFFC"  
    registers["x3"] = "0x10000000" 

    # x0 must always be 0 in RISC-V
    registers["x0"] = "0x00000000"

    return registers


# def get_default_registers():
#     """
#     Return a list of (reg, value) for the default state before run/step.
#     """
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

def window_memory(mem_dict, start_addr, num_words=40):
    """
    Create a list of (word_address, [byte0, byte1, byte2, byte3]) tuples 
    starting at start_addr for num_words words.
    The address range is clamped so that it does not exceed the maximum (0x7FFFFFFC).
    """
    max_mem_addr = 0x7FFFFFFC
    result = []
    addr = start_addr
    for _ in range(num_words):
        # Ensure that we do not exceed max memory address for the first byte of the word.
        if addr > max_mem_addr:
            break
        # Fetch 4 bytes for the word. If not in mem_dict, default to 0.
        word_bytes = [
            mem_dict.get(addr, 0),
            mem_dict.get(addr + 1, 0),
            mem_dict.get(addr + 2, 0),
            mem_dict.get(addr + 3, 0)
        ]
        result.append((addr, word_bytes))
        addr += 4
    return result


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
    # memory = {}
    # base_addr = 0x10000000
    # region_size = 0x10000  # 64KB of memory
    
    # # Initialize each address in the region with 0
    # for addr in range(base_addr, base_addr + region_size):
    #     memory[addr] = 0
    
    return {}


# ----------------------------------------------------------------------------
# 5. RUN
# ----------------------------------------------------------------------------
if __name__ == '__main__':
    app.run(debug=True)
