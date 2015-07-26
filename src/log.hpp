#pragma once

#include <iostream>

namespace TDMS
{

class log
{
public:
    log()
    {
        debug_mode = false;
    }
    static log debug;
    static const std::string endl;

    template<typename T>
    const log& operator<< (const T& o) const
    {
        if(debug_mode)
        {
            std::cout << o;
        }
        return *this;
    }
    bool debug_mode;
};
}
