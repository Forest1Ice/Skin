cmake_minimum_required (VERSION 3.0 FATAL_ERROR)

project ("Skin")

# start MOC for QT use
set(CMAKE_AUTOMOC ON)

# set project paths
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(UTILITY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utils)

# set dependency paths
set(OCC_PATH "D:/Tools/CAD/OCC/install/OpenCASCADE-7.4.0-vc14-64/opencascade-7.4.0/cmake")
set(EIGEN_PATH "D:/Tools/Eigen/eigen")
set(QT_PATH "D:/Tools/Qt/6.7.2/msvc2019_64")

list(APPEND CMAKE_PREFIX_PATH "${OCC_PATH};${QT_PATH}")

# OCC package
find_package(OpenCASCADE CONFIG REQUIRED)
# Eigen package
include_directories(${EIGEN_PATH})
# Qt package
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# set groups
file(GLOB header_h ${INCLUDE_DIR}/*.h)
file(GLOB source_cpp ${SRC_DIR}/*.cpp)
file(GLOB utility ${UTILITY_DIR}/*)
source_group(includes FILES ${header_h})
source_group(src FILES ${source_cpp})
source_group(utils FILES ${utility})

include_directories(${INCLUDE_DIR})
include_directories(${SRC_DIR})
include_directories(${UTILITY_DIR})

add_executable(${PROJECT_NAME}
    ${header_h}
    ${source_cpp}
    ${utility}
    )

target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Widgets_INCLUDE_DIRS})

foreach(LIB ${OpenCASCADE_LIBRARIES})
     target_link_libraries(${PROJECT_NAME} PRIVATE debug      ${OpenCASCADE_LIBRARY_DIR}d/${LIB}.lib) 
     target_link_libraries(${PROJECT_NAME} PRIVATE optimized  ${OpenCASCADE_LIBRARY_DIR}/${LIB}.lib)
endforeach()
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

set(PATH_LIST
    "$<$<CONFIG:DEBUG>:${OpenCASCADE_BINARY_DIR}d>$<$<NOT:$<CONFIG:DEBUG>>:${OpenCASCADE_BINARY_DIR}>"
)
string(REPLACE ";" "\\;" PATH_STRING "${PATH_LIST}")
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_ENVIRONMENT "PATH=${PATH_STRING};%PATH%")
