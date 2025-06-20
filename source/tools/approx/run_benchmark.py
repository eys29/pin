import os
import subprocess
import sys
import threading
import time
# how to run 
# python3 run_benchmark.py benchmarkname startinstr numinstr

# benchmark name : (executable, golden output, output mat)
start_path = "../../../../perfect/suite/"
benchmarks = {
    "2dconv" :  ("pa1/kernels/ser/2d_convolution/2d_convolution",   
                 "pa1/output/2dconv_output.small.mat",
                 "2dconv_output.small.0.mat"),
    "dwt53" :   ("pa1/kernels/ser/dwt53/dwt53",                      
                 "pa1/output/dwt53_output.small.mat",
                 "dwt53_output.small.0.mat"),
    "histeq" :  ("pa1/kernels/ser/histogram_equalization/histogram_equalization",
                 "pa1/output/histeq_output.small.mat",
                 "histeq_output.small.0.mat"),
    "lk" :      ("wami/kernels/ser/lucas-kanade/wami-lucas-kanade",
                 "wami/inout/small_golden.mat",
                 "output.mat"),
    "debayer" : ("wami/kernels/ser/debayer/wami-debayer",
                 "wami/inout/small_golden_debayer_output.mat",
                 "output.mat")

}
meminfo_out = "meminfo.out"
meminfo_info = "meminfo.info"
benchmark = sys.argv[1]
executable = start_path + benchmarks[benchmark][0]
error_script = "error_script.py"




start_time = time.time()

def process_meminfo_line(line):
    parts = line.split()
    instr = int(parts[0])
    num_bytes = int(parts[1])
    num_bits = 10 #num_bytes * 8 
    addr = parts[2]

    output_string = str(instr) + " " + str(addr)
    
    for bit in range(num_bits):
        # print(f"instr {instr} bit {bit}")
        num_runs = 10
        snr_arr = [] 
        for i in range(num_runs):
            try:
                subprocess.check_output(["../../../pin", 
                                        "-t", 
                                        "obj-intel64/bitflip.so", 
                                        "--", 
                                        executable, 
                                        str(instr), 
                                        str(bit*2)],
                                        stderr=subprocess.DEVNULL,  # suppress error output
                                        timeout=30)
                snr_out = subprocess.check_output(["python", 
                                                error_script, 
                                                f"{benchmarks[benchmark][2]}.{instr}.{bit*2}", 
                                                start_path + benchmarks[benchmark][1]])
                str_snr = str(snr_out).split("\\n")
                # print(str_snr)
                snr_arr.append(float(str_snr[1].strip()))
                
            except subprocess.CalledProcessError:
                snr_arr.append("e") 
            except subprocess.TimeoutExpired:
                snr_arr.append("t")
            
        average_snr = 0
        if "e" in snr_arr:
            average_snr = "e"
        elif "t" in snr_arr:
            average_snr = "t"
        else:
            for j in range(num_runs):
                average_snr += snr_arr[j]
            average_snr /= num_runs
            
        output_string += f" {str(average_snr)}"
        

    print(output_string)
    


# Generate instructions - don't do this every run so that mem info is same across runs
subprocess.check_output(["../../../pin", 
                    "-t", 
                    "obj-intel64/meminfo.so", 
                    "--", 
                    executable])
print("Finished meminfo")
total_instrs = 0

with open(meminfo_info, "r") as total_instrs_file:
    line = total_instrs_file.readline()
    total_instrs = int(line)

threads = []
start_instr = int(sys.argv[2])
last_instr = start_instr + int(sys.argv[3])
counter = 0
with open(meminfo_out, "r") as meminfo_file:
    for line in meminfo_file:
        if counter >= start_instr and counter < last_instr:
            thread = threading.Thread(target=process_meminfo_line, args=(line,))
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
