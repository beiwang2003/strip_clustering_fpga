#include <mm_malloc.h>
#include <iostream>
#include <cmath>
#include <cassert>
#include "cluster.h"

void readinRawData(const std::string& datafilename, const SiStripConditions *conditions, std::vector<FEDRawData>& fedRawDatav, std::vector<FEDBuffer>& fedBufferv, std::vector<fedId_t>& fedIndex, FEDReadoutMode& mode, sst_data_t* sst_data) {

  std::ifstream datafile(datafilename, std::ios::in | std::ios::binary);
  datafile.seekg(sizeof(size_t)); // skip initial event mark

  //std::vector<FEDRawData> fedRawDatav;
  std::vector<fedId_t> fedIdv;
  //std::vector<FEDBuffer> fedBufferv;
  //std::vector<fedId_t> fedIndex(SiStripConditions::kFedCount);

  //  ChannelLocs chanlocs(conditions->detToFeds().size(), stream);

  fedRawDatav.reserve(SiStripConditions::kFedCount);
  fedIdv.reserve(SiStripConditions::kFedCount);
  fedBufferv.reserve(SiStripConditions::kFedCount);
  fedIndex.reserve(SiStripConditions::kFedCount);
  auto eventno = 0;

  while (!datafile.eof()) {
    eventno++;
    size_t size = 0;
    size_t totalSize = 0;
    mode = READOUT_MODE_INVALID;

    fedRawDatav.clear();
    fedBufferv.clear();
    fedIdv.clear();
    fedIndex.clear();
    fedIndex.resize(SiStripConditions::kFedCount, invFed);
    //alldata.clear();

    // read in the raw data
    while (datafile.read((char*) &size, sizeof(size)).gcount() == sizeof(size) && size != std::numeric_limits<size_t>::max()) {
      int fedId = 0;
      datafile.read((char*) &fedId, sizeof(fedId));
#if defined(DBGPRINT)
u      std::cout << "Event # " <<eventno<< " Reading FEDRawData ID " << fedId << " size " << size << std::endl;
#endif
      fedRawDatav.emplace_back(size);
      auto& rawData = fedRawDatav.back();

      datafile.read((char*) rawData.get(), size);

      fedIndex[fedId-SiStripConditions::kFedFirst] = fedIdv.size();
      fedIdv.push_back(fedId);

      fedBufferv.emplace_back(rawData.get(), rawData.size());

      if (fedBufferv.size() == 1) {
        mode = fedBufferv.back().readoutMode();
      } else {
	assert(fedBufferv.back().readoutMode() == mode);
      }
      totalSize += size;
    }
    sst_data->totalRawSize = totalSize;
    //std::cout<<"Readin Raw data size "<<totalSize<<std::endl;
  }
}

void unpackRawData(const SiStripConditions *conditions, const std::vector<FEDRawData>& fedRawDatav, const std::vector<FEDBuffer>& fedBufferv, const std::vector<fedId_t>& fedIndex, sst_data_t *sst_data, calib_data_t *calib_data, const FEDReadoutMode& mode) {

  ChannelLocs chanlocs(conditions->detToFeds().size());
  const auto& detmap = conditions->detToFeds();
  size_t offset = 0;

  // iterate over the detector in DetID/APVPair order
  // mapping out where the data are
  const uint16_t headerlen = mode == READOUT_MODE_ZERO_SUPPRESSED ? 7 : 2;

  for(size_t i = 0; i < detmap.size(); ++i) {
    const auto& detp = detmap[i];

    auto fedId = detp.fedID();
    auto fedi = fedIndex[fedId-SiStripConditions::kFedFirst];
    if (fedi != invFed) {
      const auto& buffer = fedBufferv[fedi];
      const auto& channel = buffer.channel(detp.fedCh());

      if (channel.length() >= headerlen) {
	chanlocs.setChannelLoc(i, channel.data(), channel.offset()+headerlen, offset, channel.length()-headerlen, detp.fedID(), detp.fedCh());
	offset += channel.length()-headerlen;
        } else {
	chanlocs.setChannelLoc(i, channel.data(), channel.offset(), offset, channel.length(), detp.fedID(), detp.fedCh());
	offset += channel.length();
	assert(channel.length() == 0);
      }
    } else {
      chanlocs.setChannelLoc(i, nullptr, 0, 0, 0, invFed, 0);
      std::cout << "Missing fed " << fedi << " for detID " << detp.fedID() << std::endl;
      exit (1);
    }
  }

  sst_data->nStrips = offset;
  //std::cout<<"Readin Channel raw data size "<<offset<<std::endl;
  assert(offset < MAX_STRIPS);
  //alldata.resize(offset); // resize to the amount of data

  unpack(chanlocs, conditions, sst_data, calib_data);

#ifdef CPU_DEBUG
  for (int i=0; i<sst_data->nStrips; i++) {
    std::cout<<" i "<<i<<" cpu fedId "<<sst_data->fedId[i]<<" cpu_strip "<<sst_data->stripId[i]<<" cpu_adc "<<(unsigned int)sst_data->adc[i]<<std::endl;
  }
#endif

}

