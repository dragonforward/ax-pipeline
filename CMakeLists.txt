# check CMake version
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# disable inplace builds to prevent source tree corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "FATAL: Building inplace are not allowed. You should create a separate directory for Building.")
endif()

# set cmake_install_prefix path
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
endif()
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

# check if building type is not set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()


# default compile for ax620
if(NOT AXERA_TARGET_CHIP)
    set(AXERA_TARGET_CHIP "ax620a")
endif()
message(STATUS "AXERA_TARGET_CHIP = ${AXERA_TARGET_CHIP}")

# bsp
if(NOT BSP_MSP_DIR)
    set(BSP_MSP_DIR ${CMAKE_SOURCE_DIR}/axpi_bsp_sdk/msp/out)
endif()
message(STATUS "BSP_MSP_DIR = ${BSP_MSP_DIR}")

# option(BUILD_ON_BOARD "build in axpi" OFF)
option(SIPY_BUILD "build *.so for sipeed python package" OFF)
if(SIPY_BUILD)
    set(SIPY_SOVERSION 1)
    set(SIPY_VERSION ${SIPY_SOVERSION}.1.2)
endif()

# check toolchain about
message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")

project(ax_pipeline)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast -Wall -s -fPIC -Wno-unused-function -Wno-unused-label")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -Wall -s -fPIC -Wno-unused-function -Wno-unused-label")

add_subdirectory(third-party/RTSP)
add_subdirectory(third-party/RtspServer)    
add_subdirectory(third-party/ByteTrack)
add_subdirectory(third-party/mp4demux)

# 添加 FLAGS 检查代码是否有明显 bug
include(cmake/overlook.cmake)
# 编译的函数
include(cmake/build_func.cmake)

if(AXERA_TARGET_CHIP MATCHES "ax650" OR AXERA_TARGET_CHIP MATCHES "AX650")
    set(AXERA_TARGET_CHIP "AX650")
    message(STATUS "SET AXERA_TARGET_CHIP = ${AXERA_TARGET_CHIP}")
elseif(AXERA_TARGET_CHIP MATCHES "ax620e"
    OR AXERA_TARGET_CHIP MATCHES "AX620E"
    OR AXERA_TARGET_CHIP MATCHES "AX620e"
    OR AXERA_TARGET_CHIP MATCHES "AX620Q"
    OR AXERA_TARGET_CHIP MATCHES "AX620q"
    OR AXERA_TARGET_CHIP MATCHES "ax620q"
    OR AXERA_TARGET_CHIP MATCHES "AX630C"
    OR AXERA_TARGET_CHIP MATCHES "ax630C"
    OR AXERA_TARGET_CHIP MATCHES "ax630c"
    )
    set(AXERA_TARGET_CHIP "AX620E")
    message(STATUS "SET AXERA_TARGET_CHIP = ${AXERA_TARGET_CHIP}")
elseif(AXERA_TARGET_CHIP MATCHES "ax620a"
    OR AXERA_TARGET_CHIP MATCHES "AX620A"
    OR AXERA_TARGET_CHIP MATCHES "AX620a"
    OR AXERA_TARGET_CHIP MATCHES "ax620a"
    OR AXERA_TARGET_CHIP MATCHES "ax620u"
    OR AXERA_TARGET_CHIP MATCHES "AX620u"
    OR AXERA_TARGET_CHIP MATCHES "AX620U"
    )
    set(AXERA_TARGET_CHIP "AX620A")
    message(STATUS "SET AXERA_TARGET_CHIP = ${AXERA_TARGET_CHIP}")
endif()

if(AXERA_TARGET_CHIP MATCHES "AX650")
    add_compile_options(-DAXERA_TARGET_CHIP_AX650)
    add_compile_options(-UAXERA_TARGET_CHIP_AX620E)
    add_compile_options(-UAXERA_TARGET_CHIP_AX620)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=armv8.2-a")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=armv8.2-a")

    # opencv
    if(NOT OpenCV_DIR)
        set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/libopencv-4.5.5-aarch64/lib/cmake/opencv4)
    endif()
    message(STATUS "OpenCV_DIR = ${OpenCV_DIR}")
    find_package(OpenCV REQUIRED)
    
    add_subdirectory(examples/common/common_pipeline)
    add_subdirectory(examples/libaxdl)
    add_subdirectory(examples/sample_demux_ivps_npu_rtsp)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_hdmi_vo)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_multi_rtsp)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_multi_rtsp_hdmi_vo)
    add_subdirectory(examples/sample_demux_ivps_npu_rtsp_hdmi_vo)
    add_subdirectory(examples/sample_demux_ivps_npu_hdmi_vo)
    add_subdirectory(examples/sample_vin_ivps_npu_hdmi_vo)
    add_subdirectory(examples/sample_vin_ivps_npu_venc_rtsp)
    file(GLOB SAMPLE_RUN_JOINT_CONFIG_FILES "${CMAKE_SOURCE_DIR}/examples/libaxdl/config/ax650/*")
    install(FILES ${SAMPLE_RUN_JOINT_CONFIG_FILES} DESTINATION bin/config)
