AOCL_COMPILE_CONFIG=$(shell aocl compile-config)
AOCL_LINK_CONFIG=$(shell aocl link-config)

CC = g++
CXXFLAGS = -std=c++17 -fPIC -g -DCALIB_1D -DUSE_FPGA -DEMULATOR -DOUTPUT #-DNDRANGE #-DOUTPUT
SRCS = $(wildcard *.cc)
OBJS = $(SRCS:.cc=.o)

OCLSRCS = $(wildcard *.cl)

strip-cluster: $(OBJS)
	$(CC) -o $@ $^ $(AOCL_LINK_CONFIG)

%.o: %.cc
	$(CC) -o $@ -c $< $(CXXFLAGS) $(AOCL_COMPILE_CONFIG)

clean:
	rm -rf $(OBJS) strip-cluster
