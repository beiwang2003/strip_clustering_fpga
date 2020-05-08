#include "clppContext.h"

#include<assert.h>
#include <iostream>
#include <string.h>

using namespace std;

clppContext::clppContext() {
  setup(0, 0);
}

clppContext::~clppContext() {
  cl_int clStatus;
  if (clContext)
    clStatus = clReleaseContext(clContext);
  assert(clStatus == CL_SUCCESS);
}

void clppContext::setup(unsigned int platformId, unsigned int deviceId)
{
  Vendor = Vendor_Unknown;
  
  cl_int clStatus;
  
  size_t infoLen;
  char infoStr[STRING_BUFFER_LEN];
  cl_device_type infoType;
  
  //---- Retreive information about platforms
  cl_uint platformsCount;
  clStatus = clGetPlatformIDs(0, NULL, &platformsCount);
  assert(clStatus == CL_SUCCESS);
  
  cl_platform_id* platforms = new cl_platform_id[platformsCount];
  clStatus = clGetPlatformIDs(platformsCount, platforms, NULL);
  assert(clStatus == CL_SUCCESS);
  
  platformId = min(platformId, platformsCount - 1);
  clPlatform = platforms[platformId];
  
  printf("Querying platform for info:\n");      
  printf("==========================\n");       
  clGetPlatformInfo(clPlatform, CL_PLATFORM_NAME, sizeof(infoStr), infoStr, &infoLen);
  printf("%-40s = %s\n", "CL_PLATFORM_NAME", infoStr);    
  clGetPlatformInfo(clPlatform, CL_PLATFORM_VENDOR, sizeof(infoStr), infoStr, &infoLen);
  printf("%-40s = %s\n", "CL_PLATFORM_VENDOR", infoStr);
  if (stristr(infoStr, "Intel") != NULL)
    Vendor = Vendor_Intel;
  else if (stristr(infoStr, "AMD") != NULL)
    Vendor = Vendor_AMD;
  else if (stristr(infoStr, "NVidia") != NULL)
    Vendor = Vendor_NVidia;
  else if (stristr(infoStr, "Apple") != NULL)
    Vendor = Vendor_NVidia;
  clGetPlatformInfo(clPlatform, CL_PLATFORM_VERSION, sizeof(infoStr), infoStr, &infoLen);
  printf("%-40s = %s\n\n", "CL_PLATFORM_VERSION ", infoStr);  
  
  //---- Devices
  cl_uint devicesCount;
  clStatus = clGetDeviceIDs(clPlatform, CL_DEVICE_TYPE_ALL, 0, NULL, &devicesCount);
  assert(clStatus == CL_SUCCESS);
  assert(devicesCount > 0);
  
  cl_device_id* devices = new cl_device_id[devicesCount];
  clStatus = clGetDeviceIDs(clPlatform, CL_DEVICE_TYPE_ALL, devicesCount, devices, NULL);
  assert(clStatus == CL_SUCCESS);
  
  clDevice = devices[min(deviceId, devicesCount - 1)];
  //---- Display Device info
  display_device_info(clDevice);
  
  //---- Context
  clContext = clCreateContext(0, 1, &clDevice, NULL, NULL, &clStatus);
  assert(clStatus == CL_SUCCESS);
  
  //---- Queue
  clQueue = clCreateCommandQueue(clContext, clDevice, 0, &clStatus);
  assert(clStatus == CL_SUCCESS);
}

void clppContext::device_info_ulong( cl_device_id device, cl_device_info param, const char* name) {
  cl_ulong a;
  clGetDeviceInfo(device, param, sizeof(cl_ulong), &a, NULL);
  printf("%-40s = %lu\n", name, a);
}

void clppContext::device_info_uint( cl_device_id device, cl_device_info param, const char* name) {
  cl_uint a;
  clGetDeviceInfo(device, param, sizeof(cl_uint), &a, NULL);
  printf("%-40s = %u\n", name, a);
}

