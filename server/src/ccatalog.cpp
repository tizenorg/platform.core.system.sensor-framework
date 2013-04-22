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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <common.h>
#include <cobject_type.h>
#include <ctrim.h>
#include <clist.h>
#include <ccatalog.h>
#define TMP_BUF_SIZE	512

ccatalog::ccatalog()
: m_head(NULL)
, m_tail(NULL)
{
}

ccatalog::~ccatalog()
{
	if (m_head) unload();
}

bool ccatalog::load(const char *conf_file , char splitter_id)
{
	FILE *fp;
	group_t *grp = NULL;
	desc_t *desc;
	char line_buffer[TMP_BUF_SIZE];
	char tmp_buffer[TMP_BUF_SIZE];
	int rtrim_num;
	int split_num = 0;
	int group_name_check = 0;
	char name[TMP_BUF_SIZE];
	char value[TMP_BUF_SIZE];
	char *ptr;	
	
	switch (splitter_id ) {
		case SPLITTER_FOR_CONF_FILE:
		case '=':
			break;
		case SPLITTER_FOR_CPU_INFO:
		case ':':
			if ( !m_head ) {
				try {
					grp = new group_t;				
				} catch (...) {
					ERR("No memory");
					return false;
				}		
				m_head = m_tail = grp;	
				grp->name = strdup("CPU_INFO_TABLE");				
				if (!grp->name) {
					ERR("Failed to allocate buffer for the tmp_buffer\n");
					delete grp;
					return false;
				}
				DBG("grp->name : %s\n",grp->name);
				grp->attr_cnt = 0;

				grp->head = NULL;
				grp->tail = NULL;
			} else {
				ERR("already CPU_INFO_TABLE exist !! cannot make again\n");
				return false;
			}
			break;

		default:
			ERR("fail does not support splitter idx : %x",splitter_id);
			return false;
	}

	fp = fopen(conf_file, "r");
	if (!fp) {
		return false;
	}
	
	while (fgets(line_buffer, TMP_BUF_SIZE, fp)) {
		ptr = ctrim::ltrim(line_buffer);
		if (*ptr == '#') {
			continue;
		}
		DBG("read line_buffer : %s\n",line_buffer);

		/* Replcate the last character if it was new line. */
		if ( (ptr[strlen(ptr)-1] == '\n') || (ptr[strlen(ptr)-1] == '\r')) ptr[strlen(ptr)-1] = '\0';

		rtrim_num = ctrim::rtrim(ptr); 				

		switch (splitter_id ) {
			case SPLITTER_FOR_CONF_FILE:
			case '=':
				group_name_check = sscanf(ptr, "[%[^]] ", tmp_buffer);
				break;
			case SPLITTER_FOR_CPU_INFO:
			case ':':
				group_name_check = 0;
				break;
		}
		
		if (group_name_check == 1) {
			/* We have previous group, Just link it to the end of group list */
			DBG("check tmp_buffer : %s\n",tmp_buffer);
			if (grp) {
				if (m_tail) {
					grp->link(clist::AFTER, m_tail);
					m_tail = grp;
				} else {
					m_head = m_tail = grp;
				}				
			}

			try {
				grp = new group_t;				
			} catch (...) {
				ERR("No memory");
				fclose(fp);
				return false;
			}

			grp->name = strdup(tmp_buffer);
			DBG("grp->name : %s\n",grp->name);
			if (!grp->name) {
				ERR("Failed to allocate buffer for the tmp_buffer\n");
				delete grp;
				fclose(fp);
				return false;
			}
			grp->attr_cnt = 0;
			grp->head = NULL;
			grp->tail = NULL;
			continue;
		}

		switch (splitter_id ) {
			case SPLITTER_FOR_CONF_FILE:
			case '=':
				split_num = sscanf(ptr, "%[^=]=%[^\n]", name, value);
				break;
			case SPLITTER_FOR_CPU_INFO:
			case ':':
				split_num = sscanf(ptr, "%[^:]:%[^\n]", name, value);
				DBG("split_num : %d , raw_name : [start]%s[end] , raw_value : [start]%s[end]\n",split_num ,name , value);
				break;
			default:
				ERR("fail does not support splitter idx : %x",splitter_id);
				split_num = 0;
				break;
		} 

		if ( (split_num == 2) && (grp!=NULL) ) {
			try {
				desc = new desc_t;
			} catch (...) {				
				ERR("Failed to allocate new description\n");	
				free(grp->name);
				delete grp;
				fclose(fp);
				return false;
			}

			ptr = ctrim::ltrim(name);
			rtrim_num = ctrim::rtrim(ptr);			
			desc->name = strdup(ptr);
			DBG("desc->name :%s[end]\n",desc->name);

			if (!desc->name) {
				ERR("Failed to allocate buffer for the name\n");
				free(grp->name);
				delete grp;
				delete desc;
				fclose(fp);
				return false;
			}

			ptr = ctrim::ltrim(value);
			rtrim_num = ctrim::rtrim(ptr);
			desc->value = strdup(ptr);
			DBG("desc->value :%s[end]\n",desc->value);

			if (!desc->value) {
				ERR("Failed to allocate buffer for the value\n");
				free(grp->name);
				delete grp;
				free(desc->name);
				delete desc;
				fclose(fp);
				return false;
			}

			if (grp->tail) {
				desc->link(clist::AFTER, grp->tail);
				grp->tail = desc;
			} else {
				grp->head = grp->tail = desc;
			}

			grp->attr_cnt ++;			
			continue;
		}
	}

	if (m_tail != grp) {
		if (m_tail && grp != NULL) {
			grp->link(clist::AFTER, m_tail);
			m_tail = grp;
		} else {
			m_tail = m_head = grp;
		}
	}

	fclose(fp);
	return true;
}

