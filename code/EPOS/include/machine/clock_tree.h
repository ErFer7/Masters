// EPOS Clock Tree Mediator Common Package

#ifndef __clock_tree_h
#define __clock_tree_h

#include <system/config.h>
#include <utility/debug.h>

__BEGIN_SYS

class Clock_Tree_Common {
  public:
    static void init() {}
};

__END_SYS

#endif

#if defined(__CLK_H) && !defined(__CLK_common_only__)
#include __CLK_H
#endif
