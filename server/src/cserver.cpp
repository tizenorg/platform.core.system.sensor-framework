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





#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <time.h>

#include <common.h>

#include <cobject_type.h>

#include <csync.h>

#include <clist.h>
#include <cmutex.h>

#include <cworker.h>
#include <cipc_worker.h>
#include <csock.h>

#include <cmodule.h>
#include <cpacket.h>
#include <sf_common.h>

#include <ccatalog.h>

#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <cdata_stream.h>

#include <cserver.h>
#include <resource_str.h>

#include <glib.h>
#include <vconf.h>
#include <heynoti.h>

#define POWEROFF_NOTI_NAME "power_off_start"
#define SF_POWEROFF_VCONF "memory/private/sensor/poweroff"

static GMainLoop *g_mainloop;
static int g_poweroff_fd = -1;

static void system_poweroff_cb(void *data)
{
	vconf_set_int(SF_POWEROFF_VCONF, 1);
	heynoti_close(g_poweroff_fd);
	g_poweroff_fd = -1;
	DBG("system_poweroff_cb called");
}

cserver::cserver()
{
	memset(m_cmd_handler, '\0', sizeof(cmd_func_t) * CMD_LAST);
	m_cmd_handler[CMD_HELLO]	= cmd_hello;
	m_cmd_handler[CMD_BYEBYE]	= cmd_byebye;
	m_cmd_handler[CMD_GET_VALUE]	= cmd_get_value;
	m_cmd_handler[CMD_START]	= cmd_start;
	m_cmd_handler[CMD_STOP]		= cmd_stop;
	m_cmd_handler[CMD_REG]		= cmd_register_event;
	m_cmd_handler[CMD_SET_VALUE]		= cmd_set_value;
	m_cmd_handler[CMD_GET_PROPERTY]		= cmd_get_property;
	m_cmd_handler[CMD_GET_STRUCT]		= cmd_get_struct;
}



cserver::~cserver()
{
}

void *cserver::cmd_hello(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx;
	cmd_hello_t *payload;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	cmd_done_t *return_payload;
	cfilter_module *filter;
	csensor_module *sensor;
	cdata_stream *stream;
	long value = -1;

	DBG("CMD_HELLO Handler invoked\n");
	ctx = (client_ctx_t*)arg->client_ctx;
	payload = (cmd_hello_t*)queue_item->packet->data();

	DBG("Hello %d, %s\n", payload->id, payload->name);

	if (payload->id != ID_UNKNOWN) {
		ctx->module = (ctype*)cmodule::search_module(payload->id);
		if (!ctx->module) 
			ctx->module = (ctype*)cdata_stream::search_stream(payload->id);
		 		
	} else {
		ctx->module = (ctype*)cmodule::search_module(payload->name);
		if (!ctx->module) 
			ctx->module = (ctype*)cdata_stream::search_stream(payload->name);
	}

	if (!ctx->module) {
		ERR("There is no proper module found (%s)\n", __FUNCTION__ );		
		value = -1;
	} else {	
		DBG("ctx->module->type() = [%d]\n");
		switch (ctx->module->type()) {
			case csensor_module::SF_PLUGIN_SENSOR:
				sensor = (csensor_module*)ctx->module;
				value = sensor->get_sensor_type();
				break;
				
			case cdata_stream::SF_DATA_STREAM:
				stream = (cdata_stream *)ctx->module;
				filter = stream->my_filter();
				if (filter) {
					value = filter->get_sensor_type();
				} else {
					value = ID_UNKNOWN;
				}
				break;
				
			default :
				value = ID_UNKNOWN;
				break;
				
		}

	}

	ctx->client_state = 0;

	packet->set_cmd(CMD_DONE);
	packet->set_payload_size(sizeof(cmd_done_t));
	return_payload = (cmd_done_t*)packet->data();
	
	if ( !return_payload ) {		
		ERR("Error return_payload ptr check (%s) , retry to send using dummy packet\n", __FUNCTION__);
		cpacket dummy_packet(sizeof(cmd_done_t)+4);
		cmd_done_t *dummy_payload;
		dummy_packet.set_cmd(CMD_DONE);
		dummy_packet.set_payload_size(sizeof(cmd_done_t));
		dummy_payload = (cmd_done_t*)dummy_packet.data();
		if ( !dummy_payload ) {
			ERR("Error second dummy_payload ptr check");
			return (void *)NULL;
		}

		dummy_payload->value = value;
		
		if (ipc.send( dummy_packet.packet(), dummy_packet.size()) == false) {
			ERR("Failed to send a dummy value packet (%s)\n",__FUNCTION__);
			return (void *)NULL;
		}
		
		return (void *)NULL;
	}
	return_payload->value = value;

	DBG("Send value %ld\n", value);
	if (ipc.send(queue_item->packet->packet(), queue_item->packet->size()) == false) {
		ERR("Failed to send a value packet (%s)\n",__FUNCTION__);
		return (void *)NULL;
	}

	return (void *)NULL;
}



