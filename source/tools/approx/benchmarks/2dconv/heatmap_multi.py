import os
import subprocess
import sys
import threading
import time

meminfo_out = "meminfo.out"
meminfo_info = "meminfo.info"
executable = "../../perfect/suite/pa1/kernels/ser/2d_convolution/2d_convolution"
error_script = "../../error_script.py"
heatmap_out = "heatmap_multi.out"

num_samples = 4

locks = [threading.Lock() for _ in range(num_samples)]


start_time = time.time()

def process_meminfo_line(line, sample):
    parts = line.split()
    instr = int(parts[0])
    num_bytes = int(parts[1])
    num_bits = 10 #num_bytes * 8 
    addr = parts[2]

    output_string = str(instr) + " " + str(addr)
    
    for bit in range(num_bits):
        # print(f"instr {instr} bit {bit}")
        num_runs = 5
        snr_arr = [] 
        for i in range(num_runs):
            try:
                subprocess.check_output(["../../../../../pin", 
                                        "-t", 
                                        "../../obj-intel64/bitflip.so", 
                                        "--", 
                                        executable, 
                                        str(instr), 
                                        str(bit*2)], 
                                        timeout=30)
                snr_out = subprocess.check_output(["python", 
                                                error_script, 
                                                f"2dconv_output.small.0.mat.{instr}.{bit*2}", 
                                                "../../perfect/suite/pa1/output/2dconv_output.small.mat"])
                str_snr = str(snr_out).split("\\n")
                # print(str_snr)
                snr_arr.append(float(str_snr[1].strip()))
                
            except subprocess.CalledProcessError:
                snr_arr.append("e") 
            except subprocess.TimeoutExpired:
                snr_arr.append("t")
            subprocess.run(["rm",
                            f"2dconv_output.small.0.mat.{instr}.{bit*2}"])
        average_snr = 0
        for i in range(5):
            if "e" in snr_arr:
                average_snr = "e"
            elif "t" in snr_arr:
                average_snr = "t"
            else:
                average_snr = (snr_arr[0] + snr_arr[1] + snr_arr[2] + snr_arr[3] + snr_arr[4]) / 5
                
        output_string += f" {str(average_snr)}"
        
    
    output_string += "\n"
    
    with locks[sample]:
        with open("sample"+str(sample)+".out", "a") as heatmap_file:
            heatmap_file.write(output_string)




# remove .out files
# subprocess.run(["rm", meminfo_out])
# subprocess.run(["rm", heatmap_out])
for sample in range(num_samples):
    subprocess.run(["rm", "sample"+str(sample)+".out"])

# Build everything
# subprocess.run(["make", "obj-intel64/bitflip.so", "TARGET=intel64"])
# subprocess.run(["make", "obj-intel64/meminfo.so", "TARGET=intel64"])
# print("Finished building")

# Generate instructions - don't do this every run so that mem info is same across runs
# subprocess.run(["../../../pin", 
#                     "-t", 
#                     "obj-intel64/meminfo.so", 
#                     "--", 
#                     executable])
# print("Finished meminfo")
total_instrs = 0

with open(meminfo_info, "r") as total_instrs_file:
    line = total_instrs_file.readline()
    total_instrs = int(line)

start_instrs = [0, int(total_instrs / 4), int(total_instrs / 2), int(3 * total_instrs / 4)]
threads = []
for sample in range(num_samples):
    start_instr = start_instrs[sample]
    last_instr = start_instr + 100
    counter = 0
    with open(meminfo_out, "r") as meminfo_file:
        for line in meminfo_file:
            if counter >= start_instr and counter < last_instr:
                thread = threading.Thread(target=process_meminfo_line, args=(line,sample))
                thread.start()
                threads.append(thread)
            if counter >= last_instr:
                break

            counter += 1

    for thread in threads:
        thread.join()


end_time = time.time()
total_time = (end_time - start_time) / 60
print(f"Total execution time: {total_time:.2f} minutes")
