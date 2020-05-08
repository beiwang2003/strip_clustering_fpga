typedef uchar uint8_t;
typedef ushort uint16_t;
typedef uint uint32_t;

__kernel void findLeftRightBoundary(int nStrips, __global uint32_t* restrict detId, __global uint16_t* restrict stripId, __global uint8_t* restrict adc, __global int* restrict seedStripsNCIndex, __global float* restrict noise, __global int* restrict clusterLastIndexLeft, __global int* restrict clusterLastIndexRight, __global bool* restrict trueCluster) {

   const uint8_t MaxSequentialHoles = 0;
   const float  ChannelThreshold = 2.0;
   const float ClusterThresholdSquared = 25.0;
   const unsigned i = get_global_id(0);
//   printf("i=%u nStrips=%d\n", i, nStrips);

   int index, testIndexLeft, testIndexRight, indexLeft, indexRight, rangeLeft, rangeRight;
   uint8_t testADC;
   float noise_i, testNoise, noiseSquared_i, adcSum_i;
   bool noiseSquaredPass, sameDetLeft, sameDetRight;
   uint16_t strip, testStrip;

   index=seedStripsNCIndex[i];
   indexLeft = index;
   indexRight = index;
   noise_i = noise[index];

   noiseSquared_i = noise_i*noise_i;
   adcSum_i = (float)adc[index];

     // find left boundary
   testIndexLeft=index-1;
   if (testIndexLeft>=0) {
     rangeLeft = stripId[indexLeft]-stripId[testIndexLeft]-1;
     sameDetLeft = detId[index] == detId[testIndexLeft];
     while(sameDetLeft&&testIndexLeft>=0&&rangeLeft>=0&&rangeLeft<=MaxSequentialHoles) {
       testNoise = noise[testIndexLeft];
       testADC = adc[testIndexLeft];

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
   testIndexRight=index+1;
   if (testIndexRight<nStrips) {
     rangeRight = stripId[testIndexRight]-stripId[indexRight]-1;
     sameDetRight = detId[index] == detId[testIndexRight];
     while(sameDetRight&&testIndexRight<nStrips&&rangeRight>=0&&rangeRight<=MaxSequentialHoles) {
       testNoise = noise[testIndexRight];
       testADC = adc[testIndexRight];
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

   noiseSquaredPass = noiseSquared_i*ClusterThresholdSquared <= adcSum_i*adcSum_i;
   trueCluster[i] = noiseSquaredPass;
   clusterLastIndexLeft[i] = indexLeft;
   clusterLastIndexRight[i] = indexRight;
}

__kernel void checkClusterCondition(int nSeedStripsNC, __global uint16_t* restrict stripId, __global uint8_t* restrict adc, __global float* restrict gain, __global int* restrict clusterLastIndexLeft, __global int* restrict clusterLastIndexRight, __global bool* restrict trueCluster, __global uint8_t* restrict clusterADCs, __global float* restrict barycenter) {

   const float minGoodCharge = 1620.0;
   const uint16_t stripIndexMask = 0x7FFF;
   const unsigned i = get_global_id(0);

   int left, right, size, j;
   int charge;
   uint8_t adc_j;
   float gain_j;
   float adcSum=0.0f;
   int sumx=0;
   int suma=0;

   if (trueCluster[i]) {
     left=clusterLastIndexLeft[i];
     right=clusterLastIndexRight[i];
     size=right-left+1;

     if (i>0&&clusterLastIndexLeft[i-1]==left) {
       trueCluster[i] = 0;  // ignore duplicates
     } else {
       for (j=0; j<size; j++){
         adc_j = adc[left+j];
         gain_j = gain[left+j];
	 charge = (int)( (float)adc_j/gain_j + 0.5f );
	 if (adc_j < 254) adc_j = ( charge > 1022 ? 255 : (charge > 253 ? 254 : charge));
	 clusterADCs[j*nSeedStripsNC+i] = adc_j;
	 adcSum += (float)adc_j;
	 sumx += j*adc_j;
	 suma += adc_j;
       }
       barycenter[i] = (float)(stripId[left] & stripIndexMask) + (float)sumx/(float)suma + 0.5f;
     }    

     trueCluster[i] = (adcSum/0.047f) > minGoodCharge;
   }   
}