void *cserver::cmd_byebye(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx;
	cprocessor_module *processor;
	cfilter_module *filter;
	csensor_module *sensor;
	cdata_stream *stream;
	cpacket *packet = queue_item->packet;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	ipc.set_on_close(false);

	ctx = (client_ctx_t*)arg->client_ctx;

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	DBG("CMD_BYEBYE Handler invoked ctx->client_state = %d\n",ctx->client_state);

	if (ctx->client_state > 0) {
		if ( !ctx->module ) {
			ERR("Error ctx->module ptr check");
			goto out;			
		}
		switch (ctx->module->type()) {
			case csensor_module::SF_PLUGIN_SENSOR:
				sensor = (csensor_module*)ctx->module;
				DBG("Server does not received cmd_stop before detach!! forced stop, sensor_type : %d\n", sensor->get_sensor_type());
				sensor->stop();
				ctx->client_state --;			
				break;
				
			case cprocessor_module::SF_PLUGIN_PROCESSOR:
				processor = (cprocessor_module*)ctx->module;		
				DBG("Server does not received cmd_stop before detach!! forced stop, processor\n" );
				processor->stop();
				processor->wakeup_all_client();
				ctx->client_state -- ;
				break;
				
			case cfilter_module::SF_PLUGIN_FILTER:
				filter = (cfilter_module *)ctx->module;
				DBG("Server does not received cmd_stop before detach!! forced stop, filter_type : %d\n", filter->get_sensor_type());
				filter->stop();
				ctx->client_state -- ;	
				break;
			
			case cdata_stream::SF_DATA_STREAM:
				stream = (cdata_stream*)ctx->module;
				DBG("doesnot recevied cmd_stop before detach!! forced stop, stream_name : %s stop\n", stream->name());
				stream->stop();
				stream->wakeup_all_client();
				ctx->client_state -- ;
				break;
			
			default:
				ERR("Unknown type (%s)\n",__FUNCTION__);
				break;
		}
	}

out:
	
	packet->set_cmd(CMD_DONE);

	if (ipc.send(packet->packet(), packet->size()) == false) {
		ERR("Failed to send reply packet (%s)\n", __FUNCTION__);
		return (void*)NULL;
	}

	arg->worker->stop();
	
	return (void *)NULL;
}



void *cserver::cmd_start(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cpacket *packet = queue_item->packet;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	cmd_done_t *return_payload;
	long value = 0;

	ipc.set_on_close(false);
	
	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		value = -1;
		goto out;
	}

	DBG("CMD_START Handler invoked ctx->client_state = %d\n", ctx->client_state);
	
	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		value = -1;
		goto out;
	}
	
	if (ctx->module->type() == cprocessor_module::SF_PLUGIN_PROCESSOR) {
		cprocessor_module *processor = (cprocessor_module*)ctx->module;
		DBG("Invoke Processor start\n");
		if (ctx->client_state > 0) {
			ERR("This module has already started\n");
		}
		else {
			if ( processor->start() )
				ctx->client_state ++ ;
			else
				value = -3;
		}
	} else if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *stream = (cdata_stream*)ctx->module;
		DBG("Invoke Stream start for [%s]\n",stream->name());
		if (ctx->client_state > 0) {
			ERR("This module has already started\n");
		}
		else {
			if ( stream->start())
				ctx->client_state++ ;
			else
				value = -3;
		}
	} else if (ctx->module->type() == csensor_module::SF_PLUGIN_SENSOR) {
		csensor_module *sensor = (csensor_module*)ctx->module;
		DBG("Invoke Sensor start\n");
		if (ctx->client_state > 0) {
			ERR("This module has already started\n");
		}
		else {
			if ( sensor->start() )
				ctx->client_state ++ ;
			else
				value = -3;
		}
	} else if (ctx->module->type() == cfilter_module::SF_PLUGIN_FILTER) {
		cfilter_module *filter = (cfilter_module*)ctx->module;
		DBG("Invoke Filter start\n");
		if (ctx->client_state > 0) {
			ERR("This module has already started\n");
		}
		else {
			if ( filter->start() )
				ctx->client_state ++ ;
			else
				value = -3;
		}
	} else {
		ERR("This module has no start operation (%s)\n",__FUNCTION__);
	}

