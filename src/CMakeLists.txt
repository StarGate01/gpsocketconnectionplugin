find_package(PkgConfig REQUIRED)
pkg_check_modules(GLOBALPLATFORM REQUIRED globalplatform)

add_library(gpsocketconnectionplugin SHARED gpsocketconnectionplugin.c util.c)

set_target_properties(gpsocketconnectionplugin PROPERTIES VERSION 1.0.0 SOVERSION 1 )

include_directories(${GLOBALPLATFORM_INCLUDE_DIRS})

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
SET(CMAKE_C_FLAGS_DEBUG "-g -O0")

target_link_libraries(gpsocketconnectionplugin ${GLOBALPLATFORM_LIBRARIES})
target_compile_options(gpsocketconnectionplugin PRIVATE ${GLOBALPLATFORM_CFLAGS_OTHER})
target_include_directories(gpsocketconnectionplugin PRIVATE ${PROJECT_SOURCE_DIR}/src)

install(TARGETS gpsocketconnectionplugin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})