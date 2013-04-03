/*
 * sensor-framework 
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




#if !defined(_CATALOG_CLASS_H_)
#define _CATALOG_CLASS_H_



class ccatalog
{
public:
	enum ccatalog_splitter_id {
		SPLITTER_FOR_CONF_FILE = 0x00,
		SPLITTER_FOR_CPU_INFO = 0x01,
	};
	
	ccatalog();
	virtual ~ccatalog();

	bool load(const char *conf_file , char splitter_id = SPLITTER_FOR_CONF_FILE);
	bool is_loaded(void);
	bool unload(void);

	char *value(char *group, char *name);
	char *value(void *handle, char *name);
	char *value(char *group, char *name, int idx);
	int count_of_values(char *group, char *name);

	void *iterate_init(void);
	void *iterate_next(void *handle);
	void *iterate_prev(void *handle);
	char *iterate_get_name(void *handle);

private:
	struct desc_t : public clist {
		char *name;
		char *value;
	};

	struct group_t : public clist {
		char *name;
		int attr_cnt;

		desc_t *head;
		desc_t *tail;
	};

	group_t *m_head;
	group_t *m_tail;
};



#endif
//! End of a file