void unpack(const ChannelLocs& chanlocs, const SiStripConditions* conditions, sst_data_t *sst_data, calib_data_t *calib_data) {

#pragma omp parallel for
  for (int chan=0; chan<chanlocs.size(); chan++) {
    const auto fedid = chanlocs.fedID(chan);
    const auto fedch = chanlocs.fedCh(chan);
    const auto detid = conditions->detID(fedid, fedch);
    //if (chan==0) printf("0 fedId=%d fedCh=%d detid=%d\n", fedid, fedch, detid);
    const auto ipoff = SiStripConditionsBase::kStripsPerChannel*conditions->iPair(fedid, fedch);

    const auto data = chanlocs.input(chan);
    const auto len = chanlocs.length(chan);

    if (data != nullptr && len > 0) {
      auto aoff = chanlocs.offset(chan);
      auto choff = chanlocs.inoff(chan);
      const auto end = aoff + len;

      while (aoff < end) {
        sst_data->stripId[aoff] = invStrip;
        sst_data->detId[aoff] = invDet;
        sst_data->adc[aoff] = data[(choff++)^7];
	//if (chan==0) printf("1 stripId=%d detId=%d adc=%d\n", sst_data->stripId[aoff], sst_data->detId[aoff],(int)sst_data->adc[aoff]);
        auto stripIndex = sst_data->adc[aoff++] + ipoff;


        sst_data->stripId[aoff] = invStrip;
        sst_data->detId[aoff] = detid;
        sst_data->adc[aoff] = data[(choff++)^7];
	//if (chan==0) printf("2 stripId=%d detId=%d adc=%d\n", sst_data->stripId[aoff], sst_data->detId[aoff],(int)sst_data->adc[aoff]);
        const auto groupLength = sst_data->adc[aoff++];

        for (auto i = 0; i < groupLength; ++i) {
#ifdef CALIB_1D
          calib_data->noise[aoff] = conditions->noise(fedid, fedch, stripIndex);
          calib_data->gain[aoff]  = conditions->gain(fedid, fedch, stripIndex);
          calib_data->bad[aoff]   = conditions->bad(fedid, fedch, stripIndex);
#endif
	  sst_data->fedId[aoff] = fedid;
	  sst_data->fedCh[aoff] = fedch;
          sst_data->detId[aoff] = detid;
          sst_data->stripId[aoff] = stripIndex++;
	  //if (chan==0) printf("3 stripId=%d detId=%d\n", sst_data->stripId[aoff], sst_data->detId[aoff]);
          sst_data->adc[aoff++] = data[(choff++)^7];
        }
      }
    }
  }
}

