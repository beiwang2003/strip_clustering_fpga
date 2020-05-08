#include "cluster_cl.h"
#include "clppScan_Default.h"
#include "clustering.h"
#include "findSeedStrips.h"

void allocateSSTData_cl(int max_strips, clppContext *context, sst_data_cl_t* sst_data_cl) {
  cl_int clStatus;

  sst_data_cl->detId = clCreateBuffer(context->clContext, CL_MEM_READ_ONLY, max_strips*sizeof(detId_t), NULL, &clStatus);
  sst_data_cl->stripId = clCreateBuffer(context->clContext, CL_MEM_READ_ONLY, max_strips*sizeof(uint16_t), NULL, &clStatus);
  sst_data_cl->adc = clCreateBuffer(context->clContext, CL_MEM_READ_ONLY, max_strips*sizeof(uint8_t), NULL, &clStatus);
  sst_data_cl->fedId = clCreateBuffer(context->clContext, CL_MEM_READ_ONLY, max_strips*sizeof(fedId_t), NULL, &clStatus);
  sst_data_cl->fedCh = clCreateBuffer(context->clContext, CL_MEM_READ_ONLY, max_strips*sizeof(fedCh_t), NULL, &clStatus);

  sst_data_cl->seedStripsMask = clCreateBuffer(context->clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, max_strips*sizeof(int), NULL, &clStatus);
  sst_data_cl->seedStripsNCMask = clCreateBuffer(context->clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, max_strips*sizeof(int), NULL, &clStatus);
  sst_data_cl->prefixSeedStripsNCMask = clCreateBuffer(context->clContext, CL_MEM_READ_WRITE,  max_strips*sizeof(int), NULL, &clStatus);
  sst_data_cl->seedStripsNCIndex = clCreateBuffer(context->clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, max_strips*sizeof(int), NULL, &clStatus);
}

void allocateCalibData_cl(int max_strips, clppContext *context, calib_data_cl_t* calib_data_cl) {
  cl_int clStatus;

  calib_data_cl->noise = clCreateBuffer(context->clContext, CL_MEM_READ_ONLY, max_strips*sizeof(float), NULL, &clStatus);
  calib_data_cl->gain = clCreateBuffer(context->clContext, CL_MEM_READ_ONLY, max_strips*sizeof(float), NULL, &clStatus);
  calib_data_cl->bad = clCreateBuffer(context->clContext, CL_MEM_READ_ONLY, max_strips*sizeof(bool), NULL, &clStatus);
}

void allocateClustData_cl(int max_seedstrips, clppContext *context, clust_data_cl_t* clust_data_cl) {
  cl_int clStatus;
  
  clust_data_cl->clusterLastIndexLeft = clCreateBuffer(context->clContext, CL_MEM_WRITE_ONLY, max_seedstrips*sizeof(int), NULL, &clStatus);
  clust_data_cl->clusterLastIndexRight = clCreateBuffer(context->clContext, CL_MEM_WRITE_ONLY, max_seedstrips*sizeof(int), NULL, &clStatus);
  clust_data_cl->clusterADCs = clCreateBuffer(context->clContext, CL_MEM_WRITE_ONLY, max_seedstrips*256*sizeof(uint8_t), NULL, &clStatus);
  clust_data_cl->trueCluster = clCreateBuffer(context->clContext, CL_MEM_WRITE_ONLY, max_seedstrips*sizeof(bool), NULL, &clStatus);
  clust_data_cl->barycenter = clCreateBuffer(context->clContext, CL_MEM_WRITE_ONLY, max_seedstrips*sizeof(float), NULL, &clStatus);
}

void freeSSTData_cl(sst_data_cl_t *sst_data_cl) {
  cl_int clStatus;

  clStatus = clReleaseMemObject(sst_data_cl->detId);
  clStatus = clReleaseMemObject(sst_data_cl->stripId);
  clStatus = clReleaseMemObject(sst_data_cl->adc);
  clStatus = clReleaseMemObject(sst_data_cl->fedId);
  clStatus = clReleaseMemObject(sst_data_cl->fedCh);
  clStatus = clReleaseMemObject(sst_data_cl->seedStripsMask);
  clStatus = clReleaseMemObject(sst_data_cl->seedStripsNCMask);
  clStatus = clReleaseMemObject(sst_data_cl->prefixSeedStripsNCMask);
  clStatus = clReleaseMemObject(sst_data_cl->seedStripsNCIndex);
}

void freeCalibData_cl(calib_data_cl_t *calib_data_cl) {
  cl_int clStatus;

  clStatus = clReleaseMemObject(calib_data_cl->noise);
  clStatus = clReleaseMemObject(calib_data_cl->gain);
  clStatus = clReleaseMemObject(calib_data_cl->bad);
}

void freeClustData_cl(clust_data_cl_t *clust_data_cl) {
  cl_int clStatus;

  clStatus = clReleaseMemObject(clust_data_cl->clusterLastIndexLeft);
  clStatus = clReleaseMemObject(clust_data_cl->clusterLastIndexRight);
  clStatus = clReleaseMemObject(clust_data_cl->clusterADCs);
  clStatus = clReleaseMemObject(clust_data_cl->trueCluster);
  clStatus = clReleaseMemObject(clust_data_cl->barycenter);

}