out:

	packet->set_cmd(CMD_DONE);
	packet->set_payload_size(sizeof(cmd_done_t));
	return_payload = (cmd_done_t*)packet->data();

	if ( !return_payload ) {
		ERR("Error return_payload ptr check (%s) , retry to send using dummy packet\n", __FUNCTION__);
		cpacket dummy_packet(sizeof(cmd_done_t)+4);
		cmd_done_t *dummy_payload;
		dummy_packet.set_cmd(CMD_DONE);
		dummy_packet.set_payload_size(sizeof(cmd_done_t));
		dummy_payload = (cmd_done_t*)dummy_packet.data();
		if ( !dummy_payload ) {
			ERR("Error second dummy_payload ptr check");
			return (void *)NULL;
		}
		dummy_payload->value = value;

		if (ipc.send( dummy_packet.packet(),  dummy_packet.size()) == false) 
			ERR("Failed to send a dummy value packet (%s)\n",__FUNCTION__);
		
		return (void *)NULL;
	}

	return_payload->value = value;
		
	DBG("Send value %ld\n", value);
	
	if (ipc.send(queue_item->packet->packet(), queue_item->packet->size()) == false) 
	    ERR("Failed to send a value packet (%s)\n",__FUNCTION__);
	
	return (void*)NULL;
}





void *cserver::cmd_stop(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	ipc.set_on_close(false);
		
	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	DBG("CMD_STOP Handler invoked ctx->client_state = %d\n", ctx->client_state);

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		return (void*)NULL;
	}
	
	if (ctx->module->type() == cprocessor_module::SF_PLUGIN_PROCESSOR) {
		cprocessor_module *processor = (cprocessor_module*)ctx->module;
		//! Before stopping processor, we need to wakeup the clients
		DBG("Invoke Processor stop\n");
		if (ctx->client_state < 1) {
			ERR("This module has already stopped\n");
		}
		else {
			if (processor->stop() ) {
				processor->wakeup_all_client();
				ctx->client_state --;
			}
		}
	} else if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *stream = (cdata_stream*)ctx->module;
		//! Before stopping stream, we need to wakeup the clients
		DBG("Invoke Stream stop\n");
		if (ctx->client_state < 1) {
			ERR("This module has already stopped\n");
		}
		else {
			DBG("Now stop %s \n", stream->name());
			if (stream->stop() ) {
				stream->wakeup_all_client();
				ctx->client_state --;
			}
		}
	} else if (ctx->module->type() == csensor_module::SF_PLUGIN_SENSOR) {
		csensor_module *sensor = (csensor_module*)ctx->module;
		DBG("Invoke Sensor stop\n");
		if (ctx->client_state < 1) {
			ERR("This module has already stopped\n");
		}
		else {
			if (sensor->stop() ) {				
				ctx->client_state --;
			}
		}
	} else if (ctx->module->type() == cfilter_module::SF_PLUGIN_FILTER) {
		cfilter_module *filter = (cfilter_module*)ctx->module;
		DBG("Invoke Filter stop\n");
		if (ctx->client_state < 1) {
			ERR("This module has already stopped\n");
		}
		else {
			if (filter->stop() ) {				
				ctx->client_state --;
			}
		}
	} else {
		ERR("This module has no stop operation (%s)\n",__FUNCTION__);
	}

	return (void*)NULL;
}