void allocateSSTData(int max_strips, sst_data_t *sst_data){
  sst_data->detId = (detId_t *)_mm_malloc(max_strips*sizeof(detId_t), IDEAL_ALIGNMENT);
  sst_data->fedId = (fedId_t *)_mm_malloc(max_strips*sizeof(fedId_t), IDEAL_ALIGNMENT);
  sst_data->fedCh = (fedCh_t *)_mm_malloc(max_strips*sizeof(fedCh_t), IDEAL_ALIGNMENT);
  sst_data->stripId = (uint16_t *)_mm_malloc(max_strips*sizeof(uint16_t), IDEAL_ALIGNMENT);
  sst_data->adc = (uint8_t *)_mm_malloc(max_strips*sizeof(uint8_t), IDEAL_ALIGNMENT);
  sst_data->seedStripsMask = (int *)_mm_malloc(2*max_strips*sizeof(int), IDEAL_ALIGNMENT);
  sst_data->seedStripsNCMask = sst_data->seedStripsMask + max_strips;
  sst_data->prefixSeedStripsNCMask = (int *)_mm_malloc(2*max_strips*sizeof(int), IDEAL_ALIGNMENT);
  sst_data->seedStripsNCIndex = sst_data->prefixSeedStripsNCMask + max_strips;
#ifdef NUMA_FT
#pragma omp parallel for
  for (int i=0; i<max_strips; i++) {
    sst_data->detId[i] = 0;
    sst_data->stripId[i] = 0;
    sst_data->fedId[i] = 0;
    sst_data->fedCh[i] = 0;
    sst_data->adc[i] = 0;
    sst_data->seedStripsMask[i] = 0;
    sst_data->seedStripsNCMask[i] = 0;
    sst_data->prefixSeedStripsNCMask[i] = 0;
    sst_data->seedStripsNCIndex[i] = 0;
  }
#endif
}

void allocateCalibData(int max_strips, calib_data_t *calib_data){
  calib_data->noise = (float *)_mm_malloc(2*max_strips*sizeof(float), IDEAL_ALIGNMENT);
  calib_data->gain = calib_data->noise + max_strips;
  calib_data->bad = (bool *)_mm_malloc(max_strips*sizeof(bool), IDEAL_ALIGNMENT);
#ifdef NUMA_FT
#pragma omp parallel for
  for (int i=0; i<max_strips; i++) {
    calib_data->noise[i] = 0.0;
    calib_data->gain[i] = 0.0;
    calib_data->bad[i] = false;
  }
#endif
}

void allocateClustData(int max_seedstrips, clust_data_t *clust_data){
  clust_data->clusterLastIndexLeft = (int *)_mm_malloc(2*max_seedstrips*sizeof(int), IDEAL_ALIGNMENT);
  clust_data->clusterLastIndexRight = clust_data->clusterLastIndexLeft + max_seedstrips;
  clust_data->clusterADCs = (uint8_t *)_mm_malloc(max_seedstrips*256*sizeof(uint8_t), IDEAL_ALIGNMENT);
  clust_data->trueCluster = (bool *)_mm_malloc(max_seedstrips*sizeof(bool), IDEAL_ALIGNMENT);
  clust_data->barycenter = (float *)_mm_malloc(max_seedstrips*sizeof(float), IDEAL_ALIGNMENT);
#ifdef NUMA_FT
#pragma omp parallel for
  for (int i=0; i<max_seedstrips; i++) {
    clust_data->clusterLastIndexLeft[i] = 0;
    clust_data->clusterLastIndexRight[i] = 0;
    for (int j=0; j<256; j++) {
      clust_data->clusterADCs[i*256+j] = 0;
    }
    clust_data->trueCluster[i] = false;
    clust_data->barycenter[i] = 0.0;
  }
#endif
}

void freeSSTData(sst_data_t *sst_data) {
  free(sst_data->detId);
  free(sst_data->stripId);
  free(sst_data->adc);
  free(sst_data->fedId);
  free(sst_data->fedCh);
  free(sst_data->seedStripsMask);
  free(sst_data->prefixSeedStripsNCMask);
}

void freeCalibData(calib_data_t *calib_data) {
  free(calib_data->noise);
  free(calib_data->bad);
}

