cmake_minimum_required(VERSION 3.2)

message(STATUS "Configuring core")

find_package(Threads)

file(GLOB TARGET_SRC
	"src/*.cpp"
)
source_group("Core::Sources" FILES ${TARGET_SRC})

file(GLOB TARGET_INC
	"include/*.h"
)
source_group("Core::Includes" FILES ${TARGET_INC})


# Create library
set(TARGET_NAME kacanopen_core)
add_library(${TARGET_NAME} SHARED ${TARGET_INC} ${TARGET_SRC})
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 14)
add_definitions(-DEXPORT_CORE_API)

#target_link_libraries(${TARGET_NAME} ${CAN_DRIVER})
target_link_libraries(${TARGET_NAME} ${DRIVER_LINK_NAME})
target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})

# Install
kacanopen_install(${TARGET_NAME} TRUE)
