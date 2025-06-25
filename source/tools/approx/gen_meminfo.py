
import os
import subprocess
import sys
import time
from multiprocessing import Pool, cpu_count

# benchmark name : (executable, golden output, output mat)
start_path = "../../../../perfect/suite/"
benchmarks = {
    "2dconv":  ("pa1/kernels/ser/2d_convolution/2d_convolution",   
                "pa1/output/2dconv_output.small.mat",
                "2dconv_output.small.0.mat"),
    "dwt53":   ("pa1/kernels/ser/dwt53/dwt53",                      
                "pa1/output/dwt53_output.small.mat",
                "dwt53_output.small.0.mat"),
    "histeq":  ("pa1/kernels/ser/histogram_equalization/histogram_equalization",
                "pa1/output/histeq_output.small.mat",
                "histeq_output.small.0.mat"),
    "lucas":    ("wami/kernels/ser/lucas-kanade/wami-lucas-kanade",
                "wami/inout/small_golden.mat",
                "output.mat"),
    "debayer": ("wami/kernels/ser/debayer/wami-debayer",
                "wami/inout/small_golden_debayer_output.mat",
                "output.mat")
}

benchmark = sys.argv[1]
start_instr = 0
num_instr = 0
if len(sys.argv) == 4:
    start_instr = int(sys.argv[2])
    num_instr = int(sys.argv[3])
executable = start_path + benchmarks[benchmark][0]
error_script = "error_script.py"
meminfo_out = "meminfo.out"
meminfo_info = "meminfo.info"  
# Generate meminfo file
subprocess.check_output(["../../../pin", 
                            "-t", 
                            "obj-intel64/meminfo.so", 
                            "--", 
                            executable])
print("Finished meminfo")
subprocess.check_output(["rm", f"{benchmarks[benchmark][2]}"])


# Read instructions to process
with open(meminfo_info, "r") as total_instrs_file:
    total_instrs = int(total_instrs_file.readline())

with open(meminfo_out, "r") as meminfo_file:
    all_lines = meminfo_file.readlines()

before = len(all_lines)
print("before " + str(before))
seen_addresses = set()

with open(benchmark + "_meminfo.out", 'w') as outfile:
    for line in all_lines:
        parts = line.strip().split()
        if len(parts) < 3:
            continue
        try:
            address = int(parts[2])
        except ValueError:
            continue
        if address not in seen_addresses:
            seen_addresses.add(address)
            outfile.write(line)

# Optionally overwrite all_lines if you still need it
after = len(all_lines)
print("after " + str(after))
print("reduced % " + str((before-after)/before))