void *cserver::cmd_get_value(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_get_value_t *payload;
	cmd_done_t *return_payload;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	long value = -1;

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	DBG("CMD_GET_VALUE Handler invoked\n");

	payload = (cmd_get_value_t*)packet->data();
	
	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		goto out;
	}

	if (ctx->module->type() == cfilter_module::SF_PLUGIN_FILTER) {
		if(payload->id != -1 ) {
			value = ((cfilter_module*)ctx->module)->value(payload->id);
		} else {
			value = ((cfilter_module*)ctx->module)->value(payload->port);
		}
	} else if (ctx->module->type() == csensor_module::SF_PLUGIN_SENSOR) {
		if(payload->id != -1 ) {
			value = ((csensor_module*)ctx->module)->value(payload->id);
		} else {
			value = ((csensor_module*)ctx->module)->value(payload->port);
		}
		
	} else if (ctx->module->type() == cprocessor_module::SF_PLUGIN_PROCESSOR) {
		if(payload->id != -1 ) {
			value = ((cprocessor_module*)ctx->module)->value(payload->id);
		} else {
			value = ((cprocessor_module*)ctx->module)->value(payload->port);
		}
	} else if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		if(payload->id != -1 ) {
			value = ((cdata_stream*)ctx->module)->value(cdata_stream::PROCESSOR, payload->id);
		} else {
			value = ((cdata_stream*)ctx->module)->value(cdata_stream::PROCESSOR, payload->port);
		}
	}

out:

	packet->set_cmd(CMD_DONE);
	packet->set_payload_size(sizeof(cmd_done_t));
	return_payload = (cmd_done_t*)packet->data();

	if ( !return_payload ) {		
		ERR("Error return_payload ptr check (%s) , retry to send using dummy packet\n", __FUNCTION__);
		cpacket dummy_packet(sizeof(cmd_done_t)+4);
		cmd_done_t *dummy_payload;
		dummy_packet.set_cmd(CMD_DONE);
		dummy_packet.set_payload_size(sizeof(cmd_done_t));
		dummy_payload = (cmd_done_t*)dummy_packet.data();
		if ( !dummy_payload ) {
			ERR("Error second dummy_payload ptr check");
			return (void *)NULL;
		}
		dummy_payload->value = value;
		
		if (ipc.send( dummy_packet.packet(), dummy_packet.size()) == false) {
			ERR("Failed to send a dummy value packet (%s)\n",__FUNCTION__);
			return (void *)NULL;
		}
		
		return (void *)NULL;
	}

	return_payload->value = value;

	DBG("Send value %ld\n", value);
	if (ipc.send(queue_item->packet->packet(), queue_item->packet->size()) == false) {
		ERR("Failed to send a value packet (%s)\n",__FUNCTION__);
		return (void *)NULL;
	}

	return (void *)NULL;
}

void *cserver::cmd_register_event(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_reg_t *payload;
	cmd_done_t *return_payload;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	long value = -1;

	DBG("CMD_REG Handler invoked\n");

	payload = (cmd_reg_t*)packet->data();

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		goto out;
	}

	DBG("ctx->module->type() = [%d] cdata_stream::SF_DATA_STREAM = [%d]\n", ctx->module->type(),cdata_stream::SF_DATA_STREAM);
	if ( ctx->module->type() == cdata_stream::SF_DATA_STREAM ) {
		cdata_stream *data_stream;
		data_stream = (cdata_stream*)ctx->module;
		
		switch( payload->type ){
			case REG_ADD:
				DBG("CMD_REG_ADD callback func");
				if ( data_stream->add_callback_func(payload) ) {					
					ctx->reg_state = (ctx->reg_state) | (payload->event_type) ;
					DBG("REG_ADD current(final) reg_state : %x , payload_evet_type : %x",ctx->reg_state , payload->event_type);					
					value = REG_DONE;
				}
				break;
			case REG_DEL:
				DBG("CMD_REG_DEL callback func");
				if ( data_stream->remove_callback_func(payload) ) {
					ctx->reg_state = (ctx->reg_state) ^ (payload->event_type & 0xFFFF) ;
					DBG("REG_DEL current(final) reg_state : %x , payload_evet_type : %x",ctx->reg_state , payload->event_type);
					value = REG_DONE;
				}
				break;
				
			case REG_CHK:
				DBG("CMD_REG_CHK callback func");
				if ( data_stream->check_callback_event(payload) ) {
					value = REG_DONE;
				}				
				break;
				
			default:
				ERR("unsupported reg_cmd type : %d\n" , payload->type );
				break;
		}
	} else {

		ERR("Not supported module type (%s)\n",__FUNCTION__);

	}
	
