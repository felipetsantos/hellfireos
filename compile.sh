cd usr/sim/mpsoc_sim/
make clean
make noc_$1
cd ../../../platform/noc_$1
make clean
make images
rm -rf ../../usr/sim/mpsoc_sim/objects/*.bin
cp *.bin ../../usr/sim/mpsoc_sim/objects
