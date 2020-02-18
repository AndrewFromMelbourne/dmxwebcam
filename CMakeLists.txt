cmake_minimum_required(VERSION 2.6)

find_package(JPEG REQUIRED)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)
include_directories(${JPEG_INCLUDE_DIRS})

link_directories(/opt/vc/lib)

add_executable(dmxwebcam dmxwebcam.c
                         backgroundLayer.c
                         syslogUtilities.c
                         image.c
                         imageLayer.c)

target_link_libraries(dmxwebcam bcm_host bsd ${JPEG_LIBRARIES})

set_property(TARGET dmxwebcam PROPERTY SKIP_BUILD_RPATH TRUE)
install (TARGETS dmxwebcam RUNTIME DESTINATION bin)
