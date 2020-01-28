#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>


#include "ngx_func.h"     //函数声明
#include "ngx_c_conf.h"   //和配置文件处理相关的类

//静态成员赋值
CConfig *CConfig::m_instance = NULL;

//构造函数
CConfig::CConfig()
{		
}

//析构函数
CConfig::~CConfig()
{    
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{		
		delete (*pos);
	}//end for
	m_ConfigItemList.clear(); 
}

//装载配置文件
bool CConfig::Load(const char *pconfName) 
{   
    FILE *fp;
    fp = fopen(pconfName,"r");
    if(fp == NULL)
        return false;

   
    char  linebuf[501];  
    
    while(!feof(fp))  
    {    
        if(fgets(linebuf,500,fp) == NULL) 
            continue;

        if(linebuf[0] == 0)
            continue;

        //处理注释行
        if(*linebuf==';' || *linebuf==' ' || *linebuf=='#' || *linebuf=='\t'|| *linebuf=='\n')
			continue;
        
    lblprocstring:
        //屁股后边若有换行，回车，空格等都截取掉
		if(strlen(linebuf) > 0)
		{
			if(linebuf[strlen(linebuf)-1] == 10 || linebuf[strlen(linebuf)-1] == 13 || linebuf[strlen(linebuf)-1] == 32) 
			{
				linebuf[strlen(linebuf)-1] = 0;
				goto lblprocstring;
			}		
		}
        if(linebuf[0] == 0)
            continue;
        if(*linebuf=='[')
			continue;

        char *ptmp = strchr(linebuf,'=');
        if(ptmp != NULL)
        {
            LPCConfItem p_confitem = new CConfItem;                   
            memset(p_confitem,0,sizeof(CConfItem));
            strncpy(p_confitem->ItemName,linebuf,(int)(ptmp-linebuf));
            strcpy(p_confitem->ItemContent,ptmp+1);                   

            Rtrim(p_confitem->ItemName);
			Ltrim(p_confitem->ItemName);
			Rtrim(p_confitem->ItemContent);
			Ltrim(p_confitem->ItemContent);

            m_ConfigItemList.push_back(p_confitem);
        } //end if
    } //end while(!feof(fp)) 

    fclose(fp);
    return true;
}

//根据ItemName获取配置信息字符串
const char *CConfig::GetString(const char *p_itemname)
{
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{	
		if(strcasecmp( (*pos)->ItemName,p_itemname) == 0)
			return (*pos)->ItemContent;
	}//end for
	return NULL;
}
//根据ItemName获取数字类型配置信息
int CConfig::GetIntDefault(const char *p_itemname,const int def)
{
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos !=m_ConfigItemList.end(); ++pos)
	{	
		if(strcasecmp( (*pos)->ItemName,p_itemname) == 0)
			return atoi((*pos)->ItemContent);
	}//end for
	return def;
}



