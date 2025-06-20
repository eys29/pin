#!/bin/bash

cd source/tools/approx
make obj-intel64/bitflip.so TARGET=intel64 
make obj-intel64/meminfo.so TARGET=intel64