cmake_minimum_required(VERSION 3.10)
project(PRat VERSION 1.0)

include(ExternalProject)

if(POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()

#
# DaisySP dependency
#
ExternalProject_Add(DaisySP
    GIT_REPOSITORY    https://github.com/electro-smith/DaisySP.git
    CMAKE_ARGS -DCMAKE_CXX_STANDARD=14 -DCMAKE_CXX_STANDARD_REQUIRED=True -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9
    STEP_TARGETS download build
    EXCLUDE_FROM_ALL TRUE
)

ExternalProject_Get_Property(DaisySP SOURCE_DIR)
set(DAISYSP_SRC ${SOURCE_DIR})
set(DAISYSP_INC ${DAISYSP_SRC}/Source)

ExternalProject_Get_Property(DaisySP BINARY_DIR)
set(DAISYSP_LIB ${BINARY_DIR}/libDaisySP.a)

set(DAISYSP_STATIC ${DAISYSP_SRC}/build/libdaisysp.a)

#
# libDaisy dependency
#
ExternalProject_Add(libDaisy
    GIT_REPOSITORY    https://github.com/electro-smith/libDaisy.git
    STEP_TARGETS download
    EXCLUDE_FROM_ALL TRUE
)

ExternalProject_Get_Property(libDaisy SOURCE_DIR)
set(LIBDAISY_SRC ${SOURCE_DIR})
set(LIBDAISY_INC ${LIBDAISY_SRC})

set(LIBDAISY_STATIC ${LIBDAISY_INC}/build/libdaisy.a)

#
# RackSDK dependency
#
if(APPLE)
    ExternalProject_Add(RackSDK
        URL         https://vcvrack.com/downloads/Rack-SDK-2.0.6-mac.zip
        URL_HASH    SHA256=0024121a6ad56c16eb29c89d19dca225cb27b9193361e5e722a929687cdd4214
        STEP_TARGETS download
        EXCLUDE_FROM_ALL TRUE
    )
else()
    ExternalProject_Add(RackSDK
        URL         https://vcvrack.com/downloads/Rack-SDK-2.0.6-lin.zip
        URL_HASH    SHA256=c4391aff06ba2f232d31b5107cc0e2fa38ebdef42dd0e29c4ed172d649689b6e
        STEP_TARGETS download
        EXCLUDE_FROM_ALL TRUE
    )
endif()

ExternalProject_Get_Property(RackSDK SOURCE_DIR)
set(RACK_DIR ${SOURCE_DIR})

#
# PRat
#
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)

