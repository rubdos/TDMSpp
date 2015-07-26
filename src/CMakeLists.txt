add_library(tdmspp log.cpp tdms_file.cpp tdms_segment.cpp)
set_property(TARGET tdmspp PROPERTY CXX_STANDARD 11)
set_property(TARGET tdmspp PROPERTY CXX_STANDARD_REQUIRED ON)
