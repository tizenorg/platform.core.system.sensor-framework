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





class cserver
{
public:
	enum {
		MAX_CMD_COUNT = 256,
	};

	cserver();
	virtual ~cserver();

	
	void sf_main_loop(void);

private:

	struct client_ctx_t {
		ctype *module;
		int client_state;
		unsigned int reg_state;
	};	

	static void *cb_ipc_worker(void *data);
	static void *cb_ipc_start(void *data);
	static void *cb_ipc_stop(void *data);

	void  command_handler(void *cmd_item, void *ctx);
	
	//! Attach
	static void *cmd_hello(void *cmd_item, void *data);
	//! Detach
	static void *cmd_byebye(void *cmd_item, void *data);
	//! Get value
	static void *cmd_get_value(void *cmd_item, void *data);
	//! Wait event
	static void *cmd_wait_event(void *cmd_item, void *data);
	//! Start
	static void *cmd_start(void *cmd_item, void *data);
	//! Stop
	static void *cmd_stop(void *cmd_item, void *data);
	//! Register lib_callback
	static void *cmd_register_event(void *cmd_item, void *data);
	//! Set cmd
	static void *cmd_set_value(void *cmd_item, void *data);
	//! Get property
	static void *cmd_get_property(void *cmd_item, void *data);
	//! Get struct_data
	static void *cmd_get_struct(void *cmd_item, void *data);


	static void *cb_event_handler(cprocessor_module *, void *);
	static void cb_rm_cb_data(void *);


	cmd_func_t m_cmd_handler[CMD_LAST];
};



//! End of a file
