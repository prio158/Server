//
// Created by czr on 24-3-17.
//

#ifndef SERVER_HOOK_H
#define SERVER_HOOK_H

#include <unistd.h>

namespace Server {
    bool is_hook_enable();

    void set_hook_enable(bool flag);
}

extern "C" {
///sleep
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

///usleep
typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;




}



class Hook {

};


#endif //SERVER_HOOK_H
