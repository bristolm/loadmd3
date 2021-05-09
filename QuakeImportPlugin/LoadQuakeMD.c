/* Modeller include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//K:\Lightwave\Programs\Lightwave_Support\Modeler.exe
/* Mystuff */
#include <lwmeshes.h>
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
BuildData		MY_Data = {0};
BuildData		*CurrentData = (BuildData *)NULL;

LWDirInfoFunc 			*DirInfo = 0;

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
loadquakemdfile (long version,GlobalFunc *global,void *local,void *serverData)
{

	int				retval = AFUNC_OK;
	unsigned short	lwpntIndexCounter = 0;

	char			*pExt = (char *)NULL;	
	XCALL_INIT;

//	if (version > 1)
//		return AFUNC_BADVERSION;


	if (version > 2 || version < 1)
		return AFUNC_BADVERSION;

	FindProductInfo(global);

	MY_Data.Globals = global;
	MY_Data.Funcs	= local;
	MY_Data.Message = global("Info Messages",GFUSE_TRANSIENT);
	if ( !MY_Data.Message)
		return AFUNC_BADGLOBAL;

	MY_Data.Panel = (*global) (PANEL_SERVICES_NAME, GFUSE_TRANSIENT);

	// Set path ...

	DirInfo = (LWDirInfoFunc  *)global(LWDIRINFOFUNC_GLOBAL,GFUSE_TRANSIENT);
	StoreLastPath(DirInfo("Objects"));

	if (version == 1)
	{
//		return LW56_LoadinMD3(&MY_Data);
		if (MY_Data.ScaleForSkinMesh < 0.0)
			MY_Data.ScaleForSkinMesh = 0.0;
	}
	else
	{
		MY_Data.LoadSkinVerts = -1;			// This means not to ask about skin
		return LW60_LoadinMD3(&MY_Data);
	}

	return retval;
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

	if (version != 3)
		return AFUNC_BADVERSION;

	FindProductInfo(global);

	MY_Data.Globals = global;
	MY_Data.Funcs	= local;
	MY_Data.Message = (*global)("Info Messages",GFUSE_TRANSIENT);
	if ( !MY_Data.Message)
		return AFUNC_BADGLOBAL;

	MY_Data.Panel = (*global) (PANEL_SERVICES_NAME, GFUSE_TRANSIENT);

	// Set path ...
	DirInfo = (LWDirInfoFunc  *)global("Directory Info",GFUSE_TRANSIENT);
	StoreLastPath(DirInfo("Objects"));

	if (ProductInfo.major == 5)
	{
//		return LW56_ImportMD3(&MY_Data);
		if (MY_Data.ScaleForSkinMesh < 0.0)
			MY_Data.ScaleForSkinMesh = 0.0;
	}
	else
	{
		MY_Data.LoadSkinVerts = -1;			// This means not to ask about skin
		return LW60_ImportMD3(&MY_Data);
	}

	return retval;
};

ServerRecord ServerDesc[] = {
    { "ObjectLoader", "MRB_QuakeMD_FileLoader", loadquakemdfile},
	{ "MeshDataEdit", "MRB_QuakeMD_Import", impquakemdfile},
    { NULL }
};