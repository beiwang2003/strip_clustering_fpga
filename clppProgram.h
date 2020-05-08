#ifndef __CLPP_PROGRAM_H__
#define __CLPP_PROGRAM_H__

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <assert.h>
#include <math.h>

#if defined (__APPLE__) || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include "clppContext.h"

using namespace std;

class clppProgram
{
public:
  clppProgram();
  virtual ~clppProgram();
  
  // Wait for the end of the program
  virtual void waitCompletion();
  
  // Helper method : use to retreive textual error message
  static void checkCLStatus(cl_int clStatus);
  
  // Load binary file
  unsigned char *loadBinaryFile(const char *file_name, size_t *size);
  bool createProgramFromBinary(clppContext* context, string prefix);
  
 protected:
  cl_program _clProgram;
  clppContext* _context;

  static const char* getOpenCLErrorString(cl_int err);
  
  static size_t toMultipleOf(size_t N, size_t base) 
  {
    return (ceil((double)N / (double)base) * base);
  }
};

#endif
