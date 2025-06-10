cmake_minimum_required(VERSION 3.10)

project(rknn_yolov8_demo)

if (ENABLE_ASAN)
	message(STATUS "BUILD WITH ADDRESS SANITIZER")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

if (TARGET_SOC STREQUAL "rv1106" OR TARGET_SOC STREQUAL "rv1103")
    add_definitions(-DRV1106_1103)
    set(rknpu_yolov8_file rknpu2/yolov8_rv1106_1103.cc)
    #dma
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/allocator/dma)
endif()

if(TARGET_SOC STREQUAL "rk1808" OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126")
    add_definitions(-DRKNPU1)
    set(rknpu_yolov8_file rknpu1/yolov8.cc)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/uclibc)

#Opencv 4
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/uclibc/lib/cmake/opencv4")
find_package(OpenCV REQUIRED)
#Thread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(${PROJECT_NAME}
    main.cc
    postprocess.cc
    luckfox_mpi.cc
    ${rknpu_yolov8_file}
)


add_compile_options(-g -Wall
                    -DISP_HW_V30 -DRKPLATFORM=ON -DARCH64=OFF
                    -DROCKIVA -DUAPI2
                    -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 
                    )

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,${CMAKE_CURRENT_SOURCE_DIR}/../../../lib:/usr/lib")

target_link_libraries(${PROJECT_NAME}
                    ${OpenCV_LIBS}
                    rknnmrt
                    Threads::Threads
                    rockiva
                    sample_comm
                    rockit
                    rockchip_mpp
                    rkaiq
                    pthread
                    rtsp
                    rga

                    imageutils
                    fileutils
                    imagedrawing    
                    ${LIBRKNNRT}
                    dl
                    )

message(STATUS "!!!!!!!!!!!CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE                        
                            ${OpenCV_INCLUDE_DIRS}
                            ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../common 
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/isp3.x   
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rknn
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/librga 
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rkaiq
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rkaiq/uAPI2
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rkaiq/common
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rkaiq/xcore
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rkaiq/algos
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rkaiq/iq_parser
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rkaiq/iq_parser_v2
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/rkaiq/smartIr

                            # ${CMAKE_CURRENT_SOURCE_DIR}
                            ${LIBRKNNRT_INCLUDES}
)


# Currently zero copy only supports rknpu2, v1103/rv1103b/rv1106 supports zero copy by default
if (NOT (TARGET_SOC STREQUAL "rv1106" OR TARGET_SOC STREQUAL "rv1103" OR TARGET_SOC STREQUAL "rk1808" 
    OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126" OR TARGET_SOC STREQUAL "rv1103b"))
    add_executable(${PROJECT_NAME}_zero_copy
        main.cc
        postprocess.cc
        rknpu2/yolov8_zero_copy.cc
    )

    target_compile_definitions(${PROJECT_NAME}_zero_copy PRIVATE ZERO_COPY)

    target_link_libraries(${PROJECT_NAME}_zero_copy
        imageutils
        fileutils
        imagedrawing    
        ${LIBRKNNRT}
        dl
    )

    if (CMAKE_SYSTEM_NAME STREQUAL "Android")
        target_link_libraries(${PROJECT_NAME}_zero_copy
        log
    )
    endif()

    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(THREADS_PREFER_PTHREAD_FLAG ON)
        find_package(Threads REQUIRED)
        target_link_libraries(${PROJECT_NAME}_zero_copy Threads::Threads)
    endif()

    target_include_directories(${PROJECT_NAME}_zero_copy PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBRKNNRT_INCLUDES}
    )
    install(TARGETS ${PROJECT_NAME}_zero_copy DESTINATION .)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/bus.jpg DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/coco_80_labels_list.txt DESTINATION model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION model)