cmake_minimum_required(VERSION 3.10)
project(AsyncLogServer VERSION 1.2.1 LANGUAGES CXX)
SET(LIBRARY_OUTPUT_PATH ../output/lib/)

if(UNIX AND NOT APPLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") # 保留完整调试信息（-g）
set(CMAKE_BUILD_TYPE Debug) #关闭优化防止符号剥离（-O0）
elseif(WIN32)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2") #禁用优化
endif()


set(LogCore
LogCore/LogCore.h
LogCore/Global.h
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

if(WIN32)
#生成PDB文件
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

#版本信息
if(MSVC)
	set(PROJECT_MINIUM_VERSION 1)
    set(RC "${CMAKE_BINARY_DIR}/rc/AsyncLogServer.rc")
    configure_file("${CMAKE_SOURCE_DIR}/rc/AsyncLogServer.rc.in" "${RC}")
endif()

#add_library(AsyncLogServer SHARED ${LogCore} ${Logger} ${LogView} ${LogTool} ${Test})

#如果需要验证日志库的能力，可以放开下面的注释，并将上面一行给注释掉，重新生成测试Demo即可
add_executable(AsyncLogServer ${LogCore} ${Logger} ${LogView} ${LogTool} ${Test})

target_include_directories(AsyncLogServer PRIVATE
./
../
)

source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${LogCore})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${Logger})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${LogView})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${LogTool})
source_group(TREE ${CMAKE_SOURCE_DIR}/ PREFIX "Source" FILES ${Test})
source_group(TREE ${CMAKE_SOURCE_DIR}/rc PREFIX "rc" FILES ${RC})