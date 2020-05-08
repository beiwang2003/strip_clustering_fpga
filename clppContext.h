#ifndef __CLPP_CONTEXT_H__
#define __CLPP_CONTEXT_H__

#if defined (__APPLE__) || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

enum clppVendor { Vendor_Unknown, Vendor_NVidia, Vendor_AMD, Vendor_Intel };

#define STRING_BUFFER_LEN 1024

class clppContext
{
 public:

  clppContext();

  ~clppContext();

  cl_context clContext;			// OpenCL context
  cl_platform_id clPlatform;		// OpenCL Platform
  cl_device_id clDevice;		// OpenCL Device
  cl_command_queue clQueue;		// OpenCL command queue 
  
  // Setup with a specific platform and device
  void setup(unsigned int platformId, unsigned int deviceId);

  // Informations
  clppVendor Vendor;
  
  void device_info_ulong( cl_device_id device, cl_device_info param, const char* name);
  void device_info_uint( cl_device_id device, cl_device_info param, const char* name);
  void device_info_bool( cl_device_id device, cl_device_info param, const char* name);
  void device_info_string( cl_device_id device, cl_device_info param, const char* name);
  void display_device_info( cl_device_id device );
  
 private:
  
  // Case-insensitive strstr() work-alike.
  static char* stristr(const char *String, const char *Pattern);
};

#endif
