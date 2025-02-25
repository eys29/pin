import os
import subprocess
import sys
import threading
import time

meminfo_out = "meminfo.out"
executable = "perfect/suite/pa1/kernels/ser/2d_convolution/2d_convolution"
error_script = "error_2dconv.py"
heatmap_out = "heatmap_multi.out"

heatmap_lock = threading.Lock()

start_time = time.time()

def process_meminfo_line(line):
    parts = line.split()
    instr = int(parts[0])
    num_bytes = int(parts[1])
    num_bits = num_bytes * 8 
    addr = parts[2]

    output_string = str(addr)
    
    for bit in range(num_bits):
        # print(f"instr {instr} bit {bit}")
        snr = 0
        try:
            subprocess.check_output(["../../../pin", 
                                     "-t", 
                                     "obj-intel64/bitflip.so", 
                                     "--", 
                                     executable, 
                                     str(instr), 
                                     str(bit)],
                                     timeout=10)
            snr_out = subprocess.check_output(["python", 
                                               error_script, 
                                               f"2dconv_output.small.0.mat.{instr}.{bit}", 
                                               "perfect/suite/pa1/output/2dconv_output.small.mat"])
            str_snr = str(snr_out).split("\\n")
            # print(str_snr)
            snr = float(str_snr[1].strip())
            
        except subprocess.CalledProcessError:
            snr = -float('inf')
        except subprocess.TimeoutExpired:
            snr = -float('inf')
        subprocess.run(["rm",
                        f"2dconv_output.small.0.mat.{instr}.{bit}"])
        output_string += f" {str(snr)}"
        
    
    output_string += "\n"
    
    with heatmap_lock:
        with open(heatmap_out, "a") as heatmap_file:
            heatmap_file.write(output_string)




# remove .out files
subprocess.run(["rm", meminfo_out])
subprocess.run(["rm", heatmap_out])

# Build everything
subprocess.run(["make", "obj-intel64/bitflip.so", "TARGET=intel64"])
subprocess.run(["make", "obj-intel64/meminfo.so", "TARGET=intel64"])
print("Finished building")

# Generate instructions
subprocess.run(["../../../pin", 
                    "-t", 
                    "obj-intel64/meminfo.so", 
                    "--", 
                    executable])
print("Finished meminfo")

threads = []
with open(meminfo_out, "r") as meminfo_file:
    for line in meminfo_file:
        thread = threading.Thread(target=process_meminfo_line, args=(line,))
        thread.start()
        threads.append(thread)

for thread in threads:
    thread.join()


end_time = time.time()
total_time = (end_time - start_time) / 60
print(f"Total execution time: {total_time:.2f} minutes")
