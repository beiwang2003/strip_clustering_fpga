### Content

This is the OpenCL implementation of the CMS standard "three threshold" algorithm for FPGA. It is based on the parallel CPU and GPU version availble at https://github.com/beiwang2003/strip_clustering_gpu.
The prefix sum implemenation is based on the clpp library at https://code.google.com/archive/p/clpp/downloads

### Building and running

1. setup the env for FPGA <br />
source /opt/intel/fpga-d5005/inteldevstack/init_env.sh (this is the default 19.2 OpenCL SDK)
(we can load 19.4 and 20.1 OpenCL SDK using 
source /opt/intel/fpga-d5005/inteldevstack/init_env_19_4.sh and
source /opt/intel/fpga-d5005/inteldevstack/init_env_20.1.sh)
 
2. offline compile the *.cl kernels in emulation mode <br />
aoc -march=emulator -legacy-emulator ./clppScan_Default.cl <br />
aoc -march=emulator -legacy-emulator ./clustering.cl <br />
aoc -march=emulator -legacy-emulator ./findSeedStrips.cl <br />
(note: option -legacy-emulator is required for compiling using 19.2 OpenCL SDK. For newer version, e.g., 19.4 and 
20.1, the compiler is using -fast-emulator option by default)

3. compile the host code by running Makefile <br />
make <br />

4. run the code in the host <br />
./strip-cluster <br />
