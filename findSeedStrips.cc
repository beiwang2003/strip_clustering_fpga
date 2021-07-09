#include "findSeedStrips.h"

findSeedStrips::findSeedStrips(clppContext *context) {

  //  if (!createProgramFromBinary(context, "findSeedStrips")) return;
  if (!createProgramFromBinary(context, "unifiedKernels")) return;

  cl_int clStatus;

  _kernel_setSeedStrips = clCreateKernel(_clProgram, "setSeedStrips", &clStatus);

  _kernel_setNCSeedStrips = clCreateKernel(_clProgram, "setNCSeedStrips", &clStatus);

  _kernel_setStripIndex = clCreateKernel(_clProgram, "setStripIndex", &clStatus);

  checkCLStatus(clStatus);
}

void findSeedStrips::setSeedStrips(sst_data_cl_t *sst_data_cl, calib_data_cl_t* calib_data_cl) {

  cl_int clStatus;

  size_t global_size = sst_data_cl->nStrips;

  clStatus = clSetKernelArg(_kernel_setSeedStrips, 0, sizeof(cl_mem), (void *)&sst_data_cl->stripId);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 1, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsMask);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 2, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCMask);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 3, sizeof(cl_mem), (void *)&sst_data_cl->adc);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 4, sizeof(cl_mem), (void *)&calib_data_cl->noise);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 5, sizeof(cl_ushort), (void *)&invStrip);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 6, sizeof(cl_int), (void *)&global_size);

  checkCLStatus(clStatus);

#ifdef NDRANGE
  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setSeedStrips, 1, NULL, &global_size, NULL, 0, NULL, NULL);
#else
  size_t single_size = 1;
  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setSeedStrips, 1, NULL, &single_size, &single_size, 0, NULL, NULL);
#endif
  checkCLStatus(clStatus);
}

void findSeedStrips::setNCSeedStrips(sst_data_cl_t *sst_data_cl) {

  cl_int clStatus;

  size_t global_size = sst_data_cl->nStrips;

  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 0, sizeof(cl_mem), (void *)&sst_data_cl->detId);
  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 1, sizeof(cl_mem), (void *)&sst_data_cl->stripId);
  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 2, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsMask);
  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 3, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCMask);
  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 4, sizeof(cl_int), (void *)&global_size);

  checkCLStatus(clStatus);

#ifdef NDRANGE
  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setNCSeedStrips, 1, NULL, &global_size, NULL, 0, NULL, NULL);
#else
  size_t single_size=1;
  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setNCSeedStrips, 1, NULL, &single_size, &single_size, 0, NULL, NULL);
#endif

  checkCLStatus(clStatus);
}

void findSeedStrips::setStripIndex(sst_data_cl_t *sst_data_cl) {

  cl_int clStatus;

  size_t global_size = sst_data_cl->nStrips;

  clStatus = clSetKernelArg(_kernel_setStripIndex, 0, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCMask);
  clStatus = clSetKernelArg(_kernel_setStripIndex, 1, sizeof(cl_mem), (void *)&sst_data_cl->prefixSeedStripsNCMask);
  clStatus = clSetKernelArg(_kernel_setStripIndex, 2, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCIndex);
  clStatus = clSetKernelArg(_kernel_setStripIndex, 3, sizeof(cl_int), (void *)&global_size);

  checkCLStatus(clStatus);
#ifdef NDRANGE
  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setStripIndex, 1, NULL, &global_size, NULL, 0, NULL, NULL);
#else
  size_t single_size=1;
  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setStripIndex, 1, NULL, &single_size, &single_size, 0, NULL, NULL);
#endif
  checkCLStatus(clStatus);

}
