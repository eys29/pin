import subprocess
import sys
import time

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
executable = start_path + benchmarks[benchmark][0]
error_script = "error_script.py"
meminfo_out = benchmark+"_meminfo.out"






def bucket_bitflip(num_bitflips, num_runs, bucket_start, bucket_size):

    wrong = 1.01
    timeout = 1.011
    output_string = ""
    
    for flip in range(num_bitflips):
        metric_arr = []
        for i in range(num_runs):
            try:
                subprocess.check_output(["../../../pin", 
                                         "-t", 
                                         "obj-intel64/bucketbitflip.so", 
                                         "--", 
                                         executable, #random_bitflips, bucket_start, bucket_size, meminfo_filename
                                         str(flip),
                                         str(bucket_start), 
                                         str(bucket_size),
                                         meminfo_out
                                         ],
                                         stderr=subprocess.DEVNULL,
                                         timeout=5)
                
                metric_out = subprocess.check_output(["python3", 
                                                   error_script, 
                                                   f"{benchmarks[benchmark][2]}.{flip}.{bucket_start}", 
                                                   start_path + benchmarks[benchmark][1]])
                subprocess.check_output(["rm", f"{benchmarks[benchmark][2]}.{flip}.{bucket_start}"])
                str_metric = metric_out.decode().split("\n")
                metric = float(str_metric[1].strip())
                metric_arr.append(1 if metric > 1 else metric)
                # if metric > 1:
                #     print("WEIRD ")
                #     print("\t" + str(str_metric))
                #     print("\t" + str(metric))
            except subprocess.CalledProcessError:
                metric_arr.append(wrong) # error
            except subprocess.TimeoutExpired:
                metric_arr.append(timeout) # timeout
        
        # if "e" in snr_arr:
        #     output_string += "e"
        #     snr_arr.remove("e")
        # elif "t" in snr_arr:
        #     output_string += "t"
        #     snr_arr.remove("t")
        
        average_metric = sum(metric_arr) / num_runs
        
        output_string += f"{average_metric},"
    
    print(output_string, flush=True)


if __name__ == "__main__":
    start_time = time.time()
    num_buckets = 10000
    num_bitflips = 16
    num_runs = 10
    debug = 10 #num_buckets

    print(benchmark)
    lscpu = subprocess.check_output(["lscpu"])
    print()
    print(lscpu.decode())
    print("buckets," + str(num_buckets))
    print("bitflips," + str(num_bitflips))
    print("runs," + str(num_runs))


    #debug
    # num_bitflips = 8
    # num_runs = 10
    # bucket_start = 200
    # bucket_size = 300
    # bucket_bitflip(num_bitflips, num_runs, bucket_start, bucket_size)
    


    try: 
        wcout = subprocess.check_output(["wc", "-l", meminfo_out])
        num_loads = int(wcout.split()[0])
        print("loads," + str(num_loads))
        bucket_size = num_loads // num_buckets 
        print("bucket size," + str(bucket_size))

        # for bucket in range(num_buckets):
        #     bucket_start = bucket * bucket_size
        #     if (bucket == num_buckets - 1):
        #         bucket_size = num_loads[0] - bucket_start
        #     bucket_bitflip(num_bitflips, num_runs, bucket_start, bucket_size)
        args_list = []
        for bucket in range(debug):
            start = bucket * bucket_size
            size = num_loads - start if bucket == num_buckets - 1 else bucket_size
            bucket_bitflip(num_bitflips, num_runs, start, size)


        # Run in parallel
        # with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
        #     pool.starmap(bucket_bitflip, args_list)

    except subprocess.CalledProcessError:
        print("can't get line count")



    # with open(meminfo_out, "r") as meminfo_file:
    #     all_lines = meminfo_file.readlines()
    # # Use multiprocessing Pool
    # with Pool(processes=cpu_count()) as pool:
    #     if len(sys.argv) == 4:
    #         lines_to_process = all_lines[start_instr:min(start_instr + num_instr, len(all_lines))]
    #         pool.map(process_meminfo_line, lines_to_process)
    #     else: 
    #         pool.map(process_meminfo_line, all_lines)

    total_time = (time.time() - start_time) / 60
    print(f"Total execution time: {total_time:.2f} minutes")
