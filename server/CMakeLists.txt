cmake_minimum_required(VERSION 2.6)
project(sf_server CXX)

#add_definitions(-Wall -g -DUSE_FILE_DEBUG)
#add_definitions(-Wall -g -D_DEBUG)
##add_definitions(-Wall -g -pg)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME}
	src/cserver.cpp
	src/csensor_catalog.cpp
	src/cfilter_catalog.cpp
	src/cprocessor_catalog.cpp
	src/cdata_stream.cpp
	src/resource_str.cpp
	src/ctrim.cpp
	src/ccatalog.cpp
	src/main.cpp
)

target_link_libraries(${PROJECT_NAME} ${rpkgs_LDFLAGS})
set(CMAKE_CXX_FLAGS "-ldl -lpthread ${CMAKE_CXX_FLAGS}")


install(TARGETS ${PROJECT_NAME} DESTINATION bin)
