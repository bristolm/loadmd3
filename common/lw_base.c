/**************************************
 *
 *  lw_base.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Few helper functions for LW plugins
 *
 **************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lw_base.h"
#include "lwobjimp.h"
#include "lwhost.h"

char					LastPath[1024] = ".";
LightwaveProductInfo	ProductInfo;

LWPanControlDesc	desc;	 								// required by macros in lwpanel.h
LWValue				ival={LWT_INTEGER},ivecval={LWT_VINT},	// required by macros in lwpanel.h
  fval={LWT_FLOAT},fvecval={LWT_VFLOAT},sval={LWT_STRING};


// Grab the last path
char *GetLastPath(const char *name)
{
	return LastPath;
}

// This assumes we are getting a path name, so it adds a / if necessary
void StoreLastPath(const char *name)
{
	char *p;

	if (name == (char *)NULL || name[0] == '0')
		return;

	strcpy(LastPath,name);
	p = LastPath + strlen(LastPath) -1;
	if (*(p++) != '\\')
		*(p++) = '\\';

	*p = 0;
}

// This assumes we are getting a filename, so it whittles back to the path
void FindLastPathfromName(const char *name)
{
	char *p;

	if (name == (char *)NULL || name[0] == '0')
		return;

	strcpy(LastPath,name);
	p = LastPath + strlen(LastPath);
	while (p > LastPath)
	{
		if (*p == '\\')
		{
			*p = 0;
			break;
		}
		p--;
	}
}

void SetFailureStuff(LWObjectImport *oi, char *msg, char *xtramsg)
{
	oi->failedLen = strlen(msg) +1;

	if (xtramsg != (char *)NULL)
		oi->failedLen += strlen(xtramsg);
	
	oi->failedBuf = calloc(oi->failedLen,1);

	if (xtramsg != (char *)NULL)
		sprintf(oi->failedBuf,"%s%s",msg,xtramsg);
	else
		strcpy(oi->failedBuf, msg);
}

void cmdGetInt(LWControl *ctl, int *dat)
{
	GET_INT(ctl,*dat);
}

void cmdGetStr(LWControl *ctl, char **dat)
{
	GET_STR(ctl,*dat,68);
}

void FindProductInfo(GlobalFunc *global)
{	// MeshDataEdit is version == 3 for both 5.6 and 6.0 - use the product
	// info to figure out which one we are
	static unsigned long prodinfo = 0;

	if (prodinfo != 0)
		return;

	prodinfo = (unsigned long)global(LWPRODUCTINFO_GLOBAL,GFUSE_TRANSIENT);

	ProductInfo.major = LWINF_GETMAJOR(prodinfo);
	ProductInfo.minor = LWINF_GETMINOR(prodinfo);
	ProductInfo.build = LWINF_GETBUILD(prodinfo);
}