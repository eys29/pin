#!/bin/bash

test_file=$1
meminfo="meminfo.out"
executable="perfect/suite/pa1/kernels/ser/2d_convolution/2d_convolution"
expected_out="expected.out"
actual_out="actual.out"
error_out="error.out"

#build everything
make obj-intel64/bitflip.so TARGET=intel64
make obj-intel64/meminfo.so TARGET=intel64
# g++ "${test_file}.cpp" -o $executable 

echo "finished building"

#run without error insertion
# "./${executable}" > $expected_out

#generate instructions 
../../../pin -t obj-intel64/meminfo.so -- "./${executable}" > $meminfo
echo "finished meminfo"
while read line; do
    instr=$(cut -d' ' -f1 <<< $line)
    num_bytes=$(cut -d' ' -f2 <<< $line)
    num_bits=$((num_bytes * 8 - 1))
    addr=$(cut -d' ' -f3 <<< $line)
    if [[ $(( instr % 100 )) == 0 ]]; then
        echo "running instruction ${instr} ${num_bits} ${addr}"
    fi 
    # for loop is only doing one bit at a time
    # should do every combination and count how many flips are in a combination
    for bit in $(seq 0 $num_bits); do
        echo "bit ${bit}"

        ../../../pin -t obj-intel64/bitflip.so -- "./${executable}" $instr $bit > $actual_out
        # tolerable= python script that compares expected and actual under error threshold
        python error_2dconv.py 2dconv_output.small.0.mat perfect/suite/pa1/output/2dconv_output.small.mat 
        # write $address $bit tolerable or not

    done 
done < $meminfo
# rm $meminfo
# rm "$1.temp"