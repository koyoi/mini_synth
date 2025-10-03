"""
Generate a C++ header with MIDI note (0-127) -> SVF normalized frequency f table.
f is computed as: f = 2 * sin(pi * fc / fs)
where fc is an exponential map from note to frequency (e.g., note to frequency using mtof)
This script writes MiniSynthNoteTable.h into the project root include folder.
"""
import math

# Parameters
fs = 16384.0  # audio rate used in project
min_fc = 80.0
max_fc = 6000.0

# MIDI note to frequency (A4 = 69 -> 440Hz)
def mtof(note):
    return 440.0 * (2.0 ** ((note - 69) / 12.0))

notes = list(range(128))
# Map note to target fc. We want lower notes to have fc scaled lower; use note frequency to scale.
# Option: map note frequency proportionally between min_fc and max_fc via log mapping.

fc_values = []
for n in notes:
    # use midi frequency normalized to range 0..1 between midi note 0 and 127
    note_freq = mtof(n)
    # map note_freq in log-domain between min_fc and max_fc
    # compute normalized position based on midi note number instead (simpler)
    t = n / 127.0
    fc = min_fc * (max_fc / min_fc) ** t
    fc_values.append(fc)

f_values = []
for fc in fc_values:
    f = 2.0 * math.sin(math.pi * fc / fs)
    # clamp to reasonable range
    if f > 0.999:
        f = 0.999
    f_values.append(f)

# write header
out_path = "c:/Users/N2232/no_OneDrive/Arduino/mini_synth/MiniSynthNoteTable.h"
with open(out_path, "w", encoding="utf-8") as fh:
    fh.write("#pragma once\n\n")
    fh.write("// Generated note->f table (MIDI 0..127)\n")
    fh.write("static const float kNoteFTable[128] = {\n")
    for i, v in enumerate(f_values):
        fh.write(f"    {v:.8ff}, // {i}\n")
    fh.write("};\n")

print("Wrote", out_path)
