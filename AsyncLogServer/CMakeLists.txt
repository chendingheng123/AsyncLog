cmake_minimum_required(VERSION 3.10)
project(AsyncLog VERSION 1.2.1 LANGUAGES CXX)
SET(LIBRARY_OUTPUT_PATH ../output/lib/)
set(HEADER
LogCore/Global.h
LogCore/LogCore.h
)

set(LogCore
LogCore/Global.cpp
)

set(Logger
Logger/AsyncLogger.h
Logger/AsyncLogger.cpp
Logger/AsyncLoggerImpl.h
Logger/AsyncLoggerImpl.cpp
)

set(LogTool
LogTool/LogAlgorithm.h
LogTool/LogAlgorithm.cpp
)

set(LogView
LogView/LogPreview.h
LogView/LogPreview.cpp
)

set(Test
Test/main.cpp
)


#生成PDB文件
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

#版本信息
if(MSVC)
	set(PROJECT_MINIUM_VERSION 1)
    set(RC "${CMAKE_BINARY_DIR}/rc/AsyncLog.rc")
    configure_file("${CMAKE_SOURCE_DIR}/rc/AsyncLog.rc.in" "${RC}")
endif()

#add_library(AsyncLog SHARED ${HEADER} ${LogCore} ${Logger} ${Test} ${LogView} ${LogTool})

#如果需要测试异步日志库的接口功能，将下面这行注释放开，并注释上面add_library这一行
add_executable(AsyncLog ${HEADER} ${LogCore} ${Logger} ${Test} ${LogView} ${LogTool})

target_include_directories(AsyncLog PRIVATE
./
../
)

source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Header" FILES ${HEADER})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${Logger})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${Test})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${LogView})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${LogCore})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${LogTool})
source_group(TREE ${CMAKE_SOURCE_DIR}/rc PREFIX "rc" FILES ${RC})