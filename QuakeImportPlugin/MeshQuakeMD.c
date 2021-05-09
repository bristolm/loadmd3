/* Modeller include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Mystuff */
#include "LWPlugBase.h"
#include "LoadQuakeMD.h"
#include "mdv_mdformat.h"

/*
 * ObjectImport plug-in.
 *
 * Responds to the use choosing a .md3 file, pops up a requestor and 
 * asks for the anchoring tag
 */

MessageFuncs	*message;
LWPanelFuncs	*panl;
LWPanelID		panID;
LWPanControlDesc	desc;	 								// required by macros in lwpanel.h
LWValue				ival={LWT_INTEGER},ivecval={LWT_VINT},	// required by macros in lwpanel.h
  fval={LWT_FLOAT},fvecval={LWT_VFLOAT},sval={LWT_STRING};
GlobalFunc		*Globals;
ObjectImport	*OInput;

/* =============================
 * Activation function is called from the generic plug-ins popup.
 * =============================

	    #define OBJSTAT_OK       0
	    #define OBJSTAT_NOREC    1
	    #define OBJSTAT_BADFILE  2
	    #define OBJSTAT_ABORTED  3
	    #define OBJSTAT_FAILED   99
 */

	XCALL_(static int)
loadquakemdfile (long version,GlobalFunc *global,void *local,void *serverData)
{

	int				retval = AFUNC_OK;
	unsigned short	lwpntIndexCounter = 0;

	char			*pExt = (char *)NULL;	
	char			tmp[1024];
	XCALL_INIT;

	Globals = global;

	OInput = (ObjectImport *)local;
	OInput->result = OBJSTAT_OK;

	message = (MessageFuncs *) (*global)("Info Messages",GFUSE_TRANSIENT);
	if ( !message)
		return (AFUNC_BADGLOBAL);

	panl = (*global) (PANEL_SERVICES_NAME, GFUSE_TRANSIENT);
	if(!panl)
	{
		(*message->error)("Unable to activate global "PANEL_SERVICES_NAME, "     please add plugin lwpanels.p" );
		return AFUNC_BADGLOBAL;
	}

	if (strlen (OInput->filename) < 5)
	{
		SetFailureStuff(OInput,"Bad Filename: ",(char *)OInput->filename);
		OInput->result = OBJSTAT_NOREC;
		return OBJSTAT_BADFILE;
	}

	pExt = (char *)(OInput->filename) + strlen(OInput->filename) -4;
	if (strcmp(pExt,".md3") == 0)
	{
//		sprintf(tmp,"Quake3 file",NULL);
//		message->error("Update",tmp);
		retval = ParseMD3FromFile(OInput->filename);
	}
/*	else if  (strcmp(pExt,".md2") == 0)
	{
		sprintf(tmp,"Quake2 file",NULL);
		message->error("Update",tmp);
	}
*/	else
	{
		SetFailureStuff(OInput,"Invalid extension: ",(char *)OInput->filename);
		OInput->result = OBJSTAT_NOREC;
		return OBJSTAT_BADFILE;
	}

	OInput->done(OInput->data);

	return AFUNC_OK;
};

	XCALL_(static int)
importquakemdfile (long version,GlobalFunc *global,void *local,void *serverData)
{

	int				retval = AFUNC_OK;
	unsigned short	lwpntIndexCounter = 0;

	char			*pExt = (char *)NULL;	
	char			tmp[1024];
	XCALL_INIT;


	Globals = global;

	OInput = (ObjectImport *)local;
	OInput->result = OBJSTAT_OK;

	message = (MessageFuncs *) (*global)("Info Messages",GFUSE_TRANSIENT);
	if ( !message)
		return (AFUNC_BADGLOBAL);

	panl = (*global) (PANEL_SERVICES_NAME, GFUSE_TRANSIENT);
	if(!panl)
	{
		(*message->error)("Unable to activate global "PANEL_SERVICES_NAME, "     please add plugin lwpanels.p" );
		return AFUNC_BADGLOBAL;
	}
	return AFUNC_OK;


};



ServerRecord ServerDesc[] = {
    { "ObjectLoader", "MRB_QuakeMD_FileLoader", loadquakemdfile},
    { "MeshEdit", "MRB_QuakeMD_Import", importquakemdfile},
    { NULL }
};