out:

	packet->set_cmd(CMD_DONE);
	packet->set_payload_size(sizeof(cmd_done_t));
	return_payload = (cmd_done_t*)packet->data();

	if ( !return_payload ) {		
		ERR("Error return_payload ptr check (%s) , retry to send using dummy packet\n", __FUNCTION__);
		cpacket dummy_packet(sizeof(cmd_done_t)+4);
		cmd_done_t *dummy_payload;
		dummy_packet.set_cmd(CMD_DONE);
		dummy_packet.set_payload_size(sizeof(cmd_done_t));
		dummy_payload = (cmd_done_t*)dummy_packet.data();
		if ( !dummy_payload ) {
			ERR("Error second dummy_payload ptr check");
			return (void *)NULL;
		}
		dummy_payload->value = value;
		
		if (ipc.send( dummy_packet.packet(), dummy_packet.size()) == false) {
			ERR("Failed to send a dummy value packet (%s)\n",__FUNCTION__);
			return (void *)NULL;
		}
		
		return (void *)NULL;
	}

	return_payload->value = value;

	DBG("Send value %ld\n", value);
	if (ipc.send(queue_item->packet->packet(), queue_item->packet->size()) == false) {
		ERR("Failed to send a value packet (%s)\n",__FUNCTION__);
		return (void *)NULL;
	}

	return (void *)NULL;	
}

void *cserver::cmd_set_value(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_set_value_t *payload;
	cmd_done_t *return_payload;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	long value = -1;

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	DBG("CMD_SET_VALUE  Handler invoked\n");

	payload = (cmd_set_value_t*)packet->data();

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		goto out;
	}

	if (ctx->module->type() == cprocessor_module::SF_PLUGIN_PROCESSOR) {
		cprocessor_module *processor;
		processor = (cprocessor_module*)ctx->module;
		value = processor->set_cmd(payload->sensor_type, payload->property, payload->value);
					
	} else if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *data_stream;
		data_stream = (cdata_stream*)ctx->module;
		value = data_stream->set_cmd(payload->sensor_type, payload->property, payload->value);

	}

out:
	packet->set_cmd(CMD_DONE);
	packet->set_payload_size(sizeof(cmd_done_t));
	return_payload = (cmd_done_t*)packet->data();
	if ( !return_payload ) {		
		ERR("Error return_payload ptr check (%s) , retry to send using dummy packet\n", __FUNCTION__);
		cpacket dummy_packet(sizeof(cmd_done_t)+4);
		cmd_done_t *dummy_payload;
		dummy_packet.set_cmd(CMD_DONE);
		dummy_packet.set_payload_size(sizeof(cmd_done_t));
		dummy_payload = (cmd_done_t*)dummy_packet.data();
		if ( !dummy_payload ) {
			ERR("Error second dummy_payload ptr check");
			return (void *)NULL;
		}
		dummy_payload->value = value;
		
		if (ipc.send( dummy_packet.packet(), dummy_packet.size()) == false) {
			ERR("Failed to send a dummy value packet (%s)\n",__FUNCTION__);
			return (void *)NULL;
		}
		
		return (void *)NULL;
	}
	return_payload->value = value;

	DBG("Send value %ld\n", value);
	if (ipc.send(queue_item->packet->packet(), queue_item->packet->size()) == false) {
		ERR("Failed to send a value packet (%s)\n",__FUNCTION__);
		return (void *)NULL;
	}

	return (void *)NULL;
}

void *cserver::cmd_get_property(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_get_property_t *payload;
	cmd_return_property_t *return_payload;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	
	int state = -1;
	unsigned int get_property_level = 0;
	size_t get_property_size = 0;
	void *return_property_struct = NULL;
		
	base_property_struct return_base_property;

	memset(&return_base_property, '\0', sizeof(return_base_property));

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	DBG("CMD_GET_PROPERTY Handler invoked\n");

	payload = (cmd_get_property_t*)packet->data();
	get_property_level =  payload->get_level;

	
	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		goto out;
	}
			
	if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *data_stream;
		data_stream = (cdata_stream*)ctx->module;

		get_property_size = sizeof(base_property_struct);
		state = data_stream->get_property(get_property_level, (void*)&return_base_property);
		if ( state != 0 ) {
			ERR("data_stream get_property fail");
			goto out;
		}
		return_property_struct = (void*)&return_base_property;
	}