bool ccatalog::is_loaded(void)
{
	return (m_head != NULL) ?  true : false ;
}

bool ccatalog::unload(void)
{
	group_t *grp;
	group_t *next_grp;
	desc_t *desc;
	desc_t *next_desc;

	grp = m_head;
	while (grp) {
		next_grp = (group_t*)grp->next();
		desc = grp->head;
		while (desc) {
			next_desc = (desc_t*)desc->next();
			free(desc->name);
			free(desc->value);
			delete desc;
			desc = next_desc;
		}
		free(grp->name);
		delete grp;
		grp = next_grp;
	}
	m_head = NULL;
	m_tail = NULL;
	return true;
}

char *ccatalog::value(char *group, char *name)
{
	group_t *grp;

	grp = m_head;
	while (grp) {
		if (!strncmp(group, grp->name, strlen(grp->name))) {
			break;
		}
		grp = (group_t*)grp->next();
	}

	if (grp) {
		desc_t *desc;
		desc = grp->head;

		while (desc) {
			if (!strncmp(name, desc->name, strlen(desc->name))) {
				return desc->value;
			}
			desc = (desc_t*)desc->next();
		}
	}

	return NULL;
}

char *ccatalog::value(void *handle, char *name)
{
        group_t *grp = (group_t *)handle;

        if (grp) {
                desc_t *desc;
                desc = grp->head;

                while (desc) {
                        if (!strncmp(name, desc->name, strlen(desc->name))) {
                                return desc->value;
                        }
                        desc = (desc_t*)desc->next();
                }
        }

        return NULL;
}

char *ccatalog::value(char *group, char *name, int idx)
{
	group_t *grp;
	grp = m_head;
	while (grp) {
		if (!strcmp(group, grp->name)) {
			break;
		}
		grp = (group_t*)grp->next();
	}

	if (grp) {
		register int i;
		desc_t *desc;
		desc = grp->head;
		i = 0;
		while (desc) {
			if (!strcmp(name, desc->name)) {
				if (i == idx) {
					return desc->value;
				}
					i ++;
			}
			desc = (desc_t *)desc->next();
		}
	}

	return NULL;
}

int ccatalog::count_of_values(char *group, char *name)
{
	group_t *grp;
	register int count = 0;
	grp = m_head;
	while (grp) {
		if (!strcmp(group, grp->name)) {
			break;
		}
		grp = (group_t*)grp->next();
	}

	if (grp) {
		desc_t *desc;
		desc = grp->head;

		while (desc) {
			if (!strcmp(name, desc->name)) {
				count ++;
			}
			desc = (desc_t *)desc->next();
		}
	}
	return count;
}

void *ccatalog::iterate_init(void)
{
	return (void*)m_head;
}

void *ccatalog::iterate_next(void *handle)
{
	group_t *grp = (group_t*)handle;
	return grp->next();
}

void *ccatalog::iterate_prev(void *handle)
{
	group_t *grp = (group_t*)handle;
	return grp->prev();
}

char *ccatalog::iterate_get_name(void *handle)
{
	group_t *grp = (group_t*)handle;
	return grp->name;
}
//! End of a file
