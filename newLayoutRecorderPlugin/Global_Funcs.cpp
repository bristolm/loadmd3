/**************************************
 *
 *  Global_Funcs.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  A Global class plug-in that manages common data for the file recorder
 *  To call this from your code, first get the structure with the math
 *  functions by calling the global() function.
 *  #include "lwmrbxprt.h"
 *  
 *  LWMRBExportFuncs *LW_xprtFuncs;
 *  
 *  LW_xprtFuncs = (LWMRBExportFuncs *) global( LWGLOBALMD3_GLOBAL, GFUSE_TRANSIENT );
 *  if ( !LW_xprtFuncs ) return AFUNC_BADGLOBAL;
 *  
 *  // Then just call the functions 
 *  ...
 *  int idx = LW_xprtFuncs-> ; 
 *  ...
 **************************************/

#include <lwserver.h>
#include <lwglobsrv.h>
#include <lwdisplce.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lwmrbxprt.h"
#include "LwChannel.h"

extern "C"
{
#include "LayoutRecorderPlugin.h"
}

static int _get_activation (long version, GlobalFunc *global, LWDisplacementHandler *local)
{
	_Setup(global);

	XCALL_INIT;
    if ( version != LWDISPLACEMENT_VERSION ) return AFUNC_BADVERSION;

	SetupDisplacementFuncs(local);

	return AFUNC_OK;
};

static int _get_interface (long version, GlobalFunc *global, LWInterface *local)
{
	_Setup(global);

	XCALL_INIT;
    if ( version != LWDISPLACEMENT_VERSION ) return AFUNC_BADVERSION;

	local->panel	= SetupDisplacementXPanel(local->inst);		// XPanel ...
	local->options	= 0;
	local->command	= 0;

	return AFUNC_OK;
};

extern void *getChannelFromGroup(LC_Instance, int );
static LC_Channel _get_channel( LC_Instance inst, int index )
{
	return (LC_Channel)getChannelFromGroup(inst,index);
};

static LC_FLAVOR _get_flavor( LC_Channel chan )
{
	return ((LC_cChannel *)chan)->getFlavor();
};

static char *_get_name( LC_Channel chan )
{
	return ((LC_cChannel *)chan)->getName();
};

static LC_VertexOffset *_get_vertex_at_frame	( LC_Channel chan, int vtx, int frame )
{
	return ((LC_cChannel *)chan)->getVertexAtFrame(vtx,frame);
}

static LC_ObjectPosition *_get_object_at_frame	( LC_Channel chan, int frame )
{
	return ((LC_cChannel *)chan)->getObjectAtFrame(frame);
}

static LWMRBExportFuncs _xprtFuncs = {
	&_get_activation,
	&_get_interface,

	&_get_channel,
	&_get_flavor,
	&_get_name,

	&_get_vertex_at_frame,
	&_get_object_at_frame
};

void SetupGlobalFuncs( LWGlobalService *local )
{
   ((LWGlobalService *)local)->data = &_xprtFuncs;
}
