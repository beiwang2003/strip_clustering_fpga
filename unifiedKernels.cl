#include "clppScan_Default.cl"
#define M 128
#define K 6
#define CLUSTER_SIZE 16

typedef uchar uint8_t;
typedef ushort uint16_t;
typedef uint uint32_t;


__kernel void setStripMask(__global uint16_t* restrict stripId, __global uint32_t* restrict detId,  __global uint8_t* restrict adc, __global float * restrict noise, __global int* restrict seedStripsNCMask, uint16_t invStrip, int N) {
  const float SeedThreshold = 3.0;

  bool mask_left=0;
  uint16_t stripId_left;
  uint32_t detId_left;
  const int len = N/M;

  for (unsigned int i=0; i<=len; i++) {
    bool mask_loc[M];
    bool mask2_loc[M];
    uint16_t stripId_loc[M];
    uint32_t detId_loc[M];

    for (unsigned int j=0; j<M; j++) { 
       mask_loc[j] = 0;
       int index = i*M+j;
       if (index<N) {
         stripId_loc[j] = stripId[index];
         detId_loc[j] = detId[index];
       }
       else {
         stripId_loc[j] = invStrip;
       }

       if (stripId_loc[j] != invStrip) {
         float noise_j = noise[index];
         uint8_t adc_j = adc[index];
         mask_loc[j] = (adc_j >= (uint8_t)( noise_j * SeedThreshold)) ? 1:0;
       }
       mask2_loc[j] = mask_loc[j];
    }

    if (mask_loc[0]&&mask_left&&(stripId_loc[0]-stripId_left==1)&&(detId_loc[0]==detId_left)) mask2_loc[0] = 0;

    for (unsigned int j=1; j<M; j++) {
      if (mask_loc[j]&&mask_loc[j-1]&&(stripId_loc[j]-stripId_loc[j-1]==1)&&(detId_loc[j]==detId_loc[j-1])) mask2_loc[j] = 0;
    }

    for (unsigned int j=0; j<M; j++) {
      if ((i*M+j)<N)
        seedStripsNCMask[i*M+j] = mask2_loc[j];
    }

    mask_left = mask_loc[M-1];
    detId_left = detId_loc[M-1];
    stripId_left = stripId_loc[M-1];
   
  }

}


__kernel void setSeedStrips(__global uint16_t* restrict stripId, __global int* restrict seedStripsMask, __global int* restrict seedStripsNCMask, __global uint8_t* restrict adc, __global float * restrict noise, uint16_t invStrip, int N) {

  const float SeedThreshold = 3.0;

#ifdef NDRANGE
  const unsigned i = get_global_id(0);
#else
  for (unsigned i=0; i<N; i++) {
#endif
  seedStripsMask[i] = 0;
  seedStripsNCMask[i] = 0;
  uint16_t strip = stripId[i];
  if (strip != invStrip) {
    float noise_i = noise[i];
    uint8_t adc_i = adc[i];
    seedStripsMask[i] = (adc_i >= (uint8_t)( noise_i * SeedThreshold)) ? 1:0;
    seedStripsNCMask[i] = seedStripsMask[i];
  }
#ifndef NDRANGE
  }
#endif
}

__kernel void setNCSeedStrips(__global uint32_t* restrict detId, __global uint16_t* restrict stripId, __global int* restrict seedStripsMask, __global int* restrict seedStripsNCMask, int N) { 

#ifdef NDRANGE
  const unsigned i = get_global_id(0);
#else
  for (unsigned i=0; i<N; i++) {
#endif
  if (i>0&&seedStripsMask[i]&&seedStripsMask[i-1]&&(stripId[i]-stripId[i-1])==1&&(detId[i]==detId[i-1])) seedStripsNCMask[i] = 0;
#ifndef NDRANGE
  }
#endif
}


__kernel void setStripIndex(__global int* restrict seedStripsNCMask, __global int* restrict prefixSeedStripsNCMask, __global int* restrict seedStripsNCIndex, int N) {
	 
#ifdef NDRANGE
  const unsigned i = get_global_id(0);
#else
  for (unsigned i=0; i<N; i++) {
#endif
  if (seedStripsNCMask[i] == 1) {
    int index = prefixSeedStripsNCMask[i];
    seedStripsNCIndex[index] = i;
  }
#ifndef NDRANGE
  }
#endif
}

