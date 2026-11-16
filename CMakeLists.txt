cmake_minimum_required(VERSION 3.16)
project(KernelMem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Works with Qt 6 (preferred) or Qt 5.15
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    resources/icons.qrc
    src/Theme.cpp          src/Theme.h
    src/MemoryBar.cpp      src/MemoryBar.h
    src/AllocationTab.cpp  src/AllocationTab.h
    src/PageTab.cpp        src/PageTab.h
    src/GcTab.cpp          src/GcTab.h
    src/ProcessTab.cpp     src/ProcessTab.h
    src/MainWindow.cpp     src/MainWindow.h
    backend/Algorithms.h
    backend/AllocationAlgorithm.h
    backend/PageReplacementAlgorithm.h
    backend/MemoryManagementAlgorithm.h
    backend/ProcessManagementAlgorithm.h
)

add_executable(KernelMem ${SOURCES})
target_include_directories(KernelMem PRIVATE backend src)
target_link_libraries(KernelMem PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(WIN32)
    set_target_properties(KernelMem PROPERTIES WIN32_EXECUTABLE ON)
endif()
if(APPLE)
    set_target_properties(KernelMem PROPERTIES MACOSX_BUNDLE ON)
endif()
