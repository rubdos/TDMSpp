add_executable(tdmsppinfo tdmsppinfo.cpp)
target_link_libraries(tdmsppinfo tdmspp)
set_property(TARGET tdmsppinfo PROPERTY CXX_STANDARD 11)
set_property(TARGET tdmsppinfo PROPERTY CXX_STANDARD_REQUIRED ON)
