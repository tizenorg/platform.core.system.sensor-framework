cmake_minimum_required(VERSION 2.6)
project(sensor_framework_main CXX)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED sf_common vconf)
add_definitions(${rpkgs_CFLAGS})

# to install pkgconfig setup file.
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "2")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_definitions(-Wall -O3 -omit-frame-pointer) 
add_definitions(-Wall -g -D_DEBUG)

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" MATCHES "^arm.*")
        ADD_DEFINITIONS("-DTARGET")
        MESSAGE("add -DTARGET")
ELSE("${ARCH}" MATCHES "^arm.*")
        ADD_DEFINITIONS("-DSIMULATOR")
        MESSAGE("add -DSIMULATOR")
ENDIF("${ARCH}" MATCHES "^arm.*")

add_subdirectory(server)

IF("${ARCH}" MATCHES "^arm.*")
	CONFIGURE_FILE(sf_sensor.conf.in sf_sensor.conf @ONLY)
	CONFIGURE_FILE(sf_filter.conf.in sf_filter.conf @ONLY)
	CONFIGURE_FILE(sf_processor.conf.in sf_processor.conf @ONLY)
	CONFIGURE_FILE(sf_data_stream.conf.in sf_data_stream.conf @ONLY)
	install(FILES
		sf_sensor.conf
		sf_filter.conf
		sf_processor.conf
		sf_data_stream.conf
		DESTINATION etc)
ELSE("${ARCH}" MATCHES "^arm.*")
	CONFIGURE_FILE(sf_sensor_sim.conf.in sf_sensor.conf @ONLY)
	CONFIGURE_FILE(sf_filter_sim.conf.in sf_filter.conf @ONLY)
	CONFIGURE_FILE(sf_processor_sim.conf.in sf_processor.conf @ONLY)
	CONFIGURE_FILE(sf_data_stream_sim.conf.in sf_data_stream.conf @ONLY)
	install(FILES
		sf_sensor.conf
		sf_filter.conf
		sf_processor.conf
		sf_data_stream.conf
		DESTINATION etc)
ENDIF("${ARCH}" MATCHES "^arm.*")

install(PROGRAMS sfsvc DESTINATION /etc/rc.d/init.d)
