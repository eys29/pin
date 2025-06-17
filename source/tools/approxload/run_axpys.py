import subprocess

subprocess.run(["g++", "xy.cpp", "-o", "xy"])

subprocess.run(["g++", "iaxpy.cpp", "-o", "iaxpy"])
subprocess.run(["g++", "faxpy.cpp", "-o", "faxpy"])
subprocess.run(["g++", "daxpy.cpp", "-o", "daxpy"])


subprocess.run(["g++", "iaxpy.cpp", "-S"])
subprocess.run(["g++", "faxpy.cpp", "-S"])
subprocess.run(["g++", "daxpy.cpp", "-S"])



num_runs = 100
for j in range(1,31):
    iaxpy_err = 0
    faxpy_err = 0
    daxpy_err = 0
    for i in range(num_runs):
        
        range0 = 0
        range1 = range0 + 2**j
        subprocess.run(["./xy", str(range0), str(range1)])
        iaxpy_lax = subprocess.check_output([
            "../../../pin",
            "-t",
            "obj-intel64/approxload.so",
            "--",
            "./iaxpy"
        ])
        faxpy_lax = subprocess.check_output([
            "../../../pin",
            "-t",
            "obj-intel64/approxload.so",
            "--",
            "./faxpy"
        ])
        daxpy_lax = subprocess.check_output([
            "../../../pin",
            "-t",
            "obj-intel64/approxload.so",
            "--",
            "./daxpy"
        ])

        iaxpy_golden = subprocess.check_output(["./iaxpy"])
        faxpy_golden = subprocess.check_output(["./faxpy"])
        daxpy_golden = subprocess.check_output(["./daxpy"])

        

        iaxpy_err += 100*abs((int(iaxpy_lax) - int(iaxpy_golden))/int(iaxpy_golden))
        faxpy_err += 100*abs((float(faxpy_lax) - float(faxpy_golden))/float(faxpy_golden))
        daxpy_err += 100*abs((float(daxpy_lax) - float(daxpy_golden))/float(daxpy_golden))

    print("average % error over " + str(num_runs) + " samples, range [" + str(range0) + ", " + str(range1) +"]" )
    print("iaxpy % error: " + str(iaxpy_err/num_runs))
    print("faxpy % error: " + str(faxpy_err/num_runs))
    print("daxpy % error: " + str(daxpy_err/num_runs))