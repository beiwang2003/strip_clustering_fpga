#include "findSeedStrips.h"

findSeedStrips::findSeedStrips(clppContext *context) {

  if (!createProgramFromBinary(context, "findSeedStrips")) return;

  cl_int clStatus;

  _kernel_setSeedStrips = clCreateKernel(_clProgram, "setSeedStrips", &clStatus);

  _kernel_setNCSeedStrips = clCreateKernel(_clProgram, "setNCSeedStrips", &clStatus);

  _kernel_setStripIndex = clCreateKernel(_clProgram, "setStripIndex", &clStatus);

}

void findSeedStrips::setSeedStrips(sst_data_cl_t *sst_data_cl, calib_data_cl_t* calib_data_cl) {

  cl_int clStatus;

  clStatus = clSetKernelArg(_kernel_setSeedStrips, 0, sizeof(cl_mem), (void *)&sst_data_cl->stripId);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 1, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsMask);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 2, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCMask);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 3, sizeof(cl_mem), (void *)&sst_data_cl->adc);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 4, sizeof(cl_mem), (void *)&calib_data_cl->noise);
  clStatus = clSetKernelArg(_kernel_setSeedStrips, 5, sizeof(uint16_t), (void *)&invStrip);

  size_t global_size = sst_data_cl->nStrips;

  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setSeedStrips, 1, NULL, &global_size, NULL, 0, NULL, NULL);
}

void findSeedStrips::setNCSeedStrips(sst_data_cl_t *sst_data_cl) {

  cl_int clStatus;

  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 0, sizeof(cl_mem), (void *)&sst_data_cl->detId);
  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 1, sizeof(cl_mem), (void *)&sst_data_cl->stripId);
  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 2, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsMask);
  clStatus = clSetKernelArg(_kernel_setNCSeedStrips, 3, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCMask);

  size_t global_size = sst_data_cl->nStrips;

  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setNCSeedStrips, 1, NULL, &global_size, NULL, 0, NULL, NULL);

}

void findSeedStrips::setStripIndex(sst_data_cl_t *sst_data_cl) {

  cl_int clStatus;

  clStatus = clSetKernelArg(_kernel_setStripIndex, 0, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCMask);
  clStatus = clSetKernelArg(_kernel_setStripIndex, 1, sizeof(cl_mem), (void *)&sst_data_cl->prefixSeedStripsNCMask);
  clStatus = clSetKernelArg(_kernel_setStripIndex, 2, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCIndex);

  size_t global_size = sst_data_cl->nStrips;

  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_setStripIndex, 1, NULL, &global_size, NULL, 0, NULL, NULL);

}
