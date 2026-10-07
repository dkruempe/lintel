#ifndef LINTEL_BOOSTSEGMENTALLOCATOR_H
#define LINTEL_BOOSTSEGMENTALLOCATOR_H

#include <boost/interprocess/managed_mapped_file.hpp>

#include "lintel/features/base/events/ShmSegmentAccessor.h"

/**
 * Allocator adapter that binds ShmSegmentAccessor to the boost interprocess
 * segment manager. This is the only translation unit boundary where the two
 * meet; SharedMemoryService.cpp and the examples keep the boost includes, all
 * other users of ShmSegmentAccessor stay boost free.
 *
 * The allocator has to outlive every accessor created from it, because
 * accessors stored in shared memory objects dereference it lazily.
 */
class BoostSegmentAllocator final : public ShmSegmentAccessor::Allocator
{
public:
  /** @param segment segment manager of the mapped shared memory file */
  explicit BoostSegmentAllocator(boost::interprocess::managed_mapped_file::segment_manager &segment)
    : m_segment(&segment)
  {}

  void *allocate(std::size_t bytes) override { return m_segment->allocate(bytes); }

  void deallocate(void *pointer) override { m_segment->deallocate(pointer); }

private:
  boost::interprocess::managed_mapped_file::segment_manager *m_segment;
};

#endif// LINTEL_BOOSTSEGMENTALLOCATOR_H
