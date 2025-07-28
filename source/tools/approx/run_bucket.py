import multiprocessing
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






def bucket_bitflip(bucket_id, num_bitflips, num_runs, bucket_start, bucket_size):

    wrong = 1.01
    timeout = 1.011
    output_string = f"{bucket_id},"
    
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
                                         timeout=100)
                
                metric_out = subprocess.check_output(["python3", 
                                                   error_script, 
                                                   f"{benchmarks[benchmark][2]}.{flip}.{bucket_start}", 
                                                   start_path + benchmarks[benchmark][1]])
                subprocess.check_output(["rm", f"{benchmarks[benchmark][2]}.{flip}.{bucket_start}"])
                str_metric = metric_out.decode().split("\n")
                metric = float(str_metric[1].strip())
                metric_arr.append(1 if metric > 1 else metric)
         
            except subprocess.CalledProcessError:
                metric_arr.append(wrong) # error
            except subprocess.TimeoutExpired:
                metric_arr.append(timeout) # timeout
        
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
    
    print("buckets," + str(num_buckets))
    print("bitflips," + str(num_bitflips))
    print("runs," + str(num_runs))



    try: 
        wcout = subprocess.check_output(["wc", "-l", meminfo_out])
        num_loads = int(wcout.split()[0])
        print("loads," + str(num_loads))
        bucket_size = num_loads // num_buckets 
        print("bucket_size," + str(bucket_size))
        print()

        lscpu = subprocess.check_output(["lscpu"])
        print(lscpu.decode())

        #print header
        header = "bucket_id,"
        for i in range(num_bitflips):
            header += f"error_rate={i}/32,"
        print(header)

        args_list = []
        for bucket in range(num_buckets):
            start = bucket * bucket_size
            size = num_loads - start if bucket == num_buckets - 1 else bucket_size
            args_list.append((bucket, num_bitflips, num_runs, start, size))


        # Run in parallel
        with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
            pool.starmap(bucket_bitflip, args_list)

    except subprocess.CalledProcessError:
        print("can't get line count")


    total_time = (time.time() - start_time) / 60
    print(f"Total execution time: {total_time:.2f} minutes")
