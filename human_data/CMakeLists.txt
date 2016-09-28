# Copyright: 2016 INRIA, Team LARSEN
# Author: Serena Ivaldi
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
# 

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(PROJECTNAME bodyPlayer)
PROJECT(${PROJECTNAME})

FIND_PACKAGE(YARP)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${YARP_MODULE_PATH})

include_directories(${YARP_INCLUDE_DIRS})
add_executable(bodyPlayer bodyPlayer.cpp)
target_link_libraries(bodyPlayer ${YARP_LIBRARIES})



