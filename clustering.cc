#include "clustering.h"

clustering::clustering(clppContext *context) {

  if (!createProgramFromBinary(context, "clustering")) return;

  // ----- prepare all kernels
  cl_int clStatus;

  _kernel_findBoundary = clCreateKernel(_clProgram, "findLeftRightBoundary", &clStatus);
  checkCLStatus(clStatus);

  _kernel_checkCondition = clCreateKernel(_clProgram, "checkClusterCondition", &clStatus);
  checkCLStatus(clStatus);

}

void clustering::findBoundary(sst_data_cl_t *sst_data_cl, calib_data_cl_t* calib_data_cl, clust_data_cl_t* clust_data_cl) {

  cl_int clStatus;

  clStatus = clSetKernelArg(_kernel_findBoundary, 0, sizeof(int),  (void *)&sst_data_cl->nStrips);
  clStatus = clSetKernelArg(_kernel_findBoundary, 1, sizeof(cl_mem), (void *)&sst_data_cl->detId);
  clStatus = clSetKernelArg(_kernel_findBoundary, 2, sizeof(cl_mem), (void *)&sst_data_cl->stripId);
  clStatus = clSetKernelArg(_kernel_findBoundary, 3, sizeof(cl_mem), (void *)&sst_data_cl->adc);
  clStatus = clSetKernelArg(_kernel_findBoundary, 4, sizeof(cl_mem), (void *)&sst_data_cl->seedStripsNCIndex);
  clStatus = clSetKernelArg(_kernel_findBoundary, 5, sizeof(cl_mem), (void *)&calib_data_cl->noise);
  clStatus = clSetKernelArg(_kernel_findBoundary, 6, sizeof(cl_mem), (void *)&clust_data_cl->clusterLastIndexLeft);
  clStatus = clSetKernelArg(_kernel_findBoundary, 7, sizeof(cl_mem), (void *)&clust_data_cl->clusterLastIndexRight);
  clStatus = clSetKernelArg(_kernel_findBoundary, 8, sizeof(cl_mem), (void *)&clust_data_cl->trueCluster);

  size_t global_size = sst_data_cl->nSeedStripsNC;

  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_findBoundary, 1, NULL, &global_size, NULL, 0, NULL, NULL);
  checkCLStatus(clStatus);
}

void clustering::checkCondition(sst_data_cl_t *sst_data_cl, calib_data_cl_t* calib_data_cl, clust_data_cl_t* clust_data_cl) {
  cl_int clStatus;

  clStatus = clSetKernelArg(_kernel_checkCondition, 0, sizeof(int),  (void *)&sst_data_cl->nSeedStripsNC);
  clStatus = clSetKernelArg(_kernel_checkCondition, 1, sizeof(cl_mem), (void *)&sst_data_cl->stripId);
  clStatus = clSetKernelArg(_kernel_checkCondition, 2, sizeof(cl_mem), (void *)&sst_data_cl->adc);
  clStatus = clSetKernelArg(_kernel_checkCondition, 3, sizeof(cl_mem),  (void *)&calib_data_cl->gain);
  clStatus = clSetKernelArg(_kernel_checkCondition, 4, sizeof(cl_mem), (void *)&clust_data_cl->clusterLastIndexLeft);
  clStatus = clSetKernelArg(_kernel_checkCondition, 5, sizeof(cl_mem), (void *)&clust_data_cl->clusterLastIndexRight);
  clStatus = clSetKernelArg(_kernel_checkCondition, 6, sizeof(cl_mem), (void *)&clust_data_cl->trueCluster);
  clStatus = clSetKernelArg(_kernel_checkCondition, 7, sizeof(cl_mem), (void *)&clust_data_cl->clusterADCs);
  clStatus = clSetKernelArg(_kernel_checkCondition, 8, sizeof(cl_mem), (void *)&clust_data_cl->barycenter);

  size_t global_size = sst_data_cl->nSeedStripsNC;
  clStatus = clEnqueueNDRangeKernel(_context->clQueue, _kernel_checkCondition, 1, NULL, &global_size, NULL, 0, NULL, NULL);
}
