#ifndef __FINDSEEDSTRIPS_H__
#define __FINDSEEDSTRIPS_H__

#include "clppContext.h"
#include "clppProgram.h"
#include "cluster_cl.h"

class findSeedStrips : public clppProgram
{
  public: 

  findSeedStrips(clppContext* context);

  ~findSeedStrips() { };

  void setSeedStrips(sst_data_cl_t *sst_data_cl, calib_data_cl_t* calib_data_cl);

  void setNCSeedStrips(sst_data_cl_t *sst_data_cl);

  void setStripIndex(sst_data_cl_t *sst_data_cl);

 private:
  
  cl_kernel _kernel_setSeedStrips;
  cl_kernel _kernel_setNCSeedStrips;
  cl_kernel _kernel_setStripIndex;

};

#endif 
