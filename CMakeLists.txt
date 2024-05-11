cmake_minimum_required(VERSION 3.0)
project(TP-VC)

# Find OpenCV package
find_package(OpenCV REQUIRED)
# Include directories from OpenCV
include_directories(${OpenCV_INCLUDE_DIRS})

# Create an executable
add_executable(TP-VC main.cpp vc.c)

# Link OpenCV libraries
target_link_libraries(TP-VC ${OpenCV_LIBS})

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)