__kernel void formClusters_opt(int nStrips, int nSeedStripsNC, __global uint32_t* restrict detId, __global uint16_t* restrict stripId, __global uint8_t* restrict adc, __global int* restrict seedStripsNCIndex, __global float* restrict gain, __global float* restrict noise, __global int* restrict clusterLastIndexLeft, __global int* restrict clusterLastIndexRight, __global bool* restrict trueCluster,  __global uint8_t* restrict clusterADCs, __global float* restrict barycenter, int N) {

   const uint8_t MaxSequentialHoles = 0;
   const float  ChannelThreshold = 2.0;
   const float ClusterThresholdSquared = 25.0;

   const float minGoodCharge = 1620.0;
   const uint16_t stripIndexMask = 0x7FFF;

#ifdef NDRANGE
   const unsigned i = get_global_id(0);
#else
   for (unsigned i=0; i<N; i++) {
#endif
   int index=seedStripsNCIndex[i];
   int indexLeft = index;
   int indexRight = index;
   float noise_i = noise[index];
   float gain_i = gain[index];
   uint8_t adc_i = adc[index];
   uint32_t detId_i = detId[index];

   float noiseSquared_i = noise_i*noise_i;
   float adcSum_i = (float)adc_i;
   
   int charge_i = (int)( (float)adc_i/gain_i + 0.5f );
   if (adc_i < 254) adc_i = ( charge_i > 1022 ? 255 : (charge_i > 253 ? 254 : charge_i));
   float adcSum2_i = (float)adc_i;
   
   uint16_t stripId_loc[2*CLUSTER_SIZE-1];
   uint32_t detId_loc[2*CLUSTER_SIZE-1];
   float noise_loc[2*CLUSTER_SIZE-1];
   float gain_loc[2*CLUSTER_SIZE-1];
   uint8_t adc_loc[2*CLUSTER_SIZE-1];
   float adc2_loc[2*CLUSTER_SIZE-1];

   for (int j=0; j<2*CLUSTER_SIZE-1; j++) {
     int id = index-CLUSTER_SIZE+1;
     if ((id+j)>=0&&(id+j)<nStrips) {
       stripId_loc[j] = stripId[id+j];
       detId_loc[j] = detId[id+j];
       noise_loc[j] = noise[id+j];
       gain_loc[j] = gain[id+j];
       adc_loc[j] = adc[id+j];
     }
   }
   adc2_loc[CLUSTER_SIZE-1] = (float)adc_i;

   float noiseSquared_loc[K];
   float adcSum_loc[K];
   float adcSum2_loc[K];
   for (int k=0; k<K; k++) {
     noiseSquared_loc[k] = 0.0;
     adcSum_loc[k] = 0.0;
     adcSum2_loc[k] = 0.0;
   }

   // find left boundary
   uint16_t stripId_left = stripId_loc[CLUSTER_SIZE-1];
   for (int j=0; j<CLUSTER_SIZE-1; j++) {
     int testIndexLeft = index-1-j;
     int id = CLUSTER_SIZE-2-j;
     if (testIndexLeft>=0) {
       int rangeLeft = stripId_left-stripId_loc[id]-1;
       bool sameDetLeft = detId_i == detId_loc[id];
       if (rangeLeft>=0&&rangeLeft<=MaxSequentialHoles&&sameDetLeft) {

         float testNoise = noise_loc[id];
	 float testGain = gain_loc[id];
         uint8_t testADC = adc_loc[id];

         if (testADC >= (uint8_t)(testNoise * ChannelThreshold)) {
           --indexLeft;
           stripId_left = stripId_loc[id];
           //noiseSquared_i += testNoise*testNoise;
           //adcSum_i += (float)testADC;

           noiseSquared_loc[K-1] = noiseSquared_loc[0] + testNoise*testNoise;
           adcSum_loc[K-1] = adcSum_loc[0] + (float)testADC;

	   int charge = (int)( (float)testADC/testGain + 0.5f );
           if (testADC < 254) testADC = ( charge > 1022 ? 255 : (charge > 253 ? 254 : charge));
	   adcSum2_loc[K-1] = adcSum2_loc[0] + (float)testADC;
	   adc2_loc[id] = (float)testADC;

           #pragma unroll
           for (unsigned k=0; k<K-1; k++) {
             noiseSquared_loc[k] = noiseSquared_loc[k+1];
             adcSum_loc[k] = adcSum_loc[k+1];
	     adcSum2_loc[k] = adcSum2_loc[k+1];
           }
         }
       }
       else
         break;
     }
   }

   #pragma unroll
   for (int k=0; k<K-1; k++) {
     noiseSquared_i += noiseSquared_loc[k];
     adcSum_i += adcSum_loc[k];
     adcSum2_i += adcSum2_loc[k];

     noiseSquared_loc[k] = 0.0;
     adcSum_loc[k] = 0.0;
     adcSum2_loc[k] = 0.0;
   }

   // find right boundary
   uint16_t stripId_right = stripId_loc[CLUSTER_SIZE-1];
   for (int j=0; j<CLUSTER_SIZE-1; j++) {
     int testIndexRight = index+1+j;
     int id=CLUSTER_SIZE+j;
     if (testIndexRight<nStrips) {
       int rangeRight = stripId_loc[id]-stripId_right-1;
       bool sameDetRight = detId_i == detId_loc[id];
       if (rangeRight>=0&&rangeRight<=MaxSequentialHoles&&sameDetRight) {

         float testNoise = noise_loc[id];
	 float testGain = gain_loc[id];
         uint8_t testADC = adc_loc[id];

         if (testADC >= (uint8_t)(testNoise * ChannelThreshold)) {
           ++indexRight;
           stripId_right = stripId_loc[id];
           //noiseSquared_i += testNoise*testNoise;
           //adcSum_i += (float)testADC;
           noiseSquared_loc[K-1] = noiseSquared_loc[0] + testNoise*testNoise;
           adcSum_loc[K-1] = adcSum_loc[0] + (float)testADC;

           int charge = (int)( (float)testADC/testGain + 0.5f );
           if (testADC < 254) testADC = ( charge > 1022 ? 255 : (charge > 253 ? 254 : charge));
	   adcSum2_loc[K-1] = adcSum2_loc[0] + (float)testADC;
	   adc2_loc[id] = (float)testADC;

           #pragma unroll
           for (unsigned k=0; k<K-1; k++) {
             noiseSquared_loc[k] = noiseSquared_loc[k+1];
             adcSum_loc[k] = adcSum_loc[k+1];
	     adcSum2_loc[k] = adcSum2_loc[k+1];
           }
         }
       }
       else
         break;
     }
   }

   #pragma unroll
   for (int k=0; k<K-1; k++) {
     noiseSquared_i += noiseSquared_loc[k];
     adcSum_i += adcSum_loc[k];
     adcSum2_i += adcSum2_loc[k];
   }

   bool noiseSquaredPass = noiseSquared_i*ClusterThresholdSquared <= adcSum_i*adcSum_i;
   //trueCluster[i] = noiseSquaredPass;
   clusterLastIndexLeft[i] = indexLeft;
   clusterLastIndexRight[i] = indexRight;
   bool trueClust = noiseSquaredPass;

   if (trueClust) {
     int size=indexRight-indexLeft+1;
     int offset=CLUSTER_SIZE-1-(index-indexLeft);
     int sumx=0;
     int suma=0;
     for (int j=0; j<size; j++) {
       uint8_t adc_j = adc2_loc[offset+j];
       sumx += j*adc_j;
       suma += adc_j;
       clusterADCs[j*nSeedStripsNC+i] = adc_j;
     }
     barycenter[i] = (float)(stripId_left & stripIndexMask) + (float)sumx/(float)suma + 0.5f;

     if (i>0&&clusterLastIndexLeft[i-1]==indexLeft) {
       trueClust = 0;  // ignore duplicates
     }
     trueClust =  (adcSum2_i/0.047f) > minGoodCharge;
   }

   trueCluster[i] = trueClust;

#ifndef NDRANGE
   }
#endif

}