out:

	packet->set_cmd(CMD_GET_PROPERTY);
	packet->set_payload_size(sizeof(cmd_return_property_t) + get_property_size +  4);
	return_payload = (cmd_return_property_t*)packet->data();
	
	if ( !return_payload ) {		
		ERR("Error return_payload ptr check (%s) , retry to send using dummy packet\n", __FUNCTION__);
		cpacket dummy_packet(sizeof(cmd_return_property_t)+4);
		cmd_return_property_t *dummy_payload;
		dummy_packet.set_cmd(CMD_GET_PROPERTY);
		dummy_packet.set_payload_size(sizeof(cmd_return_property_t));
		dummy_payload = (cmd_return_property_t*)dummy_packet.data();
		if ( !dummy_payload ) {
			ERR("Error second dummy_payload ptr check");
			return (void *)NULL;
		}
		dummy_payload->state = -1;
		
		if (ipc.send( dummy_packet.packet(), dummy_packet.size()) == false) {
			ERR("Failed to send a dummy value packet (%s)\n",__FUNCTION__);
			return (void *)NULL;
		}
		
		return (void *)NULL;
	}

	
	return_payload->state = state;
	return_payload->property_struct_size = get_property_size;
	if ( return_property_struct && (get_property_size!=0) ) {
		memcpy(return_payload->property_struct , return_property_struct , get_property_size );
	}
	

	DBG("Send get_property state %d\n", state);
	if (ipc.send(queue_item->packet->packet(), queue_item->packet->size()) == false) {
		ERR("Failed to send a value packet (%s)\n",__FUNCTION__);
		return (void *)NULL;
	}

	return (void *)NULL;
}

void *cserver::cmd_get_struct(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_get_data_t *payload;
	cmd_get_struct_t *return_payload;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	int state = -1;

	unsigned int struct_data_id = 0;
	size_t struct_data_size = 0;	
	void *return_struct_data = NULL;

	base_data_struct base_return_data;

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	DBG("CMD_GET_VALUE Handler invoked\n");

	payload = (cmd_get_data_t*)packet->data();
	struct_data_id = payload->data_id;	

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		goto out;
	}

	if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *data_stream;
		data_stream = (cdata_stream*)ctx->module;
		if ( ( (struct_data_id&0xFFFF) > 0) && ( (struct_data_id&0xFFFF) < 10) ) {
			struct_data_size = sizeof(base_data_struct);
			state = data_stream->get_struct_value(cdata_stream::PROCESSOR , struct_data_id, (void*)&base_return_data);
			if ( state != 0 ) {
				ERR("data_stream cmd_get_struct fail");
				goto out;
			}		
			return_struct_data = (void*)&base_return_data;
		}
	} else {
		ERR("Does not support module type for cmd_get_struct , current module_type : %d\n", ctx->module->type());
	}

out:

	packet->set_cmd(CMD_GET_STRUCT);
	packet->set_payload_size(sizeof(cmd_get_struct_t) + struct_data_size +4);
	return_payload = (cmd_get_struct_t*)packet->data();

	if ( !return_payload ) {		
		ERR("Error return_payload ptr check (%s) , retry to send using dummy packet\n", __FUNCTION__);
		cpacket dummy_packet(sizeof(cmd_get_struct_t)+4);
		cmd_get_struct_t *dummy_payload;
		dummy_packet.set_cmd(CMD_GET_STRUCT);
		dummy_packet.set_payload_size(sizeof(cmd_get_struct_t));
		dummy_payload = (cmd_get_struct_t*)dummy_packet.data();
		if ( !dummy_payload ) {
			ERR("Error second dummy_payload ptr check");
			return (void *)NULL;
		}

		dummy_payload->state = -1;
		
		if (ipc.send( dummy_packet.packet(), dummy_packet.size()) == false) {
			ERR("Failed to send a dummy value packet (%s)\n",__FUNCTION__);
			return (void *)NULL;
		}
		
		return (void *)NULL;
	}
	
	return_payload->state = state;
	return_payload->data_struct_size = struct_data_size;

	if (return_struct_data && (struct_data_size!=0 ) ) {
		memcpy(return_payload->data_struct , return_struct_data , struct_data_size);
	}

	DBG("Send get_property state %d\n", state);
	if (ipc.send(queue_item->packet->packet(), queue_item->packet->size()) == false) {
		ERR("Failed to send a value packet (%s)\n",__FUNCTION__);
		return (void *)NULL;
	}
	
	return (void *)NULL;
}



