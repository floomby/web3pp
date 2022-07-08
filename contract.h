#pragma once

#include "iostream"

class Contract {
   public:
    virtual const char *__data() = 0;
    inline void deploy() {
        std::cout << __data() << std::endl;
    }
};