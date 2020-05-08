#ifndef __CLUSTERING_H__
#define __CLUSTERING_H__

#include "clppContext.h"
#include "clppProgram.h"
#include "cluster_cl.h"

class clustering : public clppProgram 
{
 public:

  clustering(clppContext* context);
  ~clustering() { };
  
  void findBoundary(sst_data_cl_t *sst_data_cl, calib_data_cl_t* calib_data_cl, clust_data_cl_t* clust_data_cl);

  void checkCondition(sst_data_cl_t *sst_data_cl, calib_data_cl_t* calib_data_cl, clust_data_cl_t* clust_data_cl);

 private:

  cl_kernel _kernel_findBoundary;
  cl_kernel _kernel_checkCondition;
};

#endif
