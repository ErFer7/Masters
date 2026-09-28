// EPOS Cache Mediator Common Package

#ifndef __cache_h
#define __cache_h

#include <system/config.h>
#include <utility/debug.h>

__BEGIN_SYS

class Cache_Common {
  protected:
    Cache_Common() {}

  public:
    static void init() {}

    static void flush(const void *const ptr, unsigned int size) {}

    static void barrier() {}
};

__END_SYS

#endif

#if defined(__Cache_H) && !defined(__cache_common_only__)
#include __Cache_H
#endif
