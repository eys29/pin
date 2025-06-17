#!/bin/bash
../../../../../pin -t ../../obj-intel64/approxload.so -- ../../../../../../perfect/suite/pa1/kernels/ser/2d_convolution/2d_convolution
python ../../error_script.py gold.mat 2dconv_output.small.0.mat
