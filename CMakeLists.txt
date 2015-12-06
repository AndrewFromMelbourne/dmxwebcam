cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

link_directories(/opt/vc/lib)

add_executable(dmxwebcam dmxwebcam.c
                         backgroundLayer.c
                         syslogUtilities.c
                         yuv420Image.c
                         yuv420ImageLayer.c)

target_link_libraries(dmxwebcam bcm_host bsd)

set_property(TARGET dmxwebcam PROPERTY SKIP_BUILD_RPATH TRUE)
install (TARGETS dmxwebcam RUNTIME DESTINATION bin)
