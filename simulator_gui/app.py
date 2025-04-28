import os
import subprocess
from flask import Flask, request, render_template
from werkzeug.utils import secure_filename

# ─── Configuration ──────────────────────────────────────────────────────────────
BASE_DIR           = os.path.dirname(os.path.abspath(__file__))
ASSEMBLER_BIN      = os.path.join(BASE_DIR, 'assembler')
SIMULATOR_BIN      = os.path.join(BASE_DIR, 'simulator')
ALLOWED_EXTENSIONS = {'mc'}
PIPE_REG_KEYWORDS  = ['IF/ID', 'ID/EX', 'EX/MEM', 'MEM/WB']

app = Flask(__name__)


def parse_exec_table(lines):
    idx = next((i for i, l in enumerate(lines)
                if l.lstrip().startswith('Instruction')), None)
    if idx is None:
        return [], []
    header = lines[idx].split()
    cycles = header[1:]
    rows = []
    for line in lines[idx + 2:]:
        if not line.strip():
            continue
        name = line[:15].strip()
        stages = [
            line[i : i + 4].strip()
            for i in range(15, 15 + 4 * len(cycles), 4)
        ]
        rows.append({'name': name, 'stages': stages})
    return cycles, rows


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        # ————————————————————————————————————————————————
        # 1) Grab code from editor
        asm_code = request.form.get('asm_code', '').rstrip()
        if not asm_code:
            return "Error: no assembly code provided.", 400

        # 2) Write it out to input.asm
        asm_path = os.path.join(BASE_DIR, 'input.asm')
        with open(asm_path, 'w') as f:
            f.write(asm_code)

        # 3) Clean up any previous output.mc
        mc_path = os.path.join(BASE_DIR, 'output.mc')
        if os.path.exists(mc_path):
            os.remove(mc_path)

        # 4) Run assembler → output.mc
        asm_proc = subprocess.run(
            [ASSEMBLER_BIN, asm_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=BASE_DIR
        )
        asm_out = asm_proc.stdout.splitlines()

        # 5) If assembly failed, show assembler errors immediately
        if asm_proc.returncode != 0:
            return render_template(
                'result.html',
                error=True,
                stats={},
                cycles=[],
                instr_rows=[],
                reg_cycles_logs=[],
                pipe_cycles_logs=[],
                bp_cycles_logs=[],
                trace_regs=False,
                trace_pipe=False,
                trace_bp=False,
                asm_errors=asm_out
            )

        # 6) Ensure output.mc exists
        if not os.path.exists(mc_path):
            return f"Error: assembler did not produce output.mc", 500

        # — now exactly as before, but use output.mc as input ——

        # 7) Read knob settings
        knobs = {
            'pipeline':   request.form.get('pipeline')   == 'on',
            'forwarding': request.form.get('forwarding') == 'on',
            'trace_regs': request.form.get('trace_regs') == 'on',
            'trace_pipe': request.form.get('trace_pipe') == 'on',
            'trace_bp':   request.form.get('trace_bp')   == 'on',
            'trace_inst': request.form.get('trace_inst') == 'on',
            'trace_idx':  request.form.get('trace_inst_idx')
        }

        # 8) Remove old stats file
        stats_file = os.path.join(BASE_DIR, 'pipeline_sim.txt')
        if os.path.exists(stats_file):
            os.remove(stats_file)

        # 9) Build simulator cmd
        sim_cmd = [SIMULATOR_BIN]
        if not knobs['pipeline']:
            sim_cmd.append('--no-pipeline')
        if not knobs['forwarding']:
            sim_cmd.append('--no-forward')
        if knobs['trace_regs']:
            sim_cmd.append('--trace-regs')
        if knobs['trace_pipe']:
            sim_cmd.append('--trace-pipe')
        if knobs['trace_inst'] and knobs['trace_idx']:
            sim_cmd += ['--trace-inst', knobs['trace_idx']]
        if knobs['trace_bp']:
            sim_cmd.append('--trace-bp')
        sim_cmd.append(mc_path)

        # 10) Run simulator
        sim_proc = subprocess.run(
            sim_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=BASE_DIR
        )
        sim_out = sim_proc.stdout.splitlines()

        # 11) Split off the exec‐table
        split_i = next((i for i, l in enumerate(sim_out)
                        if l.lstrip().startswith('Instruction')), None)
        logs = sim_out[:split_i] if split_i is not None else sim_out
        exec_lines = sim_out[split_i:] if split_i is not None else []

        # 12) Chunk raw logs into cycles
        raw_cycles = []
        cur = []
        for ln in logs:
            if ln.startswith('[Cycle '):
                if cur:
                    raw_cycles.append(cur)
                cur = [ln]
            else:
                if cur:
                    cur.append(ln)
        if cur:
            raw_cycles.append(cur)

        # 13) Classify each cycle
        reg_cycles, pipe_cycles, bp_cycles = [], [], []
        for cycle in raw_cycles:
            regs, pipes, bps = [], [], []
            for ln in cycle:
                if ln.startswith('[Cycle '):
                    continue
                if ln.startswith('Branch Predictor State') or ln.strip().startswith('PC='):
                    bps.append(ln)
                elif any(k in ln for k in PIPE_REG_KEYWORDS):
                    pipes.append(ln)
                else:
                    regs.append(ln)
            reg_cycles.append(regs)
            pipe_cycles.append(pipes)
            bp_cycles.append(bps)

        # 14) If simulation failed, show its raw logs
        if sim_proc.returncode != 0:
            return render_template(
                'result.html',
                error=True,
                stats={},
                cycles=[],
                instr_rows=[],
                reg_cycles_logs=reg_cycles,
                pipe_cycles_logs=pipe_cycles,
                bp_cycles_logs=bp_cycles,
                trace_regs=knobs['trace_regs'],
                trace_pipe=knobs['trace_pipe'],
                trace_bp=knobs['trace_bp'],
                asm_errors=[]
            )

        # 15) Parse execution‐table
        cycles, instr_rows = parse_exec_table(exec_lines)

        # 16) Read stats
        stats = {}
        if os.path.exists(stats_file):
            with open(stats_file) as sf:
                for L in sf:
                    if ':' in L:
                        k, v = L.strip().split(':', 1)
                        stats[k.strip()] = v.strip()

        # 17) Render final
        return render_template(
            'result.html',
            error=False,
            stats=stats,
            cycles=cycles,
            instr_rows=instr_rows,
            reg_cycles_logs=reg_cycles,
            pipe_cycles_logs=pipe_cycles,
            bp_cycles_logs=bp_cycles,
            trace_regs=knobs['trace_regs'],
            trace_pipe=knobs['trace_pipe'],
            trace_bp=knobs['trace_bp'],
            asm_errors=[]
        )

    # GET: show editor + knobs
    return render_template('index.html')


if __name__ == '__main__':
    app.run(debug=True)