elseif(AXERA_TARGET_CHIP MATCHES "AX620E")
    add_compile_options(-DAXERA_TARGET_CHIP_AX620E)
    add_compile_options(-UAXERA_TARGET_CHIP_AX650)
    add_compile_options(-UAXERA_TARGET_CHIP_AX620)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=armv8.2-a")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=armv8.2-a")

    # opencv
    if(NOT OpenCV_DIR)
        set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/libopencv-4.5.5-aarch64/lib/cmake/opencv4)
    endif()
    message(STATUS "OpenCV_DIR = ${OpenCV_DIR}")
    find_package(OpenCV REQUIRED)
    
    add_subdirectory(examples/common/common_pipeline)
    add_subdirectory(examples/libaxdl)
    add_subdirectory(examples/sample_demux_ivps_npu_rtsp)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_multi_rtsp)
    add_subdirectory(examples/sample_vin_ivps_npu_venc_rtsp)
    file(GLOB SAMPLE_RUN_JOINT_CONFIG_FILES "${CMAKE_SOURCE_DIR}/examples/libaxdl/config/ax650/*")
    install(FILES ${SAMPLE_RUN_JOINT_CONFIG_FILES} DESTINATION bin/config)

elseif(AXERA_TARGET_CHIP MATCHES "AX620A")
    add_compile_options(-DAXERA_TARGET_CHIP_AX620)
    add_compile_options(-UAXERA_TARGET_CHIP_AX620E)
    add_compile_options(-UAXERA_TARGET_CHIP_AX650)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=armv7-a -mfloat-abi=hard -mfpu=neon")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=armv7-a -mfloat-abi=hard -mfpu=neon")

    # opencv
    if(NOT OpenCV_DIR)
        set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/opencv-arm-linux/lib/cmake/opencv4)
    endif()
    message(STATUS "OpenCV_DIR = ${OpenCV_DIR}")
    find_package(OpenCV REQUIRED)

    add_subdirectory(examples/common/common_pipeline)
    add_subdirectory(examples/libaxdl)
    # deprecated
    # add_subdirectory(examples/sample_run_joint)
    # add_subdirectory(examples/sample_vin_joint)
    # add_subdirectory(examples/sample_vin_ivps_joint_vo_old)
    # add_subdirectory(examples/sample_vin_ivps_joint_venc_rtsp_old)
    add_subdirectory(examples/sample_vin_ivps_npu_venc_rtsp_vo)
    add_subdirectory(examples/sample_vin_ivps_npu_venc_rtsp)
    add_subdirectory(examples/sample_vin_ivps_npu_vo)
    add_subdirectory(examples/sample_vin_ivps_npu_vo_h265)
    add_subdirectory(examples/sample_v4l2_ivps_npu_vo)
    add_subdirectory(examples/sample_v4l2_user_ivps_npu_vo)
    # add_subdirectory(examples/sample_h264_ivps_joint_vo)
    # add_subdirectory(examples/sample_rtsp_ivps_joint_vo)
    # add_subdirectory(examples/sample_rtsp_ivps_joint_rtsp)
    add_subdirectory(examples/sample_demux_ivps_npu_vo)
    add_subdirectory(examples/sample_demux_ivps_npu_rtsp)
    add_subdirectory(examples/sample_demux_ivps_npu_rtsp_vo)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_multi_rtsp)

    file(GLOB SAMPLE_RUN_JOINT_CONFIG_FILES "${CMAKE_SOURCE_DIR}/examples/libaxdl/config/ax620/*")
    install(FILES ${SAMPLE_RUN_JOINT_CONFIG_FILES} DESTINATION bin/config)
endif()
