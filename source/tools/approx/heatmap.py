import os
import subprocess

# Input file
import sys

meminfo_out = "meminfo.out"
executable = "perfect/suite/pa1/kernels/ser/2d_convolution/2d_convolution"
expected_out = "expected.out"
error_script = "error_2dconv.py"
error_out = "error.out"
heatmap_out = "heatmap.out"


# Build everything
subprocess.run(["make", "obj-intel64/bitflip.so", "TARGET=intel64"])
subprocess.run(["make", "obj-intel64/meminfo.so", "TARGET=intel64"])
print("Finished building")

# Generate instructions
with open(meminfo_out, "w") as meminfo_file:
    subprocess.run(["../../../pin", 
                    "-t", 
                    "obj-intel64/meminfo.so", 
                    "--", 
                    executable], 
                    stdout=meminfo_file)
print("Finished meminfo")

with open(meminfo_out, "r") as meminfo_file:
    for line in meminfo_file:
        parts = line.split()
        instr = int(parts[0])
        num_bytes = int(parts[1])
        num_bits = num_bytes * 8 
        addr = parts[2]

        with open(heatmap_out, "a") as heatmap_file:
            heatmap_file.write(str(addr))
        
            for bit in range(num_bits):
                print(f"instr {instr} bit {bit}")
                snr = 0
                try:
                    bitflip_out = subprocess.check_output(["../../../pin", 
                                                        "-t", 
                                                        "obj-intel64/bitflip.so", 
                                                        "--", 
                                                        executable, 
                                                        str(instr), 
                                                        str(bit)],
                                                        timeout=10)
                    snr_out = subprocess.check_output(["python", 
                                                error_script, 
                                                "2dconv_output.small.0.mat", 
                                                "perfect/suite/pa1/output/2dconv_output.small.mat"])
                    str_snr = str(snr_out).split("\\n")
                    snr = float(str_snr[1].strip())

                except subprocess.CalledProcessError:
                    snr = -float('inf')
                except subprocess.TimeoutExpired:
                    snr = -float('inf')
                heatmap_file.write(f" {str(snr)}")
            heatmap_file.write("\n")
                
                



            