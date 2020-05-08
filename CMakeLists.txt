cmake_minimum_required(VERSION 3.10)
project(restbeast
    VERSION     0.0.1
    LANGUAGES   CXX
)

find_package(Boost      1.67    COMPONENTS system       REQUIRED)
find_package(OpenSSL    1.0.1   COMPONENTS SSL Crypto   REQUIRED)
find_package(Threads                                    REQUIRED)

add_subdirectory(cpprouter)

add_library(tamed INTERFACE)
add_library(tamed::tamed ALIAS tamed)

target_link_libraries(tamed INTERFACE router::router)
target_link_libraries(tamed INTERFACE Boost::system)
target_link_libraries(tamed INTERFACE OpenSSL::SSL)
target_link_libraries(tamed INTERFACE OpenSSL::Crypto)
target_link_libraries(tamed INTERFACE Threads::Threads)

target_include_directories(tamed INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set(TAMED_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(TAMED_MASTER_PROJECT ON)
endif()

option(TAMED_TEST "Build the tests" ${TAMED_MASTER_PROJECT})

install(
    DIRECTORY   include/tamed
    DESTINATION include
)

install(
    FILES       include/tamed.h
    DESTINATION include
)

install(
    TARGETS     tamed
    EXPORT      tamed-targets
    DESTINATION lib
)

install(
    EXPORT      tamed-targets
    NAMESPACE   tamed::
    DESTINATION lib/cmake/tamed
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/tamed/tamed-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

export(
    EXPORT      tamed-targets
    FILE        "${CMAKE_CURRENT_BINARY_DIR}/tamed/tamed-targets.cmake"
    NAMESPACE   tamed::
)

configure_file(cmake/tamed-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/tamed/tamed-config.cmake"
    COPYONLY
)

install(
    FILES
        cmake/tamed-config.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/tamed/tamed-config-version.cmake"
    DESTINATION
        lib/cmake/tamed
)

if (TAMED_TEST)
    add_subdirectory(tests)
endif()
