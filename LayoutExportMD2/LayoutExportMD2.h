#ifndef _LW_LAYOUTEXPORTMD3_H
#define _LW_LAYOUTEXPORTMD3_H

#include <lwserver.h>
#include <lwhost.h>
#include <lwmrbxprt.h>
#include <lwglobsrv.h>

#define PLUGINNAME			LWMRBPREFIX"Quake2"
#define GLOBALPLUGINNAME	LWMRBPREFIX"Quake2Global"

#define _PLUGINVERSION(a)		1.##a
#define PLUGINVERSION		(float)_PLUGINVERSION(PROG_PATCH_VER)

extern GlobalFunc		*LW_globalFuncs;
extern LWMRBExportFuncs	*LW_xprtFuncs;
extern LWMessageFuncs	*LW_msgsFuncs;
extern LWXPanelFuncs	*LW_xpanFuncs;

extern LWMRBExportType	 *getFunc();
extern LWMRBCallbackType *getCallback();

extern char	*DIR_SEPARATOR;
extern char	tmp[1024];


#endif //_LW_LAYOUTEXPORTMD3_H
