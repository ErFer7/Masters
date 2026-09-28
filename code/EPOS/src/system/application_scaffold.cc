// EPOS Application Scaffold and Application Component Implementation

#include <utility/spin.h>
#include <system.h>
#include <network.h>

// Application class attributes
__BEGIN_SYS
char Application::_preheap[];
Application::Heap * Application::_heap;

__END_SYS

__BEGIN_API

// Global objects
__USING_UTIL
OStream cout;
OStream cerr;

__END_API


// Initialize application objects that need the system to be fully functional and running, like DHCP clients
extern "C" {
    void __pre_main() {

#ifdef __NIC_H
        if(_SYS::Traits<_SYS::Network>::enabled)
            _SYS::Network::init();
#endif
    }
}
