/**************************************
 *
 *  GUI.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Just gets info from the user
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

#include "sys_math.h"

extern "C"
{
#include "ImportMD2.h"
}
#include "lwxpanel.h"
#include "lwrender.h"
#include "lwenvel.h"

enum { ID_FRAME = 0x8000, ID_MAKEMORPH};

#define STR_PickFrame_TEXT		"Choose Base Frame: "
#define STR_MakeMorph_TEXT		"Frames as Endomorphs"

// Open a panel with a filename requestor on it, and fire away
GUIData *Get_FrameNum(int range)
{
	LWXPanelFuncs *xpanFuncs = 
			(LWXPanelFuncs *)LW_globalFuncs( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT  );

	static LWXPanelControl xctl[] = {
		{ ID_FRAME,		STR_PickFrame_TEXT,	"integer" },
		{ ID_MAKEMORPH,	STR_MakeMorph_TEXT,	"iBoolean" },
		{ 0 }
	};

	static LWXPanelDataDesc xdata[] = {
		{ ID_FRAME,		STR_PickFrame_TEXT,	"integer" },
		{ ID_MAKEMORPH,	STR_MakeMorph_TEXT,	"integer" },
		{ 0 }
	};

	LWXPanelHint xhint[] = {
		XpLABEL(0,"MD2 Import - build "BUILD_VERSION1(PROG_PATCH_VER)),
		XpMIN(ID_FRAME,0),
		XpMAX(ID_FRAME,range),
		XpEND
	};

	LWXPanelID xpanid = xpanFuncs->create( LWXP_FORM, xctl );
	if ( !xpanid ) return 0;

	GUIData *data = new GUIData;
	memset(data,0,sizeof(GUIData));

	// Init the form
	xpanFuncs->hint( xpanid, 0, xhint );
	xpanFuncs->describe( xpanid, xdata, 0, 0 );
	xpanFuncs->formSet( xpanid, ID_FRAME, &(data->frame) );
	xpanFuncs->formSet( xpanid, ID_MAKEMORPH, &(data->makemorphs) );

	// Open modally
	xpanFuncs->post(xpanid);

	// get values
	data->frame = *(int *)xpanFuncs->formGet( xpanid, ID_FRAME );
	data->makemorphs = *(int *)xpanFuncs->formGet( xpanid, ID_MAKEMORPH );

	// Kill the XPanel
	xpanFuncs->destroy(xpanid);

	return data;
}
