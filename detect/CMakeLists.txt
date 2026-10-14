cmake_minimum_required(VERSION 3.10)
project(person_detect CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

option(HAVE_RKNN "Link Rockchip librknnrt" ON)
option(HAVE_CURL "Link libcurl for HTTP POST" OFF)

add_executable(person_detect
  src/main.cpp
  src/rknn_wrapper.cpp
  src/yolo_postprocess.cpp
  src/alarm_client.cpp
)

target_include_directories(person_detect PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(person_detect PRIVATE ${OpenCV_LIBS})

if(HAVE_RKNN)
  target_compile_definitions(person_detect PRIVATE HAVE_RKNN)
  # 板端常见路径；可 -DRKNN_LIB=... 覆盖
  if(NOT RKNN_LIB)
    set(RKNN_LIB rknnrt)
  endif()
  target_link_libraries(person_detect PRIVATE ${RKNN_LIB})
  # 若头文件不在默认路径：cmake -DRKNN_INCLUDE=/usr/include
  if(RKNN_INCLUDE)
    target_include_directories(person_detect PRIVATE ${RKNN_INCLUDE})
  endif()
endif()

if(HAVE_CURL)
  find_package(CURL REQUIRED)
  target_compile_definitions(person_detect PRIVATE HAVE_CURL)
  target_link_libraries(person_detect PRIVATE CURL::libcurl)
endif()

install(TARGETS person_detect RUNTIME DESTINATION bin)
