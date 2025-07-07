//
// Created by wastl on 02.07.25.
//

#include "fronius_module.h"

int16_t uint2int(uint16_t u) {
    if (u <= (uint16_t)INT16_MAX)
        return (int16_t)u;
    else
        return -(int16_t)~u - 1;
}