__kernel void formClusters(int nStrips, int nSeedStripsNC, __global uint32_t* restrict detId, __global uint16_t* restrict stripId, __global uint8_t* restrict adc, __global int* restrict seedStripsNCIndex, __global float* restrict gain, __global float* restrict noise, __global int* restrict clusterLastIndexLeft, __global int* restrict clusterLastIndexRight, __global bool* restrict trueCluster,  __global uint8_t* restrict clusterADCs, __global float* restrict barycenter, int N) {

   const uint8_t MaxSequentialHoles = 0;
   const float  ChannelThreshold = 2.0;
   const float ClusterThresholdSquared = 25.0;

   const float minGoodCharge = 1620.0;
   const uint16_t stripIndexMask = 0x7FFF;

#ifdef NDRANGE
   const unsigned i = get_global_id(0);
#else
   for (unsigned i=0; i<N; i++) {
#endif
   int index=seedStripsNCIndex[i];
   int indexLeft = index;
   int indexRight = index;
   float noise_i = noise[index];
   uint8_t adc_i = adc[index];
   float gain_i = gain[index];
   float adcSum_i = (float)adc_i;

   int charge = (int)( (float)adc_i/gain_i + 0.5f );
   if (adc_i < 254) adc_i = ( charge > 1022 ? 255 : (charge > 253 ? 254 : charge));
   float noiseSquared_i = noise_i*noise_i;
  
   int offset = CLUSTER_SIZE-index;
   uint8_t adc_loc[2*CLUSTER_SIZE+1];
   for (int j=0; j<2*CLUSTER_SIZE+1; j++) adc_loc[j] = 0;
   adc_loc[index+offset] = adc_i;
   float adcSum_i2 = (float)adc_i;

   // find left boundary
   int testIndexLeft=index-1;
   for (;index-testIndexLeft<CLUSTER_SIZE; testIndexLeft--) {
     if (testIndexLeft>=0) {
       int rangeLeft = stripId[indexLeft]-stripId[testIndexLeft]-1;
       bool sameDetLeft = detId[index] == detId[testIndexLeft];
       if (rangeLeft>=0&&rangeLeft<=MaxSequentialHoles&&sameDetLeft) {

         float testNoise = noise[testIndexLeft];
         uint8_t testADC = adc[testIndexLeft];
	 float testGain = gain[testIndexLeft];

         if (testADC >= (uint8_t)(testNoise * ChannelThreshold)) {
           --indexLeft;
           noiseSquared_i += testNoise*testNoise;
           adcSum_i += (float)testADC;

	   int charge = (int)( (float)testADC/testGain + 0.5f );
	   if (testADC < 254) testADC = ( charge > 1022 ? 255 : (charge > 253 ? 254 : charge));
	   adc_loc[indexLeft+offset] = testADC;
	   adcSum_i2 += (float)testADC;
         }
       }
       else
         break;
     }
   }

   // find right boundary
   int testIndexRight=index+1;
   for (; testIndexRight-index<CLUSTER_SIZE; testIndexRight++) {
     if (testIndexRight<nStrips) {
       int rangeRight = stripId[testIndexRight]-stripId[indexRight]-1;
       bool sameDetRight = detId[index] == detId[testIndexRight];
       if (rangeRight>=0&&rangeRight<=MaxSequentialHoles&&sameDetRight) {

         float testNoise = noise[testIndexRight];
         uint8_t testADC = adc[testIndexRight];
         float testGain = gain[testIndexRight];

         if (testADC >= (uint8_t)(testNoise * ChannelThreshold)) {
           ++indexRight;
           noiseSquared_i += testNoise*testNoise;
           adcSum_i += (float)testADC;

           int charge =	(int)( (float)testADC/testGain + 0.5f );
           if (testADC < 254) testADC = ( charge > 1022 ? 255 : (charge > 253 ? 254 : charge));
           adc_loc[indexRight+offset] = testADC;
           adcSum_i2 +=(float)testADC;	
         }
       }
       else
         break;
     }
   }

   bool noiseSquaredPass = noiseSquared_i*ClusterThresholdSquared <= adcSum_i*adcSum_i;
   //trueCluster[i] = noiseSquaredPass;
   clusterLastIndexLeft[i] = indexLeft;
   clusterLastIndexRight[i] = indexRight;
   bool trueClust = noiseSquaredPass;

   if (trueClust) {
     int size=indexRight-indexLeft+1;
     int sumx=0;
     int suma=0;
     for (int j=0; j<size; j++) {
       uint8_t adc_j = adc_loc[offset+indexLeft+j];
       sumx += j*adc_j;
       suma += adc_j;
       clusterADCs[j*nSeedStripsNC+i] = adc_j;
     } 
     barycenter[i] = (float)(stripId[indexLeft] & stripIndexMask) + (float)sumx/(float)suma + 0.5f;
   
     if (i>0&&clusterLastIndexLeft[i-1]==indexLeft) {
       trueClust = 0;  // ignore duplicates
     }
     trueClust =  (adcSum_i2/0.047f) > minGoodCharge;
   }

   trueCluster[i] = trueClust;

#ifndef NDRANGE
   }
#endif

}

