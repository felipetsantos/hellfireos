#!/bin/bash
cd platform/single_core
make image
../../usr/sim/hf_risc_sim/hf_risc_sim image.bin
cd ../../
