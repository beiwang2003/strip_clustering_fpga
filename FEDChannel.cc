#include <cassert>

#include "FEDChannel.h"

ChannelLocs::ChannelLocs(size_t size)
  : ChannelLocsBase(size)
{
  if (size > 0) {
    input_ = std::make_unique<const uint8_t*[]>(size);
    inoff_ = std::make_unique<size_t[]>(size);
    offset_ = std::make_unique<size_t[]>(size);
    length_ = std::make_unique<uint16_t[]>(size);
    fedID_ = std::make_unique<fedId_t[]>(size);
    fedCh_ = std::make_unique<fedCh_t[]>(size);
  }
}

ChannelLocs::~ChannelLocs() {}