if(ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

add_definitions(-DUSE_DAISYSP)

set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

if(NOT DEPS_ONLY)
# compile PRat library
add_library(PRat STATIC ./src/prat/PRatDist.cpp)
target_include_directories(PRat PRIVATE "${SOURCE_DIR}/commons" ${DAISYSP_INC})
add_dependencies(PRat DaisySP-build)
set(PRAT_LIB ${PROJECT_BINARY_DIR}/libPRat.a)
set(NOISEGATE_LIB ${PROJECT_BINARY_DIR}/libNoiseGate.a)

add_library(NoiseGate STATIC ./src/noisegate/NoiseGate.cpp)
target_include_directories(NoiseGate PRIVATE "${SOURCE_DIR}/commons" ${DAISYSP_INC})
add_dependencies(NoiseGate DaisySP-build)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/")

# compile tests
add_executable(test EXCLUDE_FROM_ALL ${CMAKE_SOURCE_DIR}/src/test/test.cpp)
target_include_directories(test PRIVATE "${SOURCE_DIR}/commons" "${SOURCE_DIR}/prat" "${SOURCE_DIR}/noisegate" ${DAISYSP_INC})
target_link_libraries(test PRat NoiseGate ${DAISYSP_LIB})

# compile DaisySP firmware
add_custom_target(
    firmware
    DEPENDS ${LIBDAISY_STATIC} ${DAISYSP_STATIC}
    COMMAND ${CMAKE_COMMAND} -E env
    DAISYSP_DIR=\"${DAISYSP_SRC}\"
    LIBDAISY_DIR=\"${LIBDAISY_INC}\"
    make -C ${CMAKE_SOURCE_DIR}/src/patchinit
)

# compile DaisySP versio firmware
add_custom_target(
    firmware_versio
    DEPENDS ${LIBDAISY_STATIC} ${DAISYSP_STATIC}
    COMMAND ${CMAKE_COMMAND} -E env
    DAISYSP_DIR=\"${DAISYSP_SRC}\"
    LIBDAISY_DIR=\"${LIBDAISY_INC}\"
    make -C ${CMAKE_SOURCE_DIR}/src/versio
)

# compile DaisySP legio firmware
add_custom_target(
    firmware_legio
    DEPENDS ${LIBDAISY_STATIC} ${DAISYSP_STATIC}
    COMMAND ${CMAKE_COMMAND} -E env
    DAISYSP_DIR=\"${DAISYSP_SRC}\"
    LIBDAISY_DIR=\"${LIBDAISY_INC}\"
    make -C ${CMAKE_SOURCE_DIR}/src/legio
)

# flash DaisySP firmware
add_custom_target(
    flash
    DEPENDS firmware
    COMMAND ${CMAKE_COMMAND} -E env
    DAISYSP_DIR=\"${DAISYSP_SRC}\"
    LIBDAISY_DIR=\"${LIBDAISY_INC}\"
    make -C ${CMAKE_SOURCE_DIR}/src/patchinit program-dfu
)

# flash DaisySP versio firmware
add_custom_target(
    flash_versio
    DEPENDS firmware_versio
    COMMAND ${CMAKE_COMMAND} -E env
    DAISYSP_DIR=\"${DAISYSP_SRC}\"
    LIBDAISY_DIR=\"${LIBDAISY_INC}\"
    make -C ${CMAKE_SOURCE_DIR}/src/versio program-dfu
)

# flash DaisySP legio firmware
add_custom_target(
    flash_legio
    DEPENDS firmware_legio
    COMMAND ${CMAKE_COMMAND} -E env
    DAISYSP_DIR=\"${DAISYSP_SRC}\"
    LIBDAISY_DIR=\"${LIBDAISY_INC}\"
    make -C ${CMAKE_SOURCE_DIR}/src/legio program-dfu
)

# compile VCV Rack module
if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
add_custom_target(
    vcvrack ALL
    COMMAND ${CMAKE_COMMAND} -E env
    CXXFLAGS=\"-I${DAISYSP_INC} -I${SOURCE_DIR}/commons -I${SOURCE_DIR}/prat -I${SOURCE_DIR}/noisegate -DUSE_DAISYSP\"
    LDFLAGS=\"${PRAT_LIB} ${NOISEGATE_LIB} ${DAISYSP_LIB}\"
    RACK_DIR=\"${RACK_DIR}\"
    make -C ${CMAKE_SOURCE_DIR}/src/vcvrack
)
add_dependencies(vcvrack RackSDK-download PRat NoiseGate)
endif()

endif()

# compile libDaisy
add_custom_command(
    OUTPUT ${LIBDAISY_STATIC}
    DEPENDS libDaisy-download
    COMMAND ${CMAKE_COMMAND} -E env
    CPP_STANDARD=-std=gnu++14
    make -C ${LIBDAISY_INC}
)

# compile DaisySP (ARM)
add_custom_command(
    OUTPUT ${DAISYSP_STATIC}
    DEPENDS DaisySP-download
    COMMAND ${CMAKE_COMMAND} -E env
    CPP_STANDARD=-std=gnu++14
    make -C ${DAISYSP_SRC}
)

add_custom_target(
    deps
    DEPENDS ${LIBDAISY_STATIC} ${DAISYSP_STATIC} DaisySP-build RackSDK-download
)

# install VCV Rack module
if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E env RACK_DIR=${RACK_DIR} make -C ${CMAKE_SOURCE_DIR}/src/vcvrack -f Makefile install)")
else()
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E env RACK_DIR=${RACK_DIR} true)")
endif()
