#ifndef _CLUSTER_CL_
#define _CLUSTER_CL_

#include <CL/cl.h>
#include "SiStripConditions.h"
#include "clppScan_Default.h"
#include "clppContext.h"
#include "cluster.h"

typedef struct {
  cl_mem detId;
  cl_mem stripId;
  cl_mem adc;
  cl_mem fedId;
  cl_mem fedCh;
  cl_mem seedStripsNCIndex;
  cl_mem seedStripsMask;
  cl_mem seedStripsNCMask;
  cl_mem prefixSeedStripsNCMask;
  //  clppScan* prefixScan;
  int nSeedStripsNC;
  int nStrips;
} sst_data_cl_t;

typedef struct {
  cl_mem noise;
  cl_mem gain;
  cl_mem bad;
} calib_data_cl_t;

typedef struct {
  cl_mem clusterLastIndexLeft;
  cl_mem clusterLastIndexRight;
  cl_mem clusterADCs;
  cl_mem trueCluster;
  cl_mem barycenter;
} clust_data_cl_t;

void allocateSSTData_cl(int max_strips, clppContext *context, sst_data_cl_t* sst_data_cl);
void allocateCalibData_cl(int max_strips, clppContext *context, calib_data_cl_t* calib_data_cl);
void allocateClustData_cl(int max_strips, clppContext *context, clust_data_cl_t* clust_data_cl);

void copyDataToFPGA(clppContext *context, sst_data_t* sst_data, sst_data_cl_t* sst_data_cl, calib_data_t* calib_data, calib_data_cl_t* calib_data_cl);
void setSeedStripsNCIndex(clppContext *context, sst_data_cl_t* sst_data_cl, sst_data_t* sst_data, calib_data_cl_t* calib_data_cl, clust_data_cl_t* clust_data_cl);
void findCluster(clppContext *context, sst_data_cl_t* sst_data_cl, calib_data_cl_t* calib_data_cl, clust_data_cl_t* clust_data_cl);
void copyDataBackCPU(clppContext *context, clust_data_t* clust_data, clust_data_cl_t* clust_data_cl);

void freeSSTData_cl(sst_data_cl_t* sst_data_cl);
void freeCalibData_cl(calib_data_cl_t* calib_data_cl);
void freeClustData_cl(clust_data_cl_t* clust_data_cl);

#endif