void *cserver::cb_ipc_start(void *data)
{
	csock::thread_arg_t *arg = (csock::thread_arg_t*)data;
	client_ctx_t *ctx;

	try {
		ctx = new client_ctx_t;
	} catch (...) {
		ERR("Failed to get memory for ctx (%s)\n",__FUNCTION__);
		arg->client_ctx = NULL;
		return (void*)cipc_worker::TERMINATE;
	}

	DBG("IPC Worker started %d\n", pthread_self());

	ctx->client_state = -1;
	ctx->module = NULL;
	ctx->reg_state = 0x00;
	arg->client_ctx = (void*)ctx;
	return (void*)NULL;
}



void *cserver::cb_ipc_stop(void *data)
{
	client_ctx_t *ctx;
	cprocessor_module *processor;
	cfilter_module *filter;
	csensor_module *sensor;
	cdata_stream *stream;
	
	cmd_reg_t forced_reg_cmd;
	int i,j;
	unsigned int reg_state_check_mask;
	unsigned int masked_check_state;
	unsigned int reg_sensor_type;

	DBG("IPC Worker stopped %d\n", pthread_self());
	csock::thread_arg_t *arg = (csock::thread_arg_t*)data;

	ctx = (client_ctx_t*)arg->client_ctx;

	if ( !ctx ) {
		ERR("Error ctx ptr check (%s)\n",__FUNCTION__);
		arg->client_ctx = NULL;
		return (void*)cipc_worker::TERMINATE;
	}

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check (%s)\n",__FUNCTION__);
		
		arg->client_ctx = NULL;
		delete ctx;
		
		return (void*)cipc_worker::TERMINATE;
	}

	DBG("Check registeration state : %x", ctx->reg_state);
	if ((ctx->reg_state & 0xFFFF) != 0x00 ) {
		ERR("Connection broken before the call_back_func unregisted, reg_state : %x",ctx->reg_state);
		reg_sensor_type = (ctx->reg_state) & (~0xFFFF) ;
		forced_reg_cmd.type = REG_DEL;
		switch (ctx->module->type()) {
			case cdata_stream::SF_DATA_STREAM:
				stream = (cdata_stream*)ctx->module;
				for (i=0; i<16; i=i+4) {
					reg_state_check_mask = (0xF << i);
					if ( (reg_state_check_mask & ctx->reg_state) == 0x00 ) continue;
					masked_check_state = (reg_state_check_mask & ctx->reg_state);
					for (j=0; j<4; j++) {						
						if ( ( (masked_check_state >> (i+j)) & 0x0001 ) == 0x0001 ) {
							forced_reg_cmd.event_type = (reg_sensor_type | ( 0x0001 << (i+j)));
							if ( !(stream->remove_callback_func(&forced_reg_cmd)) ) {
								ERR("Error forced unregisteration for cmd_type : %x , cmd_event : %x",forced_reg_cmd.type ,forced_reg_cmd.event_type);
							}
						}
					}
				}
				break;
			default:
				ERR("forced unregisteration fail , not supported type (%s)\n",__FUNCTION__);
				break;
		}
	}

	if (ctx->client_state > 0) {				
		switch (ctx->module->type()) {
		case csensor_module::SF_PLUGIN_SENSOR:		
			sensor = (csensor_module*)ctx->module;
			ERR("Server does not received cmd_stop before connection broken!! forced stop, sensor_type : %d\n", sensor->get_sensor_type());
			sensor->stop();
			break;
			
		case cprocessor_module::SF_PLUGIN_PROCESSOR:		
			processor = (cprocessor_module*)ctx->module;
			ERR("Server does not received cmd_stop before connection broken!! forced stop, processor\n" );
			processor->stop();
			processor->wakeup_all_client();
			break;
			
		case cfilter_module::SF_PLUGIN_FILTER:	
			filter = (cfilter_module *)ctx->module;
			ERR("Server does not received cmd_stop before connection broken!! forced stop, filter_type : %d\n", filter->get_sensor_type());
			filter->stop();		
			break;
			
		case cdata_stream::SF_DATA_STREAM:
			stream = (cdata_stream*)ctx->module;
			ERR("doesnot recevied cmd_stop before connection broken!! forced stop, stream_name : %s stop\n", stream->name());
			stream->stop();
			stream->wakeup_all_client();
			break;
			
		default:
			ERR("Unknown type (%s)\n",__FUNCTION__);
			break;
		}
	}

	arg->client_ctx = NULL;
	delete ctx;

	return (void*)cipc_worker::TERMINATE;
}