void freeClustData(clust_data_t *clust_data) {
  free(clust_data->clusterLastIndexLeft);
  free(clust_data->clusterADCs);
  free(clust_data->trueCluster);
  free(clust_data->barycenter);
}

void setSeedStripsNCIndex(sst_data_t *sst_data, calib_data_t *calib_data, const SiStripConditions *conditions) {
  const detId_t *__restrict__ detId = sst_data->detId;
  const uint16_t *__restrict__ stripId = sst_data->stripId;
  const uint8_t *__restrict__ adc = sst_data->adc;
  const float *__restrict__ noise = calib_data->noise;
  const fedId_t *__restrict__ fedId = sst_data->fedId;
  const fedCh_t *__restrict__ fedCh = sst_data->fedCh;
  const int nStrips = sst_data->nStrips;
  int *__restrict__ seedStripsNCIndex = sst_data->seedStripsNCIndex;
  int *__restrict__ seedStripsMask = sst_data->seedStripsMask;
  int *__restrict__ seedStripsNCMask = sst_data->seedStripsNCMask;
  int *__restrict__ prefixSeedStripsNCMask = sst_data->prefixSeedStripsNCMask;

  int j;
  int nStripsP2 = std::pow(2, std::floor(std::log2(nStrips))+1);  int depth = std::log2(nStripsP2);

  float SeedThreshold = 3.0;
  double t0, t1;

#pragma omp parallel
  {
    // mark seed strips
#pragma omp for simd aligned(noise,fedId,fedCh,stripId,adc,seedStripsMask,seedStripsNCMask,prefixSeedStripsNCMask: CACHELINE_BYTES)
    for (int i=0; i<nStrips; i++) {
      stripId_t strip = stripId[i];
      if (strip !=invStrip) {
#ifdef CALIB_1D
      float noise_i = noise[i];
#else
      fedId_t fed = fedId[i];
      fedCh_t channel = fedCh[i];
      //stripId_t strip = stripId[i];
      float noise_i = conditions->noise(fed, channel, strip);
#endif
      uint8_t adc_i = adc[i];
      seedStripsMask[i] = (adc_i >= static_cast<uint8_t>( noise_i * SeedThreshold)) ? 1:0;
      } else {
	seedStripsMask[i] = 0;
      }
      seedStripsNCMask[i] = seedStripsMask[i];
      prefixSeedStripsNCMask[i] = 0;
    }

#pragma omp for
    for (int i=nStrips; i<nStripsP2; i++) {
      prefixSeedStripsNCMask[i] = 0;
    }

    // mark only non-consecutive seed strips (mask out consecutive seed strips)
#pragma omp for simd aligned(seedStripsNCMask,prefixSeedStripsNCMask: CACHELINE_BYTES)
    for (int i=0; i<nStrips; i++) {
      int mask = seedStripsNCMask[i];
      if (i>0&&seedStripsMask[i]&&seedStripsMask[i-1]&&(stripId[i]-stripId[i-1])==1&&(detId[i]==detId[i-1])) mask = 0;
      prefixSeedStripsNCMask[i] = mask;
      seedStripsNCMask[i] = mask;
    }
    // set index for non-consecutive seed strips
    // parallel prefix sum implementation
    // The up-sweep (reduce) phase of a work-efficient sum scan algorithm
    for (int d=0; d<depth; d++) {
      int stride = std::pow(2, d);
      int stride2 = 2*stride;
#pragma omp for
      for (int k=0; k<nStripsP2; k+=stride2) {
	prefixSeedStripsNCMask[k+stride2-1] += prefixSeedStripsNCMask[k+stride-1];
      }
    }

    // The down-sweep phase of a work-efficient sum scan algorithm

#pragma omp single
    {
      sst_data->nSeedStripsNC = prefixSeedStripsNCMask[nStripsP2-1];
      prefixSeedStripsNCMask[nStripsP2-1] = 0;
    }

    for (int d=depth-1; d>=0; d--) {
      int stride = std::pow(2, d);
      int stride2 = 2*stride;
#pragma omp for
      for (int k=0; k<nStripsP2; k+=stride2){
	int temp = prefixSeedStripsNCMask[k+stride-1];
	prefixSeedStripsNCMask[k+stride-1] = prefixSeedStripsNCMask[k+stride2-1];
	prefixSeedStripsNCMask[k+stride2-1] += temp;
      }
    }

#pragma omp for
    for (int i=0; i<nStrips; i++) {
      if (seedStripsNCMask[i]) {
	int index = prefixSeedStripsNCMask[i];
	seedStripsNCIndex[index] = i;
      }
    }
  }

#ifdef CPU_DEBUG
  for (int i=0; i<nStrips; i++) {
    if (seedStripsNCMask[i])
      std::cout<<" i "<<i<<" mask "<<seedStripsNCMask[i]<<" prefix "<<prefixSeedStripsNCMask[i]<<" index "<<seedStripsNCIndex[i]<<std::endl;
  }
  std::cout<<"nStrips="<<nStrips<<"nSeedStripsNC="<<sst_data->nSeedStripsNC<<std::endl;
#endif

}