void clppContext::device_info_bool( cl_device_id device, cl_device_info param, const char* name) {
  cl_bool a;
  clGetDeviceInfo(device, param, sizeof(cl_bool), &a, NULL);
  printf("%-40s = %s\n", name, (a?"true":"false"));
}

void clppContext::device_info_string( cl_device_id device, cl_device_info param, const char* name) {
  char a[STRING_BUFFER_LEN];
  clGetDeviceInfo(device, param, sizeof(a), &a, NULL);
  printf("%-40s = %s\n", name, a);
}

void clppContext::display_device_info( cl_device_id device ) {

  printf("Querying device for info:\n");
  printf("========================\n");
  device_info_string(device, CL_DEVICE_NAME, "CL_DEVICE_NAME");
  device_info_string(device, CL_DEVICE_VENDOR, "CL_DEVICE_VENDOR");
  device_info_uint(device, CL_DEVICE_VENDOR_ID, "CL_DEVICE_VENDOR_ID");
  device_info_string(device, CL_DEVICE_VERSION, "CL_DEVICE_VERSION");
  device_info_string(device, CL_DRIVER_VERSION, "CL_DRIVER_VERSION");
  device_info_uint(device, CL_DEVICE_ADDRESS_BITS, "CL_DEVICE_ADDRESS_BITS");
  device_info_bool(device, CL_DEVICE_AVAILABLE, "CL_DEVICE_AVAILABLE");
  device_info_bool(device, CL_DEVICE_ENDIAN_LITTLE, "CL_DEVICE_ENDIAN_LITTLE");
  device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE");
  device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE");
  device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_SIZE, "CL_DEVICE_GLOBAL_MEM_SIZE");
  device_info_bool(device, CL_DEVICE_IMAGE_SUPPORT, "CL_DEVICE_IMAGE_SUPPORT");
  device_info_ulong(device, CL_DEVICE_LOCAL_MEM_SIZE, "CL_DEVICE_LOCAL_MEM_SIZE");
  device_info_ulong(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, "CL_DEVICE_MAX_CLOCK_FREQUENCY");
  device_info_ulong(device, CL_DEVICE_MAX_COMPUTE_UNITS, "CL_DEVICE_MAX_COMPUTE_UNITS");
  device_info_ulong(device, CL_DEVICE_MAX_CONSTANT_ARGS, "CL_DEVICE_MAX_CONSTANT_ARGS");
  device_info_ulong(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE");
  device_info_uint(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS");
  device_info_uint(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, "CL_DEVICE_MEM_BASE_ADDR_ALIGN");
  device_info_uint(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE");
  device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR");
  device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT");
  device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT");
  device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG");
  device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT");
  device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE");

  {
    cl_command_queue_properties ccp;
    clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &ccp, NULL);
    printf("%-40s = %s\n", "Command queue out of order? ", ((ccp & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)?"true":"false"));
    printf("%-40s = %s\n", "Command queue profiling enabled? ", ((ccp & CL_QUEUE_PROFILING_ENABLE)?"true":"false"));
  }
}

char* clppContext::stristr(const char *String, const char *Pattern)
{
    char *pptr, *sptr, *start;
    unsigned int slen, plen;
    for (start = (char *)String,
            pptr = (char *)Pattern,
            slen = strlen(String),
            plen = strlen(Pattern);
            // while string length not shorter than pattern length
            slen >= plen;
            start++, slen--)
    {
        // find start of pattern in string
        while (toupper(*start) != toupper(*Pattern))
        {
            start++;
            slen--;
            // if pattern longer than string
            if (slen < plen)
                return(NULL);
        }
        sptr = start;
        pptr = (char *)Pattern;
        while (toupper(*sptr) == toupper(*pptr))
        {
            sptr++;
            pptr++;
            // if end of pattern then pattern was found
            if ('\0' == *pptr)
                return (start);
        }
    }
    return(NULL);
}
