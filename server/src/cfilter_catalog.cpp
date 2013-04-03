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
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <common.h>
#include <cobject_type.h>

#include <clist.h>
#include <cmutex.h>
#include <ccatalog.h>
#include <csync.h>
#include <cworker.h>
#include <cipc_worker.h>
#include <csock.h>
#include <cpacket.h>
#include <sf_common.h>
#include <cmodule.h>
#include <csensor_module.h>
#include <cfilter_module.h>
#include <cfilter_catalog.h>

#include <resource_str.h>



cfilter_catalog::cfilter_catalog()
{
}



cfilter_catalog::~cfilter_catalog()
{
}



bool cfilter_catalog::create(char *file)
{
	void *handle;
	char *name;
	char *value;
	cmodule *module;

	if (ccatalog::load(file) == false) {
		if ( ccatalog::is_loaded() ) {
			DBG("The catalog is already loaded , so first unload it");
			ccatalog::unload();
		}
		ERR("Failed to load a catalog file\n");
		return false;
	}

	handle = ccatalog::iterate_init();
	while (handle) {
		name = ccatalog::iterate_get_name(handle);
		if (!name) {
			ERR("Name is null\n");
			handle = ccatalog::iterate_next(handle);
			continue;
		}

		value = ccatalog::value(handle, (char*)STR_DISABLE);
		if (value && !strcasecmp(value, STR_YES)) {
			ERR("%s is disabled\n", name);
			handle = ccatalog::iterate_next(handle);
			continue;
		}

		value = ccatalog::value(handle, (char*)STR_PATH);
		if (!value) {
			ERR("Module path is not defined\n");
			handle = ccatalog::iterate_next(handle);
			continue;
		}

		module = cfilter_module::register_module(value, NULL, NULL);
		if (!module) {
			ERR("Failed to register a module %s\n", name);
			handle = ccatalog::iterate_next(handle);
			continue;
		}

		value = ccatalog::value(handle, (char*)STR_OVERRIDE);
		if (value && !strcasecmp(value, STR_YES)) {
			DBG("Let's override module description\n");

			if (module->update_name(name) == false) {
				ERR("Failed to update module name\n");
			}

			value = ccatalog::value(handle, (char*)STR_ID);
			if (value) {
				if (module->update_id(atoi(value)) == false) {
					ERR("Failed to update ID\n");
				}
			}

			value = ccatalog::value(handle, (char*)STR_VERSION);
			if (value) {
				if (module->update_version(atoi(value)) == false) {
					ERR("Failed to update version\n");
				}
			}
		}
		handle = ccatalog::iterate_next(handle);
	}

	DBG("Finished registeration , unload filter.conf ");
	ccatalog::unload();
	
	return true;
}



void cfilter_catalog::destroy(void)
{
	if ( ccatalog::is_loaded() ) {
		ccatalog::unload();
	}
}



//! End of a file
