#!/bin/bash
../../../../../pin -t ../../obj-intel64/approxload.so -- ../../../../../../perfect/suite/pa1/kernels/ser/dwt53/dwt53
python ../../error_script.py gold.mat dwt53_output.small.0.mat
