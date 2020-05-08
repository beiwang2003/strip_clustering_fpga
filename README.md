### Content

This is the OpenCL implementation of the CMS standard "three threshold" algorithm for FPGA. It is based on the parallel CPU and GPU version availble at https://github.com/beiwang2003/strip_clustering_gpu/tree/debug.
The prefix sum implemenation is based on the clpp library at https://code.google.com/archive/p/clpp/downloads

### Building and running

1. setup the env for FPGA 
source ./opencl.sh

2. offline compile the *.cl kernels in emulation mode
aoc -march=emulator -legacy=emulator ./clppScan_Default.cl
aoc -march=emulator -legacy=emulator ./clustering.cl
aoc -march=emulator -legacy=emulator ./findSeedStrips.cl

3. compile the host code by running Makefile
make

4. run the code in the host
./strip-cluster
