cmake_minimum_required(VERSION 3.0)
project(TP-VC)

set(TP-VC C:/opencv/build)

# Find OpenCV package
find_package(OpenCV REQUIRED)
# Include directories from OpenCV
# include_directories(${OpenCV_INCLUDE_DIRS})

# Create an executable
add_executable(${PROJECT_NAME} src/main.cpp src/vc.c)

# Link OpenCV libraries
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)

