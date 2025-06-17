#!/bin/bash
../../../../../pin -t ../../obj-intel64/approxload.so -- ../../../../../../perfect/suite/pa1/kernels/ser/histogram_equalization/histogram_equalization
python ../../error_script.py gold.mat histeq_output.small.0.mat
