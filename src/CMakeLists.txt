find_package(PkgConfig REQUIRED)
pkg_check_modules(GLOBALPLATFORM REQUIRED globalplatform)

add_library(gpsocketconnectionplugin gpsocketconnectionplugin.c)

include_directories(${GLOBALPLATFORM_INCLUDE_DIRS})

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
SET(CMAKE_C_FLAGS_DEBUG "-g -O0")

target_link_libraries(gpsocketconnectionplugin ${GLOBALPLATFORM_LIBRARIES})
target_compile_options(gpsocketconnectionplugin PRIVATE ${GLOBALPLATFORM_CFLAGS_OTHER})
target_include_directories(gpsocketconnectionplugin PUBLIC ${PROJECT_SOURCE_DIR}/src)