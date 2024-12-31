cmake_minimum_required(VERSION 3.0.2)
project(rosbag_time_filter)

## Find catkin and any catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rosbag
)

## Find Boost
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)

## Declare a catkin package
catkin_package(
  CATKIN_DEPENDS roscpp rosbag
)

## Build settings
include_directories(
  ${catkin_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

## Declare the executable
add_executable(time_filter src/time_filter.cpp)

## Specify libraries to link executable against
target_link_libraries(time_filter
  ${catkin_LIBRARIES}
  ${Boost_LIBRARIES}
)

## Install
install(TARGETS time_filter
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