void copyDataToFPGA(clppContext *context, sst_data_t* sst_data, sst_data_cl_t* sst_data_cl, calib_data_t* calib_data, calib_data_cl_t* calib_data_cl) {
  int nStrips = sst_data->nStrips;
  sst_data_cl->nStrips = nStrips;
  sst_data_cl->nSeedStripsNC = sst_data->nSeedStripsNC;

  cl_int clStatus;

  clStatus = clEnqueueWriteBuffer(context->clQueue, sst_data_cl->detId, CL_TRUE, 0, nStrips*sizeof(detId_t), sst_data->detId, 0, NULL, NULL);
  clStatus = clEnqueueWriteBuffer(context->clQueue, sst_data_cl->stripId, CL_TRUE, 0, nStrips*sizeof(uint16_t), sst_data->stripId, 0, NULL, NULL);
  clStatus = clEnqueueWriteBuffer(context->clQueue, sst_data_cl->adc, CL_TRUE, 0, nStrips*sizeof(uint8_t), sst_data->adc, 0, NULL, NULL);
  clStatus = clEnqueueWriteBuffer(context->clQueue, sst_data_cl->fedId, CL_TRUE, 0, nStrips*sizeof(fedId_t), sst_data->fedId, 0, NULL, NULL);
  clStatus = clEnqueueWriteBuffer(context->clQueue, sst_data_cl->fedCh, CL_TRUE, 0, nStrips*sizeof(fedCh_t), sst_data->fedCh, 0, NULL, NULL);

  clStatus = clEnqueueWriteBuffer(context->clQueue, calib_data_cl->noise, CL_TRUE, 0, nStrips*sizeof(float), calib_data->noise, 0, NULL, NULL);
  clStatus = clEnqueueWriteBuffer(context->clQueue, calib_data_cl->gain, CL_TRUE, 0, nStrips*sizeof(float), calib_data->gain, 0, NULL, NULL);
  clStatus = clEnqueueWriteBuffer(context->clQueue, calib_data_cl->bad, CL_TRUE, 0, nStrips*sizeof(bool), calib_data->bad, 0, NULL, NULL);
}

void setSeedStripsNCIndex(clppContext *context, sst_data_cl_t* sst_data_cl, sst_data_t* sst_data, calib_data_cl_t* calib_data_cl, clust_data_cl_t* clust_data_cl) {
  cl_int clStatus;

  auto setSeedIndex_ptr = std::make_unique<findSeedStrips>(context);
  setSeedIndex_ptr->setSeedStrips(sst_data_cl, calib_data_cl);
  setSeedIndex_ptr->setNCSeedStrips(sst_data_cl);

  clStatus = clEnqueueCopyBuffer(context->clQueue, sst_data_cl->seedStripsNCMask, sst_data_cl->prefixSeedStripsNCMask, 0, 0, sst_data_cl->nStrips*sizeof(int), 0, NULL, NULL);
  auto prefixScan_ptr = std::make_unique<clppScan_Default>(context, sizeof(int), sst_data_cl->nStrips);
  prefixScan_ptr->pushDatas(sst_data_cl->prefixSeedStripsNCMask, sst_data_cl->nStrips);
  prefixScan_ptr->scan();
  prefixScan_ptr->waitCompletion();

  setSeedIndex_ptr->setStripIndex(sst_data_cl);
  clStatus = clEnqueueReadBuffer(context->clQueue, sst_data_cl->prefixSeedStripsNCMask, CL_TRUE, (sst_data_cl->nStrips-1)*sizeof(int), sizeof(int), &sst_data_cl->nSeedStripsNC, 0, NULL, NULL); 
  printf("nSeedStripsNC=%d\n", sst_data_cl->nSeedStripsNC); 
  setSeedIndex_ptr->waitCompletion();

}

void findCluster(clppContext *context, sst_data_cl_t* sst_data_cl, calib_data_cl_t* calib_data_cl, clust_data_cl_t* clust_data_cl) {

  auto clusters_ptr = std::make_unique<clustering>(context);
  clusters_ptr->findBoundary(sst_data_cl, calib_data_cl, clust_data_cl);
  clusters_ptr->checkCondition(sst_data_cl, calib_data_cl, clust_data_cl);
  clusters_ptr->waitCompletion();

}

void copyDataBackCPU(clppContext *context, clust_data_t* clust_data, clust_data_cl_t* clust_data_cl){

  int nSeedStripsNC=150000;
  
  cl_int clStatus;
  clStatus = clEnqueueReadBuffer(context->clQueue, clust_data_cl->clusterLastIndexLeft, CL_TRUE, 0, nSeedStripsNC*sizeof(int), clust_data->clusterLastIndexLeft, 0, NULL, NULL);
  clStatus = clEnqueueReadBuffer(context->clQueue, clust_data_cl->clusterLastIndexRight, CL_TRUE, 0, nSeedStripsNC*sizeof(int), clust_data->clusterLastIndexRight, 0, NULL, NULL);
  clStatus = clEnqueueReadBuffer(context->clQueue, clust_data_cl->clusterADCs, CL_TRUE, 0, nSeedStripsNC*256*sizeof(uint8_t), clust_data->clusterADCs, 0, NULL, NULL);
  clStatus = clEnqueueReadBuffer(context->clQueue, clust_data_cl->trueCluster, CL_TRUE, 0, nSeedStripsNC*sizeof(bool), clust_data->trueCluster, 0, NULL, NULL);
  clStatus = clEnqueueReadBuffer(context->clQueue, clust_data_cl->barycenter, CL_TRUE, 0, nSeedStripsNC*sizeof(float), clust_data->barycenter, 0, NULL, NULL);

}
