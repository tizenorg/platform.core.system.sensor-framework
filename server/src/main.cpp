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

#define SF_SERVER_STATE_LOG_FILE        "/opt/share/debug/sf_server_log"
#define SF_SERVER_STATE_LOG_DIR     "/opt/share/debug"

static struct sigaction sf_act_quit;
static struct sigaction sf_oldact_quit;
static struct sigaction sf_act_pipe;
static struct sigaction sf_oldact_pipe;

static void sig_pipe_handler(int signo)
{
	int fd;
	char buf[255];
	time_t now_time;

	if(signo==SIGPIPE)
	{
		time(&now_time);

		if ( access (SF_SERVER_STATE_LOG_DIR, F_OK) <	0 ) {
			if(0 != mkdir(SF_SERVER_STATE_LOG_DIR, 0755))
				ERR("directory make fail");
		}

		fd = open(SF_SERVER_STATE_LOG_FILE,	O_WRONLY|O_CREAT|O_APPEND, 0755);

		if (fd != -1) {
			snprintf(buf,255, "\nsf_sever_sig_pipe_log now-time : %ld (s)\n\n",(long)now_time);
			write(fd, buf, strlen(buf));
			snprintf(buf,255, "sig_pipe event happend");
			write(fd, buf, strlen(buf));
		}
		if (fd != -1) {
			close(fd);
		}
	}

	return;
}

static void sig_quit_handler(int signo)
{
	int fd;
	char buf[255];
	time_t now_time;


	if( (signo==SIGTERM) || (signo==SIGQUIT) )
	{
		time(&now_time);
		if ( access (SF_SERVER_STATE_LOG_DIR, F_OK) < 0 ) {
			if(0 != mkdir(SF_SERVER_STATE_LOG_DIR, 0755))
				ERR("directory make fail");
		}

		fd = open(SF_SERVER_STATE_LOG_FILE,	O_WRONLY|O_CREAT|O_APPEND, 0755);

		if (fd != -1) {
			snprintf(buf,255, "\nsf_sever_sig_quit_term_log	now-time : %ld (s)\n\n",(long)now_time);
			write(fd, buf, strlen(buf));
		}
                if (fd != -1) {
                        close(fd);
                }
		
		server.sf_main_loop_stop();
		DBG("sf_main_loop_stop() called");
	}

	return;
}

static int sig_act_init(void)
{
	int return_state=0;

	sf_act_quit.sa_handler=sig_quit_handler;
	sigemptyset(&sf_act_quit.sa_mask);
	sf_act_quit.sa_flags = SA_NOCLDSTOP;

	sf_act_pipe.sa_handler=sig_pipe_handler;
	sigemptyset(&sf_act_pipe.sa_mask);
	sf_act_pipe.sa_flags = SA_NOCLDSTOP;


	if (sigaction(SIGTERM, &sf_act_quit, &sf_oldact_quit) < 0) {
		ERR("error sigaction register for SIGTERM");
		return_state = -1;
	}

	if (sigaction(SIGQUIT, &sf_act_quit, &sf_oldact_quit) < 0) {
		ERR("error sigaction register for SIGQUIT");
		return_state -= 1;
	}

	if (sigaction(SIGPIPE, &sf_act_pipe, &sf_oldact_pipe) < 0) {
		ERR("error sigaction register for SIGPIPE");
		return_state -= 1;
	}

	signal(SIGCHLD, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);

	return return_state;
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
			if (opt == 's' ) {
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

	
	if (sig_act_init() != 0) {
		ERR("sig_act_init fail!!");
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
