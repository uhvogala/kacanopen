cmake_minimum_required(VERSION 3.2)

message(STATUS "Configuring master")

file(GLOB TARGET_SRC
	"src/*.cpp"
)
source_group("Master::Sources" FILES ${TARGET_SRC})

file(GLOB TARGET_INC
	"include/*.h"
)
source_group("Master::Includes" FILES ${TARGET_INC})


# Create library
set(TARGET_NAME kacanopen_master)
add_library(${TARGET_NAME} SHARED ${TARGET_INC} ${TARGET_SRC})
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 14)
add_definitions(-DEXPORT_MASTER_API)

# Boost filesystem required! See ../CMakeLists.txt
target_link_libraries(${TARGET_NAME} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} kacanopen_core)

# Install
kacanopen_install(${TARGET_NAME} TRUE)
