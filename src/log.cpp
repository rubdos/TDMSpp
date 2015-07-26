#include "log.hpp"

namespace TDMS
{

log log::debug;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
const std::string log::endl ="\r\n";
#else
const std::string log::endl="\n";
#endif
}
