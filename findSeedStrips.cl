typedef uchar uint8_t;
typedef ushort uint16_t;
typedef uint uint32_t;

__kernel void setSeedStrips(__global uint16_t* restrict stripId, __global int* restrict seedStripsMask, __global int* restrict seedStripsNCMask, __global uint8_t* restrict adc, __global float * restrict noise, uint16_t invStrip) {

  const float SeedThreshold = 3.0;
  const unsigned i = get_global_id(0);


  uint16_t strip;
  float noise_i;
  uint8_t adc_i;

  seedStripsMask[i] = 0;
  seedStripsNCMask[i] = 0;
  strip = stripId[i];
  if (strip != invStrip) {
    noise_i = noise[i];
    adc_i = adc[i];
    seedStripsMask[i] = (adc_i >= (uint8_t)( noise_i * SeedThreshold)) ? 1:0;
    seedStripsNCMask[i] = seedStripsMask[i];
  }
}

__kernel void setNCSeedStrips(__global uint32_t* restrict detId, __global uint16_t* restrict stripId, __global int* restrict seedStripsMask, __global int* restrict seedStripsNCMask) { 

  const unsigned i = get_global_id(0);

  if (i>0&&seedStripsMask[i]&&seedStripsMask[i-1]&&(stripId[i]-stripId[i-1])==1&&(detId[i]==detId[i-1])) seedStripsNCMask[i] = 0;
}


__kernel void setStripIndex(__global int* restrict seedStripsNCMask, __global int* restrict prefixSeedStripsNCMask, __global int* restrict seedStripsNCIndex) {
	 
  const unsigned i = get_global_id(0);

  int index;

  if (seedStripsNCMask[i] == 1) {
    index = prefixSeedStripsNCMask[i];
    seedStripsNCIndex[index] = i;
  }
}

