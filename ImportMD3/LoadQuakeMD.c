/* Modeller include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//K:\Lightwave\Programs\Lightwave_Support\Modeler.exe
/* Mystuff */
#include <lwmeshes.h>
#include <lwmeshedt.h>
#include <lwobjimp.h>
#include "lwpanel.h"
#include "lwhost.h"

#include "lw_base.h"
#include "LoadQuakeMD.h"

/*
 * ObjectImport plug-in.
 *
 * Responds to the use choosing a .md3 file, pops up a requestor and 
 * asks for the anchoring tag

 * MeshDataEdit plug-in
 *
 * User selects file with selector.
 * requestor asks for anchoring tag
 */

// Check name
int IsMD3NameOK(const char *name)
{
	char	*pExt = (char *)NULL;

	if (strlen(name) < 5)
		return 0;

	pExt = (char *)(name) + strlen(name) -4;
	if (*pExt == '.' && *(pExt + 3) == '3' &&
		(*(pExt +1) == 'm' || *(pExt +1) == 'M') &&
		(*(pExt +2) == 'd' || *(pExt +2) == 'D')
		)
		return 1;

	return 0;
}

// Store user-returned values in here
LWDirInfoFunc 	*DirInfo = 0;
void			*LocalFuncs = 0;

GlobalFunc		*LW_globalFuncs = 0;
LWMessageFuncs	*LW_messageFuncs = 0;
LWCommandFunc   LW_cmdFunc		= (LWCommandFunc)0;


/* =============================
 * Activation function is called from the generic plug-ins popup.
 * =============================

	    #define OBJSTAT_OK       0
	    #define OBJSTAT_NOREC    1
	    #define OBJSTAT_BADFILE  2
	    #define OBJSTAT_ABORTED  3
	    #define OBJSTAT_FAILED   99
 */

// version seems to be 1 for 5.6
// version seems to be 2 for 6.0

static void FindProductInfo(GlobalFunc *global)
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

XCALL_(static int)
loadquakemdfile (long version, GlobalFunc *global, 
							LWObjectImport *local, void *serverData)
{

	int				retval = AFUNC_OK;
	unsigned short	lwpntIndexCounter = 0;

	char			*pExt = (char *)NULL;	
	XCALL_INIT;

	if (version != LWOBJECTIMPORT_VERSION) return AFUNC_BADVERSION;

	// Preload values
	local->result = LWOBJIM_NOREC;

	FindProductInfo(global);
	LocalFuncs	= local;

	LW_globalFuncs = global;
	LW_messageFuncs = global(LWMESSAGEFUNCS_GLOBAL,GFUSE_TRANSIENT);
	if ( !LW_messageFuncs)	return AFUNC_BADGLOBAL;

	// Set path ...

	DirInfo = (LWDirInfoFunc  *)global(LWDIRINFOFUNC_GLOBAL,GFUSE_TRANSIENT);
	StoreLastPath(DirInfo("Objects"));

	LW_cmdFunc = (LWCommandFunc)global( LWCOMMANDINTERFACE_GLOBAL, GFUSE_ACQUIRE );

	return LW60_LoadinMD3();
};

// version seems to be 3 for 5.6
// version seems to be 4 for 6.0

XCALL_(static int)
impquakemdfile (long version,GlobalFunc *global,void *local,void *serverData)
{

	int				retval = AFUNC_OK;

	unsigned short	lwpntIndexCounter = 0;

	char			*pExt = (char *)NULL;
	
	XCALL_INIT;

	if (version != LWMESHEDIT_VERSION) return AFUNC_BADVERSION;

	FindProductInfo(global);
	LocalFuncs	= local;

	LW_globalFuncs = global;
	LW_messageFuncs = global(LWMESSAGEFUNCS_GLOBAL,GFUSE_TRANSIENT);
	if ( !LW_messageFuncs)	return AFUNC_BADGLOBAL;

	// Set path ...
	DirInfo = (LWDirInfoFunc  *)global(LWDIRINFOFUNC_GLOBAL,GFUSE_TRANSIENT);
	StoreLastPath(DirInfo("Objects"));

	LW_cmdFunc = (LWCommandFunc)global( LWCOMMANDINTERFACE_GLOBAL, GFUSE_ACQUIRE );

	return LW60_ImportMD3();
};

ServerRecord ServerDesc[] = {
    { LWOBJECTIMPORT_CLASS,	PLUGINNAME_LOAD, loadquakemdfile},
	{ LWMESHEDIT_CLASS,		PLUGINNAME_MESH, impquakemdfile},
    { NULL }
};