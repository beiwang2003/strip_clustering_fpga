#include "clppScan_Default.cl"

typedef uchar uint8_t;
typedef ushort uint16_t;
typedef uint uint32_t;

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

__kernel void findLeftRightBoundary(int nStrips, __global uint32_t* restrict detId, __global uint16_t* restrict stripId, __global uint8_t* restrict adc, __global int* restrict seedStripsNCIndex, __global float* restrict noise, __global int* restrict clusterLastIndexLeft, __global int* restrict clusterLastIndexRight, __global bool* restrict trueCluster, int N) {

   const uint8_t MaxSequentialHoles = 0;
   const float  ChannelThreshold = 2.0;
   const float ClusterThresholdSquared = 25.0;

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
