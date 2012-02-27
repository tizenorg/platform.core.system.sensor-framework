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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ctrim.h>

#include <common.h>

ctrim::ctrim(void)
{
}



ctrim::~ctrim(void)
{
}


char *ctrim::ltrim(char *str)
{
	unsigned int index = 0;
	
	while ((str[index] == ' ')||(str[index] == '\t')) index ++;
	return &str[index];
}



int ctrim::rtrim(char *str)
{
	int len;
	int org_len;
	
	org_len = len = strlen(str) - 1;

	while ((str[len] == ' ')||(str[len] == '\t')) len --;
	str[len + 1] = (char)NULL;

	return org_len - len;
}



//! End of a file
