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




class cdata_stream : public clist
{
public:
	enum value_type_t {
		SENSOR	= 0x01,
		FILTER	= 0x02,
		PROCESSOR = 0x03,
	};

	static const int SF_DATA_STREAM = ctype::UNKNOWN + 40;

	cdata_stream();
	virtual ~cdata_stream();

	static bool create(char *conf);
	static void destroy(void);

	char *name(void);
	int id(void);

	bool update_name(char *name);
	bool update_id(int id);

	bool start(void);
	bool stop(void);

	bool add_event_callback(void  *(*handler)(cprocessor_module *, void *), void *data, bool (*rm_cb_data)(void *) = NULL);
	bool rm_event_callback(void *(*handler)(cprocessor_module *, void*), void *data);
	void wakeup_all_client(void);

	long value(value_type_t type, char *port);
	long value(value_type_t type, int id);
	int get_struct_value(value_type_t type, unsigned int struct_id , void *struct_values);

	static cdata_stream *search_stream(char *name);
	static cdata_stream *search_stream(int id);

	bool add_callback_func(cmd_reg_t *param);
	bool remove_callback_func(cmd_reg_t * param);
	bool check_callback_event(cmd_reg_t * param);

	cprocessor_module *my_processor(void);
	cfilter_module *my_filter(void);

	long set_cmd(int type , int property , long input_value);
	int get_property(unsigned int property_level , void *property_data );

private:
	struct filter_list_t : public clist {
		cfilter_module *filter;
	};

	struct processor_list_t : public clist {
		cprocessor_module *processor;
	};

	cprocessor_module *select_processor(void);
	cfilter_module *select_filter(void);

	processor_list_t *composite_processor(char *value);
	filter_list_t *composite_filter(char *value, int multi_chek);

	cfilter_module *search_filter(char *name);
	cprocessor_module *search_processor(char *name);

	filter_list_t *m_filter_head;
	filter_list_t *m_filter_tail;

	processor_list_t *m_processor_head;
	processor_list_t *m_processor_tail;

	char *m_name;
	int m_id;

	int m_client;

	static cdata_stream *m_head;
	static cdata_stream *m_tail;
	static ccatalog m_catalog;

	pthread_mutex_t mutex_lock;
};



//! End of a file
