#include "clppProgram.h"
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h> // readlink, chdir

clppProgram::clppProgram()
{
  _clProgram = 0;
  _context = 0;
}

clppProgram::~clppProgram()
{
  cl_int clStatus;
  
  if (_clProgram)
    {
      clStatus = clReleaseProgram(_clProgram);
      checkCLStatus(clStatus);
    }
}

// Loads a file in binary form.
unsigned char *clppProgram::loadBinaryFile(const char *file_name, size_t *size) {
  // Open the File 
  FILE* fp;
  long ftell_size;
  size_t elements_read;
  
  fp = fopen(file_name, "rb");
  if(fp == 0) {
    return NULL;
  }

  // Get the size of the file
  fseek(fp, 0, SEEK_END);
  ftell_size = ftell(fp);
  if (ftell_size < 0) {
    fclose(fp);
    return NULL;
  }
  *size = (unsigned)ftell_size;
  
  // Allocate space for the binary
  unsigned char *binary = new unsigned char[*size];
  
  // Go back to the file start 
  rewind(fp);
  
  // Read the file into the binary
  elements_read = fread((void*)binary, *size, 1, fp);
  if(elements_read == 0) {
    delete[] binary;
    fclose(fp);
    return NULL;
  }
  
  fclose(fp);
  return binary;

}

bool clppProgram::createProgramFromBinary(clppContext* context, string prefix)
{
  cl_int clStatus;
  
  _context = context;
  
  // Early exit for potentially the most common way of fail: AOCX does not exist
  std::string binary_name = std::string(prefix) + ".aocx";
  if (!(access(binary_name.c_str(), R_OK) != -1)) {
    printf("AOCX file '%s' does not exist.\n", binary_name.c_str());
    return false;
  }
  printf("Using AOCX: %s\n", binary_name.c_str());
  
  // Load the binary 
  size_t binary_size;
  
  unsigned char *binary = loadBinaryFile(binary_name.c_str(), &binary_size);
  if (binary == NULL) {
    printf("Failed to load binary file '%s'", binary_name.c_str());
    return false;
  }
  
  _clProgram = clCreateProgramWithBinary(_context->clContext, 1, &(_context->clDevice), &binary_size,
					 (const unsigned char **)&binary, NULL, &clStatus);
  if (clStatus != CL_SUCCESS)
    {
      printf("Failed to create program with binary '%s'", binary_name.c_str());
      return false;
    }
  
  clStatus = clBuildProgram(_clProgram, 0, NULL, "", NULL, NULL);
  
  if (clStatus != CL_SUCCESS)
    {
      size_t len;
      char buffer[5000];
      printf("Error: Failed to build program executable!\n");
      clGetProgramBuildInfo(_clProgram, context->clDevice, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
      
      printf("%s\n", buffer);
      printf("%s\n", getOpenCLErrorString(clStatus));
      
      checkCLStatus(clStatus);
      return false;
    }
  
  return true;

}

const char* clppProgram::getOpenCLErrorString(cl_int err)
{
  switch (err) {
  case CL_SUCCESS: return "Success!";
  case CL_DEVICE_NOT_FOUND: return "Device not found.";
  case CL_DEVICE_NOT_AVAILABLE: return "Device not available";
  case CL_COMPILER_NOT_AVAILABLE: return "Compiler not available";
  case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "Memory object allocation failure";
  case CL_OUT_OF_RESOURCES: return "Out of resources";
  case CL_OUT_OF_HOST_MEMORY: return "Out of host memory";
  case CL_PROFILING_INFO_NOT_AVAILABLE: return "Profiling information not available";
  case CL_MEM_COPY_OVERLAP: return "Memory copy overlap";
  case CL_IMAGE_FORMAT_MISMATCH: return "Image format mismatch";
  case CL_IMAGE_FORMAT_NOT_SUPPORTED: return "Image format not supported";
  case CL_BUILD_PROGRAM_FAILURE: return "Program build failure";
  case CL_MAP_FAILURE: return "Map failure";
  case CL_INVALID_VALUE: return "Invalid value";
  case CL_INVALID_DEVICE_TYPE: return "Invalid device type";
  case CL_INVALID_PLATFORM: return "Invalid platform";
  case CL_INVALID_DEVICE: return "Invalid device";
  case CL_INVALID_CONTEXT: return "Invalid context";
  case CL_INVALID_QUEUE_PROPERTIES: return "Invalid queue properties";
  case CL_INVALID_COMMAND_QUEUE: return "Invalid command queue";
  case CL_INVALID_HOST_PTR: return "Invalid host pointer";
  case CL_INVALID_MEM_OBJECT: return "Invalid memory object";
  case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "Invalid image format descriptor";
  case CL_INVALID_IMAGE_SIZE: return "Invalid image size";
  case CL_INVALID_SAMPLER: return "Invalid sampler";
  case CL_INVALID_BINARY: return "Invalid binary";
  case CL_INVALID_BUILD_OPTIONS: return "Invalid build options";
  case CL_INVALID_PROGRAM: return "Invalid program";
  case CL_INVALID_PROGRAM_EXECUTABLE: return "Invalid program executable";
  case CL_INVALID_KERNEL_NAME: return "Invalid kernel name";
  case CL_INVALID_KERNEL_DEFINITION: return "Invalid kernel definition";
  case CL_INVALID_KERNEL: return "Invalid kernel";
  case CL_INVALID_ARG_INDEX: return "Invalid argument index";
  case CL_INVALID_ARG_VALUE: return "Invalid argument value";
  case CL_INVALID_ARG_SIZE: return "Invalid argument size";
  case CL_INVALID_KERNEL_ARGS: return "Invalid kernel arguments";
  case CL_INVALID_WORK_DIMENSION: return "Invalid work dimension";
  case CL_INVALID_WORK_GROUP_SIZE: return "Invalid work group size";
  case CL_INVALID_WORK_ITEM_SIZE: return "Invalid work item size";
  case CL_INVALID_GLOBAL_OFFSET: return "Invalid global offset";
  case CL_INVALID_EVENT_WAIT_LIST: return "Invalid event wait list";
  case CL_INVALID_EVENT: return "Invalid event";
  case CL_INVALID_OPERATION: return "Invalid operation";
  case CL_INVALID_GL_OBJECT: return "Invalid OpenGL object";
  case CL_INVALID_BUFFER_SIZE: return "Invalid buffer size";
  case CL_INVALID_MIP_LEVEL: return "Invalid mip-map level";
  default: return "Unknown";
  }
}

void clppProgram::checkCLStatus(cl_int clStatus)
{
  const char* e = getOpenCLErrorString(clStatus);
  assert(clStatus == CL_SUCCESS);
}

void clppProgram::waitCompletion()
{
  clFinish(_context->clQueue);
}
