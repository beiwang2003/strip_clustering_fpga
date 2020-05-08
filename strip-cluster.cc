#include <fstream>
#include <iostream>
#include <cstring>
#include <memory>
#include <functional>
#include "cluster.h"
#ifdef USE_FPGA
#include <CL/cl.h>
#include "cluster_cl.h"
//#include "opencl.h"
#endif 

int main()
{
  const int max_strips = MAX_STRIPS;
  const int max_seedstrips = MAX_SEEDSTRIPS;
  auto sst_data_ptr = std::make_unique<sst_data_t>();
  auto clust_data_ptr = std::make_unique<clust_data_t>();
  auto calib_data_ptr = std::make_unique<calib_data_t>();

  allocateSSTData(max_strips, sst_data_ptr.get());
  allocateClustData(max_strips, clust_data_ptr.get());
  allocateCalibData(max_strips, calib_data_ptr.get());

#ifdef USE_FPGA
  clppContext context;

  auto sst_data_cl_ptr = std::make_unique<sst_data_cl_t>();
  auto clust_data_cl_ptr = std::make_unique<clust_data_cl_t>();
  auto calib_data_cl_ptr = std::make_unique<calib_data_cl_t>();

  allocateSSTData_cl(max_strips, &context, sst_data_cl_ptr.get());
  allocateCalibData_cl(max_strips, &context, calib_data_cl_ptr.get());
  allocateClustData_cl(max_seedstrips, &context, clust_data_cl_ptr.get());
#endif

  // read in calibration data
  std::string condfilename("stripcond.bin");
  auto conditions = std::make_unique<SiStripConditions>(condfilename);

  std::vector<FEDRawData> fedRawData;
  std::vector<FEDBuffer> fedBuffer;
  std::vector<fedId_t> fedIndex;
  FEDReadoutMode mode;

  std::string datafilename("stripdata.bin");
  readinRawData(datafilename, conditions.get(), fedRawData, fedBuffer, fedIndex, mode, sst_data_ptr.get());
  unpackRawData(conditions.get(), fedRawData, fedBuffer, fedIndex, sst_data_ptr.get(), calib_data_ptr.get(), mode);

#ifdef USE_FPGA

  copyDataToFPGA(&context, sst_data_ptr.get(), sst_data_cl_ptr.get(), calib_data_ptr.get(), calib_data_cl_ptr.get());
  
  setSeedStripsNCIndex(&context, sst_data_cl_ptr.get(), sst_data_ptr.get(), calib_data_cl_ptr.get(), clust_data_cl_ptr.get());
  sst_data_ptr.get()->nSeedStripsNC = sst_data_cl_ptr.get()->nSeedStripsNC;
  
  findCluster(&context, sst_data_cl_ptr.get(), calib_data_cl_ptr.get(), clust_data_cl_ptr.get());

  copyDataBackCPU(&context, clust_data_ptr.get(), clust_data_cl_ptr.get());

#else

  setSeedStripsNCIndex(sst_data_ptr.get(), calib_data_ptr.get(), conditions.get());  

  findCluster(sst_data_ptr.get(), calib_data_ptr.get(), conditions.get(), clust_data_ptr.get());

#endif

  // print out the result                                                                                                             
#ifdef OUTPUT
  std::cout<<" nSeedStripsNC "<<sst_data_ptr.get()->nSeedStripsNC<<std::endl;
  for (int j=0; j<sst_data_ptr.get()->nSeedStripsNC; j++) {
    if (clust_data_ptr.get()->trueCluster[j]){
      int index = clust_data_ptr.get()->clusterLastIndexLeft[j];
        //      std::cout<<" det id "<<sst_data[i]->detId[index]<<" strip "<<sst_data[i]->stripId[index]                              
      std::cout<<" bary center "<<clust_data_ptr.get()->barycenter[j]<<": ";
      int right=clust_data_ptr.get()->clusterLastIndexRight[j];
      int size=right-index+1;
      for (int k=0; k<size; k++){
	std::cout<<(unsigned int)clust_data_ptr.get()->clusterADCs[k*sst_data_ptr.get()->nSeedStripsNC+j]<<" ";
      }
      std::cout<<std::endl;
    }
  }
#endif

  freeSSTData(sst_data_ptr.get());   
  freeClustData(clust_data_ptr.get());
  freeCalibData(calib_data_ptr.get());

#ifdef USE_FPGA
  freeSSTData_cl(sst_data_cl_ptr.get());
  freeClustData_cl(clust_data_cl_ptr.get());
  freeCalibData_cl(calib_data_cl_ptr.get());
#endif

  return 0;

}
