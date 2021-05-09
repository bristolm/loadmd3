#include <stdio.h>
#include <string.h>

#include "lwxpanel.h"
#include "lwrender.h"
#include "lwenvel.h"

extern "C"
{
#include "LoadQuakeMD.h"
}
#include "ModelMD3.h"

enum
{	ID_FRAME = 0x8000, ID_ANCHOR,

	// Morph stuff
	GROUPID_MORPH,
	ID_MORPHLIST, ID_MAKEMORPH, ID_UPPERLOWER
};

#define STR_PickFrame_TEXT		"Choose Base Frame: "
#define STR_Anchor_TEXT			"Anchor tag: "
#define STR_AnimFile_TEXT		".cfg for Morphs"
#define STR_MakeMorph_TEXT		"Frames as Endomorphs"
#define STR_UpperLower_TEXT		"Frame filter"

// Open a panel with a filename requestor on it, and fire away
GUIData *GetDataFromUser(MD3& mdl)
{
	int			range = mdl.FrameCount();

	unsigned int idx = 0;
	char		**tags = new char *[mdl.TagCount() + 2];
	tags[idx++] = "--- No Anchor Tag ---";
	for (unsigned int i = 0; i < mdl.TagCount(); i++)
	{
		tags[idx++] = mdl.TagsAtFrame(0)[i].Name;
	}
	tags[idx] = 0;

	char	*choices[4] = {"None","Upper","Lower",0};

	LWXPanelFuncs *xpanFuncs = 
			(LWXPanelFuncs *)LW_globalFuncs
								( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT  );

	static LWXPanelControl xctl[] = {
		{ ID_FRAME,		STR_PickFrame_TEXT,	"integer" },
		{ ID_ANCHOR,	STR_Anchor_TEXT,	"iPopChoice" },
		{ ID_MORPHLIST,	STR_AnimFile_TEXT,	"sFileName" },
		{ ID_UPPERLOWER,STR_UpperLower_TEXT,"iChoice" },
		{ 0 }
	};

	static LWXPanelDataDesc xdata[] = {
		{ ID_FRAME,		STR_PickFrame_TEXT,	"integer" },
		{ ID_ANCHOR,	STR_Anchor_TEXT,	"integer" },
		{ ID_MORPHLIST,	STR_AnimFile_TEXT,	"string" },
		{ ID_UPPERLOWER,STR_UpperLower_TEXT,"integer" },
		{ 0 }
	};

	LWXPanelHint xhint[] = {
		XpLABEL(0,"MD3 Import - build "BUILD_VERSION1(PROG_PATCH_VER)),
		XpMIN(ID_FRAME,0),
		XpMAX(ID_FRAME,range),
		XpSTRLIST(ID_ANCHOR,tags),
		XpSTRLIST(ID_UPPERLOWER,choices),
		XpGROUP_(GROUPID_MORPH),
			XpH(ID_MORPHLIST),
			XpH(ID_UPPERLOWER),
		XpEND,

		XpEND
	};

	GUIData *data = new GUIData;
	memset(data,0,sizeof(GUIData));

	LWXPanelID xpanid = xpanFuncs->create( LWXP_FORM, xctl );
	if ( !xpanid ) return 0;

	// Init the form
	xpanFuncs->hint( xpanid, 0, xhint );
	xpanFuncs->describe( xpanid, xdata, 0, 0 );
	xpanFuncs->formSet( xpanid, ID_FRAME, &(data->FrameForImport) );
	xpanFuncs->formSet( xpanid, ID_ANCHOR, &(data->AnchorTagIndex ) );
	xpanFuncs->formSet( xpanid, ID_MORPHLIST, &(data->AnimCFG ) );
	xpanFuncs->formSet( xpanid, ID_UPPERLOWER, &(data->ModelType ) );

	// Open modally
	xpanFuncs->post(xpanid);

	// get values
	data->FrameForImport = *(int *)xpanFuncs->formGet( xpanid, ID_FRAME );
	data->AnchorTagIndex = *(int *)xpanFuncs->formGet( xpanid, ID_ANCHOR );
	strcpy(data->AnimCFG,(char *)xpanFuncs->formGet( xpanid, ID_MORPHLIST ));
	data->ModelType = *(int *)xpanFuncs->formGet( xpanid, ID_UPPERLOWER );

	// Kill the XPanel
	xpanFuncs->destroy(xpanid);

	return data;
}

