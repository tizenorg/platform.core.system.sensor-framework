/*
 *  sensor-framework
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JuHyun Kim <jh8212.kim@samsung.com>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ 




#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>


#include <common.h>

#include <cobject_type.h>

#include <cmutex.h>
#include <clist.h>
#include <cmodule.h>
#include <csync.h>

#include <cworker.h>
#include <cipc_worker.h>
#include <csock.h>
#include <cpacket.h>
#include <sf_common.h>

#include <ccatalog.h>
#include <csensor_catalog.h>
#include <cfilter_catalog.h>
#include <cprocessor_catalog.h>

#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <cdata_stream.h>

#include <cserver.h>
#include <cutil.h>

#include <resource_str.h>


#if !defined(PATH_MAX)
#define PATH_MAX 256
#endif

extern char *optarg;
extern int optind, opterr, optopt;

static cserver server;

inline static int daemonize(void)
{
	pid_t pid;

	pid = fork();
	if (pid < 0)
		return -1;
	else if (pid != 0)
		exit(0);

	setsid();
	chdir("/");

	close(0);
	close(1);
	close(2);

	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	dup(1);

	return 0;
}


int main(int argc, char *argv[])
{
	csensor_catalog *sensor_catalog = NULL;
	cfilter_catalog *filter_catalog = NULL;
	cprocessor_catalog *processor_catalog = NULL;
	cdata_stream dstream;
	char dstream_file[PATH_MAX] = { 0, };
	int opt;

	if ( argc > 1) {
		while ((opt = getopt(argc, argv, "bs:f:p:d:h")) != -1) {
			DBG("input opt : %c , opterr : %d , optind : %d ,  optopt : %d\n",(char)opt , opterr , optind , optopt);
			if (opt == 'b' ) {
				{
				daemonize();
				}
			} else if (opt == 's' ) {
				if (sensor_catalog) {
					INFO("Sensors are already loaded\n");
					continue;
				}

				try {
					sensor_catalog = new csensor_catalog;
				} catch (...) {
					ERR("csensor_catalog class create fail\n");
					if (filter_catalog) delete filter_catalog;
					if (processor_catalog) delete processor_catalog;
					return -1;
				}

				DBG("Sensor : %s\n", optarg);
				if (sensor_catalog->create(optarg) == false) {
					ERR("sensor_catalog create fail\n");
					delete sensor_catalog;
					if (filter_catalog) delete filter_catalog;
					if (processor_catalog) delete processor_catalog;
					return -1;
				}
			} else if (opt == 'f' ) {
				if (filter_catalog) {
					INFO("Filters are already loaded\n");
					continue;
				}

				try {
					filter_catalog = new cfilter_catalog;
				} catch (...) {
					ERR("cfilter_catalog class create fail\n");
					if (sensor_catalog) delete sensor_catalog;
					if (processor_catalog) delete processor_catalog;
					return -1;
				}

				DBG("Filter : %s\n", optarg);
				if (filter_catalog->create(optarg) == false) {
					ERR("filter_catalog create fail\n");
					delete filter_catalog;
					if (sensor_catalog) delete sensor_catalog;
					if (processor_catalog) delete processor_catalog;
					return -1;
				}
			} else if (opt == 'p' ) {
				if (processor_catalog) {
					INFO("Processors are already loaded\n");
					continue;
				}

				try {
					processor_catalog = new cprocessor_catalog;
				} catch (...) {
					ERR("cprocessor_catalog class create fail\n");
					if (sensor_catalog) delete sensor_catalog;
					if (filter_catalog) delete filter_catalog;
					return -1;
				}

				DBG("Processor : %s\n", optarg);
				if (processor_catalog->create(optarg) == false) {
					ERR("processor_catalog create fail\n");
					delete processor_catalog;
					if (sensor_catalog) delete sensor_catalog;
					if (filter_catalog) delete filter_catalog;
					return -1;
				}

			} else if (opt == 'd' ) {
				if (strlen(dstream_file)) {
					INFO("Data stream is already loaded\n");
					continue;
				}
				DBG("Data stream file %s\n", optarg);
				if(  strlen(optarg) < PATH_MAX )
				{
					strncpy(dstream_file, optarg,strlen(optarg));
					dstream_file[strlen(optarg)] = '\0';
				}
				else {
					ERR("Data stream file name string too long\n");
					if (sensor_catalog) delete sensor_catalog;
					if (filter_catalog) delete filter_catalog;
					if (processor_catalog) delete processor_catalog;
					return -1;
				}


			} else if (opt == 'h' ) {
				printf("Daemon process of the sensor framework v0.1\n");
				printf(" -h) This screen\n");
				printf(" -s) Set the sensor catalog file\n");
				printf(" -f) Set the filter catalog file\n");
				printf(" -p) Set the processor catalog file\n");
				printf(" -d) Set the datastream catalog file\n");
				printf(" -b) Make a daemon\n");
				if (sensor_catalog) delete sensor_catalog;
				if (filter_catalog) delete filter_catalog;
				if (processor_catalog) delete processor_catalog;
				return 0;
			}
		} 
	}else {
		DBG("No option just run by default\n");

		daemonize();

		try {
			sensor_catalog = new csensor_catalog;
		} catch (...) {
			DbgPrint("csensor_catalog class create fail\n");
			if (filter_catalog) delete filter_catalog;
			if (processor_catalog) delete processor_catalog;
			return -1;
		}

		strncpy(dstream_file, "/usr/etc/sf_sensor.conf",strlen("/usr/etc/sf_sensor.conf"));
		dstream_file[strlen("/usr/etc/sf_sensor.conf")] = '\0';

		if (sensor_catalog->create(dstream_file) == false) {
			ERR("sensor_catalog create fail\n");
			delete sensor_catalog;
			if (filter_catalog) delete filter_catalog;
			if (processor_catalog) delete processor_catalog;
			return -1;
		}

		try {
			filter_catalog = new cfilter_catalog;
		} catch (...) {
			ERR("cfilter_catalog class create fail\n");
			if (sensor_catalog) delete sensor_catalog;
			if (processor_catalog) delete processor_catalog;
			return -1;
		}

		strncpy(dstream_file, "/usr/etc/sf_filter.conf",strlen("/usr/etc/sf_filter.conf"));
		dstream_file[strlen("/usr/etc/sf_filter.conf")] = '\0';

		if (filter_catalog->create(dstream_file) == false) {
			ERR("filter_catalog create fail\n");
			delete filter_catalog;
			if (sensor_catalog) delete sensor_catalog;
			if (processor_catalog) delete processor_catalog;
			return -1;
		}

		try {
			processor_catalog = new cprocessor_catalog;
		} catch (...) {
			ERR("cprocessor_catalog class create fail\n");
			if (sensor_catalog) delete sensor_catalog;
			if (filter_catalog) delete filter_catalog;
			return -1;
		}

		strncpy(dstream_file, "/usr/etc/sf_processor.conf",strlen("/usr/etc/sf_processor.conf"));
		dstream_file[strlen("/usr/etc/sf_processor.conf")] = '\0';

		if (processor_catalog->create(dstream_file) == false) {
			ERR("processor_catalog create fail\n");
			delete processor_catalog;
			if (sensor_catalog) delete sensor_catalog;
			if (filter_catalog) delete filter_catalog;
			return -1;
		}

		strncpy(dstream_file, "/usr/etc/sf_data_stream.conf",strlen("/usr/etc/sf_data_stream.conf"));
		dstream_file[strlen("/usr/etc/sf_data_stream.conf")] = '\0';
	}
	
	if (dstream.create(dstream_file) == false) {
		if (processor_catalog) {
			processor_catalog->destroy();
			delete processor_catalog;
		}

		if (filter_catalog) {
			filter_catalog->destroy();
			delete filter_catalog;
		}

		if (sensor_catalog) {
			sensor_catalog->destroy();
			delete sensor_catalog;
		}

		ERR("Failed to build a data stream, missing configurations?\n");
		return -1;
	}

	server.sf_main_loop();

	if (processor_catalog) {
		processor_catalog->destroy();
		delete processor_catalog;
	}

	if (filter_catalog) {
		filter_catalog->destroy();
		delete filter_catalog;
	}

	if (sensor_catalog) {
		sensor_catalog->destroy();
		delete sensor_catalog;
	}

	INFO("sf_server terminated\n");
	return 0;
}



//! End of a file