static void findLeftRightBoundary(sst_data_t *sst_data, calib_data_t *calib_data, const SiStripConditions *conditions, clust_data_t *clust_data) {
  const int *__restrict__ seedStripsNCIndex = sst_data->seedStripsNCIndex;
  const detId_t *__restrict__ detId = sst_data->detId;
  const uint16_t *__restrict__ stripId = sst_data->stripId;
  const uint8_t *__restrict__ adc = sst_data->adc;
  const int nSeedStripsNC = sst_data->nSeedStripsNC;
  const float *__restrict__ noise = calib_data->noise;
  const fedId_t *__restrict__ fedId = sst_data->fedId;
  const fedCh_t *__restrict__ fedCh = sst_data->fedCh;
  const int nStrips=sst_data->nStrips;
  int *__restrict__ clusterLastIndexLeft = clust_data->clusterLastIndexLeft;
  int *__restrict__ clusterLastIndexRight = clust_data->clusterLastIndexRight;
  bool *__restrict__ trueCluster = clust_data->trueCluster;

  uint8_t MaxSequentialHoles = 0;
  float  ChannelThreshold = 2.0;
  const float ClusterThresholdSquared = 25.0;

  // (currently, we assume no bad strip. fix later)
#pragma omp parallel for
  for (int i=0; i<nSeedStripsNC; i++) {
    int index=seedStripsNCIndex[i];
    int indexLeft = index;
    int indexRight = index;
#ifdef CALIB_1D
    float noise_i = noise[index];
#else
    fedId_t fed = fedId[index];
    fedCh_t channel = fedCh[index];
    stripId_t strip = stripId[index];
    float noise_i = conditions->noise(fed, channel, strip);
#endif
    float noiseSquared_i = noise_i*noise_i;
    float adcSum_i = static_cast<float>(adc[index]);

    // find left boundary
    int testIndexLeft=index-1;
    if (testIndexLeft>=0) {
      int rangeLeft = stripId[indexLeft]-stripId[testIndexLeft]-1;
      bool sameDetLeft = detId[index] == detId[testIndexLeft];
      while(sameDetLeft&&testIndexLeft>=0&&rangeLeft>=0&&rangeLeft<=MaxSequentialHoles) {
#ifdef CALIB_1D
	float testNoise = noise[testIndexLeft];
#else
	fedId_t testFed = fedId[testIndexLeft];
	fedCh_t testChannel = fedCh[testIndexLeft];
	stripId_t testStrip = stripId[testIndexLeft];
	float testNoise = conditions->noise(testFed, testChannel, testStrip);
#endif
	uint8_t testADC = adc[testIndexLeft];

	if (testADC >= static_cast<uint8_t>(testNoise * ChannelThreshold)) {
	  --indexLeft;
	  noiseSquared_i += testNoise*testNoise;
	  adcSum_i += static_cast<float>(testADC);
	}
	--testIndexLeft;
	if (testIndexLeft>=0) {
	  rangeLeft =stripId[indexLeft]-stripId[testIndexLeft]-1;
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
#ifdef CALIB_1D
	float testNoise = noise[testIndexRight];
#else
	fedId_t testFed = fedId[testIndexRight];
        fedCh_t testChannel = fedCh[testIndexRight];
        stripId_t testStrip = stripId[testIndexRight];
        float testNoise = conditions->noise(testFed, testChannel, testStrip);
#endif
	uint8_t testADC = adc[testIndexRight];
	if (testADC >= static_cast<uint8_t>(testNoise * ChannelThreshold)) {
	  ++indexRight;
	  noiseSquared_i += testNoise*testNoise;
	  adcSum_i += static_cast<float>(testADC);
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
    if (i==0) printf("left=%d right=%d true=%d\n", indexLeft, indexRight, noiseSquaredPass);
  }
}

static void checkClusterCondition(sst_data_t *sst_data,calib_data_t *calib_data, const SiStripConditions* conditions, clust_data_t *clust_data) {
  const int *__restrict__ clusterLastIndexLeft = clust_data->clusterLastIndexLeft;
  const int *__restrict__ clusterLastIndexRight = clust_data->clusterLastIndexRight;
  const uint16_t *__restrict__ stripId = sst_data->stripId;
  const uint8_t *__restrict__ adc = sst_data->adc;
  const int nSeedStripsNC = sst_data->nSeedStripsNC;
  const float *__restrict__ gain = calib_data->gain;
  const fedId_t *__restrict__ fedId = sst_data->fedId;
  const fedCh_t *__restrict__ fedCh = sst_data->fedCh;
  bool *__restrict__ trueCluster = clust_data->trueCluster;
  uint8_t *__restrict__ clusterADCs = clust_data->clusterADCs;
  float *__restrict__ barycenter = clust_data->barycenter;
  const float minGoodCharge = 1620.0;
  const uint16_t stripIndexMask = 0x7FFF;

#pragma omp parallel for
  for (int i=0; i<nSeedStripsNC; i++){
    if (trueCluster[i]) {

      int left=clusterLastIndexLeft[i];
      int right=clusterLastIndexRight[i];
      int size=right-left+1;
      float adcSum=0.0f;
      int sumx=0;
      int suma=0;

      if (i>0&&clusterLastIndexLeft[i-1]==left) {
        trueCluster[i] = 0;  // ignore duplicates
      } else {
        for (int j=0; j<size; j++) {
          uint8_t adc_j = adc[left+j];
#ifdef CALIB_1D
          float gain_j = gain[left+j];
#else
	  fedId_t fed = fedId[left+j];
	  fedCh_t channel = fedCh[left+j];
	  stripId_t strip = stripId[left+j];
	  float gain_j = conditions->gain(fed, channel, strip);
#endif
          auto charge = int( float(adc_j)/gain_j + 0.5f );
          if (adc_j < 254) adc_j = ( charge > 1022 ? 255 : (charge > 253 ? 254 : charge));
          clusterADCs[j*nSeedStripsNC+i] = adc_j;
          adcSum += static_cast<float>(adc_j);
	  sumx += j*adc_j;
	  suma += adc_j;
	  if (i==0) printf("j=%d left=%d right=%d adc_j=%d gain=%f sumx=%d suma=%d, adcSum=%f barycenter=%f trueCluster=%d\n", j, left, right, (int)adc_j, gain_j, sumx, suma, adcSum, barycenter[i],trueCluster[i]);
        }
	barycenter[i] = static_cast<float>(stripId[left] & stripIndexMask) + static_cast<float>(sumx)/static_cast<float>(suma) + 0.5f;
      }
      trueCluster[i] = adcSum/0.047f > minGoodCharge;
    }
  }
}

void findCluster(sst_data_t *sst_data, calib_data_t *calib_data, const SiStripConditions* conditions, clust_data_t *clust_data){

  findLeftRightBoundary(sst_data, calib_data, conditions, clust_data);

  checkClusterCondition(sst_data, calib_data, conditions, clust_data);

}