__kernel void findLeftRightBoundary(int nStrips, __global uint32_t* restrict detId, __global uint16_t* restrict stripId, __global uint8_t* restrict adc, __global int* restrict seedStripsNCIndex, __global float* restrict noise, __global int* restrict clusterLastIndexLeft, __global int* restrict clusterLastIndexRight, __global bool* restrict trueCluster, int N) {

   const uint8_t MaxSequentialHoles = 0;
   const float  ChannelThreshold = 2.0;
   const float ClusterThresholdSquared = 25.0;

   const float minGoodCharge = 1620.0;
   const uint16_t stripIndexMask = 0x7FFF;

#ifdef NDRANGE
   const unsigned i = get_global_id(0);
#else
   for (unsigned i=0; i<N; i++) {
#endif
   int index=seedStripsNCIndex[i];
   int indexLeft = index;
   int indexRight = index;
   float noise_i = noise[index];

   float noiseSquared_i = noise_i*noise_i;
   float adcSum_i = (float)adc[index];
   uint32_t detId_i = detId[index];

   uint16_t stripId_loc[2*CLUSTER_SIZE-1];
   uint32_t detId_loc[2*CLUSTER_SIZE-1];
   float noise_loc[2*CLUSTER_SIZE-1];
   uint8_t adc_loc[2*CLUSTER_SIZE-1];

   for (int j=0; j<2*CLUSTER_SIZE-1; j++) {
     int id = index-CLUSTER_SIZE+1;
     //if ((id+j)>=0&&(id+j)<nStrips) {
       stripId_loc[j] = stripId[id+j];
       detId_loc[j] = detId[id+j];
       noise_loc[j] = noise[id+j];
       adc_loc[j] = adc[id+j];
     //}
   }
   
   /*
   // find left boundary
   int testIndexLeft=index-1;
   if (testIndexLeft>=0) {
     int rangeLeft = stripId[indexLeft]-stripId[testIndexLeft]-1;
     bool sameDetLeft = detId[index] == detId[testIndexLeft];
     while(sameDetLeft&&testIndexLeft>=0&&rangeLeft>=0&&rangeLeft<=MaxSequentialHoles) {
       float testNoise = noise[testIndexLeft];
       uint8_t testADC = adc[testIndexLeft];

       if (testADC >= (uint8_t)(testNoise * ChannelThreshold)) {
         --indexLeft;
         noiseSquared_i += testNoise*testNoise;
         adcSum_i += (float)testADC;
       }
       --testIndexLeft;
       if (testIndexLeft>=0) {
         rangeLeft = stripId[indexLeft]-stripId[testIndexLeft]-1;
         sameDetLeft = detId[index] == detId[testIndexLeft];
       }
     }
   }
   */

   float noiseSquared_loc[K];
   float adcSum_loc[K];
   for (int k=0; k<K; k++) {
     noiseSquared_loc[k] = 0.0;
     adcSum_loc[k] = 0.0;
   }

   // find left boundary
   uint16_t stripId_left = stripId_loc[CLUSTER_SIZE-1];
   for (int j=0; j<CLUSTER_SIZE-1; j++) {
     int testIndexLeft = index-1-j;
     int id = CLUSTER_SIZE-2-j;
     if (testIndexLeft>=0) {
       int rangeLeft = stripId_left-stripId_loc[id]-1;
       bool sameDetLeft = detId_i == detId_loc[id];
       if (rangeLeft>=0&&rangeLeft<=MaxSequentialHoles&&sameDetLeft) {

         float testNoise = noise_loc[id];
         uint8_t testADC = adc_loc[id];
       
         if (testADC >= (uint8_t)(testNoise * ChannelThreshold)) {
           --indexLeft;
           stripId_left = stripId_loc[id];
           //noiseSquared_i += testNoise*testNoise;
           //adcSum_i += (float)testADC;

           noiseSquared_loc[K-1] = noiseSquared_loc[0] + testNoise*testNoise;
           adcSum_loc[K-1] = adcSum_loc[0] + (float)testADC;

           #pragma unroll
           for (unsigned k=0; k<K-1; k++) {
             noiseSquared_loc[k] = noiseSquared_loc[k+1];
             adcSum_loc[k] = adcSum_loc[k+1];
           }
         } 
       } 
       else
         break;
     }
   }

   #pragma unroll
   for (int k=0; k<K-1; k++) {
     noiseSquared_i += noiseSquared_loc[k];
     adcSum_i += adcSum_loc[k];
     
     noiseSquared_loc[k] = 0.0;
     adcSum_loc[k] = 0.0;
     
   }

   // find right boundary
   uint16_t stripId_right = stripId_loc[CLUSTER_SIZE-1];
   for (int j=0; j<CLUSTER_SIZE-1; j++) {
     int testIndexRight = index+1+j;
     int id=CLUSTER_SIZE+j;
     if (testIndexRight<nStrips) {
       int rangeRight = stripId_loc[id]-stripId_right-1;
       bool sameDetRight = detId_i == detId_loc[id];
       if (rangeRight>=0&&rangeRight<=MaxSequentialHoles&&sameDetRight) {

         float testNoise = noise_loc[id];
         uint8_t testADC = adc_loc[id];
	 
         if (testADC >= (uint8_t)(testNoise * ChannelThreshold)) {
           ++indexRight;
	   stripId_right = stripId_loc[id];
           //noiseSquared_i += testNoise*testNoise;
           //adcSum_i += (float)testADC;
           noiseSquared_loc[K-1] = noiseSquared_loc[0] + testNoise*testNoise;
           adcSum_loc[K-1] = adcSum_loc[0] + (float)testADC;

           #pragma unroll
           for (unsigned k=0; k<K-1; k++) {
             noiseSquared_loc[k] = noiseSquared_loc[k+1];
             adcSum_loc[k] = adcSum_loc[k+1];
           }
         }
       }
       else
         break;
     }
   }

   #pragma unroll
   for (int k=0; k<K-1; k++) {
     noiseSquared_i += noiseSquared_loc[k];
     adcSum_i += adcSum_loc[k];
   }

   /*
   // find right boundary
   int testIndexRight=index+1;
   if (testIndexRight<nStrips) {
     int rangeRight = stripId[testIndexRight]-stripId[indexRight]-1;
     bool sameDetRight = detId[index] == detId[testIndexRight];
     while(sameDetRight&&testIndexRight<nStrips&&rangeRight>=0&&rangeRight<=MaxSequentialHoles) {
       float testNoise = noise[testIndexRight];
       uint8_t testADC = adc[testIndexRight];
       if (testADC >= (uint8_t)(testNoise * ChannelThreshold)) {
         ++indexRight;
         noiseSquared_i += testNoise*testNoise;
         adcSum_i += (float)testADC;
       }
       ++testIndexRight;
       if (testIndexRight<nStrips) {
         rangeRight = stripId[testIndexRight]-stripId[indexRight]-1;
         sameDetRight = detId[index] == detId[testIndexRight];
       }
     }
   }
   */

   bool noiseSquaredPass = noiseSquared_i*ClusterThresholdSquared <= adcSum_i*adcSum_i;
   trueCluster[i] = noiseSquaredPass;
   clusterLastIndexLeft[i] = indexLeft;
   clusterLastIndexRight[i] = indexRight;
   
#ifndef NDRANGE
   }
#endif
}


