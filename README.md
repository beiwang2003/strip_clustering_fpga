
### Content

This is the OpenCL implementation of the CMS standard "three threshold" algorithm for FPGA. It is based on the parallel CPU and GPU version availble at https://github.com/beiwang2003/strip_clustering_gpu.
The prefix sum implemenation is based on the clpp library at https://code.google.com/archive/p/clpp/downloads

### Building and running on the host for debugging in emulation mode

1. setup the env for FPGA <br />
source /opt/intel/fpga-d5005/inteldevstack/init_env.sh (this is the default 19.2 OpenCL SDK. We can load 19.4 and 20.1 OpenCL SDK using 
source /opt/intel/fpga-d5005/inteldevstack/init_env_19_4.sh and
source /opt/intel/fpga-d5005/inteldevstack/init_env_20.1.sh)
 
2. compile the *.cl kernels in emulation mode for debugging <br />
aoc -march=emulator ./clppScan_Default.cl <br />
aoc -march=emulator ./clustering.cl <br />
aoc -march=emulator ./findSeedStrips.cl <br />
(note: option -legacy-emulator is required for compiling using 19.2 OpenCL SDK. For newer version, e.g., 19.4 and 
20.1, the compiler is using -fast-emulator option by default)

The above steps will result in 3 different directories, one for each file. To generate a single *.cl file, we can use #include "*.cl" to include the first two in the third one.

3. setup the env for the host
module load rh/devtoolset/9

4. compile the host code by running Makefile <br />
make <br />
(use EMULATOR flag in the makfile for running in emulation mode)
(use make -n to check the the compiler options added to the compiling process) <br />

5. run the code in the host (for 19.4+ version) <br /> 
env LD_LIBRARY_PATH=/usr/licensed/anaconda3/2020.7/lib:$LD_LIBRARY_PATH ./strip-cluster <br />

(use ldd ./strip-cluster to check if the object file is linked to the right shared libraries) <br />

Note: the emulator in Prof Edition is built with GCC 7.2.0 and so the libstdc++.so have to be at least as new as provided in GCC 7.2.0, libstdc++.so.6.0.24. See: https://www.intel.com/content/dam/www/programmable/us/en/pdfs/literature/hb/opencl-sdk/aocl_getting_started.pdf#page=39. The devtoolkit provided at RHEL 7 system at Princeton does not provide the required libstdc++ version. Fortunately, anaconda carries libstdc++.so.6.0.26 which is from GCC 9.1.0. To link to that library, we will need to set up the env as indidcated ablove <br />

### Offling kernel compiling and optimization (see section 2 in aocl_programming_guide.pdf)
1. intermediate compilation with -c option (generate <kernel_filename>.aoco file) <br />
aoc -c <kernel_filename>.cl -report -v <br />

2. intermediate compilation with -rtl option (generate <kernel_filename>.aocr file) <br />
aoc -rtl <kernel_filename>.cl -report -v <br />

3. review html reprt at <kernel_filename>/reports/report.html <br />

4*. simulate (generate <kernel_filename>.aocx file> <br />
aoc -march=simulator <kernel_filename>.cl <br />

5*. compile (generate <kernel_filename>.aocx: minutes to hours) <br />
aoc -fast-compile <kernel_filename>.cl (fast compilation for small design changes) <br />
aoc -incremental <kernel_filename>.cl (incremental compilation for big design changes) <br />

6. full deployment with profiling turned on (generate <kernel_filename>.aocx: hours) <br />
aoc -profile <kernel_filename>.cl -report <br />

### run the compiled code with compiled host 
1. setup the env for the host
module load rh/devtoolset/9

2. compile the host code by running Makefile <br />
make <br />
(do NOT use EMULATOR flag in the makfile for running on board)

3. run the code in the host (for 19.4+ version) <br />
env LD_LIBRARY_PATH=/usr/licensed/anaconda3/2020.7/lib:$LD_LIBRARY_PATH ./strip-cluster <br />
(this will generate a profile.mon file for compiling with -profile option for the *.cl file)

4. Post-processing the profile.mon data into a readable profile.json file for data processing with Intel VTune
aocl profile ./strip-cluster -x unifiedKernels.cl -s unifiedKernels.source 
(this will generate a profile.json for further data processing with Intel VTune)

5. Instead of using Intel VTune, we can also launch the Intel FPGA Dynamic Profiler for OpenCL GUI
aocl report *.aocx profile.mon *.source 

### Managing FPGA board 
1. query the device name of your fpga board <br />
aocl diagnose (look for device_name, e.g., acl0, alc1, ...) <br />

2. run a board diagnostic test <br />
aocl diagnose device_name <br />

3. program the FPGA offline or without a host <br />
aocl program device_name <kernel_filename>.aocx <br />

Note: it is important to program the board before running board diagnostic test, e.g., aocl diagnose device name

4. 3. compile the host code by running Makefile <br />
make <br />

4. run the code in the host <br />
./strip-cluster <br >
