

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "gsx_func.h"
#include "gsx_c_conf.h"

CConfig *CConfig::m_instance = NULL;

CConfig::CConfig()
{
}

CConfig::~CConfig()
{
	std::vector<LPCConfItem>::iterator pos;
	for (pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{
		delete (*pos);
	}
	m_ConfigItemList.clear();
	return;
}

bool CConfig::Load(const char *pconfName)
{
	FILE *fp;
	fp = fopen(pconfName, "r");
	if (fp == NULL)
		return false;

	char linebuf[501];

	while (!feof(fp))
	{

		if (fgets(linebuf, 500, fp) == NULL)
			continue;

		if (linebuf[0] == 0)
			continue;

		if (*linebuf == ';' || *linebuf == ' ' || *linebuf == '#' || *linebuf == '\t' || *linebuf == '\n')
			continue;

	lblprocstring:

		if (strlen(linebuf) > 0)
		{
			if (linebuf[strlen(linebuf) - 1] == 10 || linebuf[strlen(linebuf) - 1] == 13 || linebuf[strlen(linebuf) - 1] == 32)
			{
				linebuf[strlen(linebuf) - 1] = 0;
				goto lblprocstring;
			}
		}
		if (linebuf[0] == 0)
			continue;
		if (*linebuf == '[')
			continue;

		char *ptmp = strchr(linebuf, '=');
		if (ptmp != NULL)
		{
			LPCConfItem p_confitem = new CConfItem;
			memset(p_confitem, 0, sizeof(CConfItem));
			strncpy(p_confitem->ItemName, linebuf, (int)(ptmp - linebuf));
			strcpy(p_confitem->ItemContent, ptmp + 1);

			Rtrim(p_confitem->ItemName);
			Ltrim(p_confitem->ItemName);
			Rtrim(p_confitem->ItemContent);
			Ltrim(p_confitem->ItemContent);

			m_ConfigItemList.push_back(p_confitem);
		}
	}

	fclose(fp);
	return true;
}

const char *CConfig::GetString(const char *p_itemname)
{
	std::vector<LPCConfItem>::iterator pos;
	for (pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{
		if (strcasecmp((*pos)->ItemName, p_itemname) == 0)
			return (*pos)->ItemContent;
	}
	return NULL;
}

int CConfig::GetIntDefault(const char *p_itemname, const int def)
{
	std::vector<LPCConfItem>::iterator pos;
	for (pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{
		if (strcasecmp((*pos)->ItemName, p_itemname) == 0)
			return atoi((*pos)->ItemContent);
	}
	return def;
}