__kernel void checkClusterCondition(int nSeedStripsNC, __global uint16_t* restrict stripId, __global uint8_t* restrict adc, __global float * restrict gain, __global int* restrict clusterLastIndexLeft, __global int* restrict clusterLastIndexRight, __global bool* restrict trueCluster, __global uint8_t* restrict clusterADCs, __global float* restrict barycenter, int N) {

   const float minGoodCharge = 1620.0;
   const uint16_t stripIndexMask = 0x7FFF;

#ifdef NDRANGE
   const unsigned i = get_global_id(0);
#else
   for (unsigned i=0; i<N; i++) {
#endif

     int left=clusterLastIndexLeft[i];
     int right=clusterLastIndexRight[i];
     int size=right-left+1;

     float adcSum=0.0f;
     int sumx=0;
     int suma=0;
     for (unsigned j=0; j<size; j++){
       uint8_t adc_j = adc[left+j];
       float gain_j = gain[left+j];
       int charge = (int)( (float)adc_j/gain_j + 0.5f );
       if (adc_j < 254) adc_j = ( charge > 1022 ? 255 : (charge > 253 ? 254 : charge));
       clusterADCs[j*nSeedStripsNC+i] = adc_j;
       adcSum += (float)adc_j;
       sumx += j*adc_j;
       suma += adc_j;
     }

     barycenter[i] = (float)(stripId[left] & stripIndexMask) + (float)sumx/(float)suma + 0.5f;

     bool trueClust=trueCluster[i];
     if (trueClust) {
       if (i>0&&clusterLastIndexLeft[i-1]==left) {
         trueClust = 0;  // ignore duplicates
       }
       trueClust =  (adcSum/0.047f) > minGoodCharge;
     }
     trueCluster[i] = trueClust;
#ifndef NDRANGE
   }
#endif
}