void *cserver::cb_ipc_worker(void *data)
{
	cmd_queue_item_t *cmd_item;
	cpacket *packet;
	csock::thread_arg_t *arg = (csock::thread_arg_t*)data;
	csock ipc(arg->client_handle, csock::SOCK_TCP);
	client_ctx_t *ctx;
	ipc.set_on_close(false);
	cserver serverobj;

	ctx = (client_ctx_t*)arg->client_ctx;

	DBG("IPC Worker started %d\n", pthread_self());

	try {
		packet = new cpacket(MAX_DATA_STREAM_SIZE);
	} catch (...) {
		return (void*)cipc_worker::TERMINATE;
	}

	DBG("Wait for a packet\n");

	if (ipc.recv(packet->packet(), packet->header_size()) == false) {
		DBG("recv failed\n");
		delete packet;
		return (void*)cipc_worker::TERMINATE;
	}

	DBG("Packet received (%d, %d)\n", packet->version(), packet->cmd());
	
	if ( !( (unsigned int)packet->payload_size() < MAX_DATA_STREAM_SIZE) ) {
		ERR("Error received packet size check (%s)\n",__FUNCTION__);
		delete packet;
		return (void*)cipc_worker::TERMINATE;
		
	}

	if (ipc.recv((char*)packet->packet() + packet->header_size(), packet->payload_size()) == false)
	{
		delete packet;
		return (void*)cipc_worker::TERMINATE;
	}

	try {
		cmd_item = new cmd_queue_item_t;
	} catch (...) {
		delete packet;
		return (void*)cipc_worker::TERMINATE;
	}

	cmd_item->packet = packet;
	cmd_item->arg = arg;

	serverobj.command_handler(cmd_item, NULL);				
	
	return (void *)cipc_worker::START;
}



void cserver::command_handler(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	cpacket *packet;	
	int cmd= CMD_NONE;
	void *ret;
	
	packet = queue_item->packet;
	cmd = packet->cmd();

	
	if ( !(cmd > 0 && cmd < CMD_LAST) ) 
	{
		ERR("cmd < 0 or cmd > CMD_LAST");
	}
	else 
	{
		ret = m_cmd_handler[cmd] ? m_cmd_handler[cmd](cmd_item, NULL) : NULL;			
	}			

	delete queue_item->packet;
	delete queue_item;
	
}


void cserver::sf_main_loop_stop(void)
{
	if(g_mainloop)
	{
		g_main_loop_quit(g_mainloop);
	}
}

void cserver::sf_main_loop(void)
{
	csock *ipc = NULL;
	int fd = -1;

	g_mainloop = g_main_loop_new(NULL, FALSE);

	try {
		ipc = new csock((char*)STR_SF_IPC_SOCKET, csock::SOCK_TCP|csock::SOCK_WORKER, 0, 1);
	} catch (int ErrNo) {
		ERR("ipc class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));		
		return ;
	}

	ipc->set_worker(cb_ipc_start, cb_ipc_worker, cb_ipc_stop);
	ipc->wait_for_client();


	g_poweroff_fd = heynoti_init();

	if(g_poweroff_fd < 0)
	{
		ERR("heynoti_init FAIL\n");
	}
	else
	{
		if(heynoti_subscribe(g_poweroff_fd, POWEROFF_NOTI_NAME, system_poweroff_cb, NULL))
		{
			ERR("heynoti_subscribe fail\n");
		}
		else
		{
			if(heynoti_attach_handler(g_poweroff_fd))
			{
				ERR("heynoti_attach_handler fail\n");
			}
		}
	}
	fd = creat("/tmp/hibernation/sfsvc_ready", 0644);
	if(fd != -1)
	{
		close(fd);
		fd = -1;
	}

	g_main_loop_run(g_mainloop);
	g_main_loop_unref(g_mainloop);	

	return;
}



//! End of a file
