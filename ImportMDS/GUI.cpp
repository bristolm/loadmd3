#include <stdio.h>
#include <string.h>

#include "lwxpanel.h"
#include "lwrender.h"
#include "lwenvel.h"

extern "C"
{
#include "ImportMDS.h"
}

#include "ParseMDS.h"

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
GUIData *GetDataFromUser(MDS& mdl)
{
	int			range = mdl.FrameCount();

	unsigned int idx = 0;
	char		**tags = new char *[mdl.TagCount() + 2];
	tags[idx++] = "--- No Anchor Tag ---";
	for (unsigned int i = 0; i < mdl.TagCount(); i++)
	{
		tags[idx++] = mdl.Tag(i).Name;
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

/*
int GetDataFromUser(MD3& mdl)
{	// Stash tag for a list
	int			i = mdl.TagCount();
	LWPanelID	panID;

	MeshNames[i +1] = 0;
	for (;i > 0; i--)
		MeshNames[i] = (mdl.TagsAtFrame(0)[i - 1].Name);
	MeshNames[0] = "None";

	// Set up a buffer for printing the filename (for display)
	const char	*PanelTxt[4];
	PanelTxt[0] = mdl.FileName;
	PanelTxt[1] = 0;

	const char	*NoTags[2];
	NoTags[0] = "--- No Tags ---";
	NoTags[1] = 0;

	
	const char *c = (const char *)NULL;
	i = 0;
	
	// Ask the user which frame
	// Ask the user which existing Tag we should anchor it to
	if( panID=PAN_CREATE(CurrentData->Panel,"Quake MD3 file Import - v" PROG_VERSION) )
	{
		// Display the filename we're working with
		c1 = TEXT_CTL(CurrentData->Panel,panID,"",PanelTxt);

		// Center it
		int x = 0, y = 0;
		int CtlLeft = CON_X(c1);
		int CtlWidth = CON_W(c1);
		if (CtlWidth < GOOD_PANEL_WIDTH)
		{
			x = CtlWidth;
			CtlWidth = GOOD_PANEL_WIDTH;
			CtlLeft = CtlLeft + (CtlWidth - x)/2;

			y = CON_Y(c1);
			MOVE_CON(c1,CtlLeft,y);
		}

		// Ask for the frame to pull the structure from
		sprintf(tmpbuf,"Choose Frame (0 - %d):",mdl.FrameCount() -1);
		c2 = MINISLIDER_CTL(CurrentData->Panel,panID,tmpbuf,60,0,mdl.FrameCount() -1);
		SET_INT(c2,CurrentData->FrameForImport);

		if (mdl.FrameCount() == 1)
		{
			GHOST_CON(c2);
		}

		// List of tag meshes
		if (mdl.TagCount() > 0)
			c3 = POPUP_CTL(CurrentData->Panel,panID,"Anchor with:",MeshNames);
		else
			c3 = TEXT_CTL(CurrentData->Panel,panID,"",NoTags);


		int h = PAN_GETH(CurrentData->Panel,panID);

		// "Build Surface?" checkbox and size box
		if (CurrentData->LoadSkinVerts >= 0)
		{
			c4 = BOOLBUTTON_CTL(CurrentData->Panel,panID," Build SkinMesh ");
			// Reposition it a little
			y = CON_Y(c2);
			x = 8 + CtlWidth;
			x = x - CON_W(c4);
			MOVE_CON(c4,x,y);

			// If there are no tags, just guess where to put the skin mesh thing...
			y = CON_Y(c3);
			x = CON_X(c4);

			c5 = FLOAT_CTL(CurrentData->Panel,panID,"Mesh Scale:");
			if (CurrentData->ScaleForSkinMesh < 0.0)
				CurrentData->ScaleForSkinMesh = 0.0;

			SET_FLOAT(c5,CurrentData->ScaleForSkinMesh);

			// Reposition it a little (under the button)
			MOVE_CON(c5,x,y);
		}

		PAN_SETH(CurrentData->Panel,panID,h);
		PAN_SETW(CurrentData->Panel,panID,CtlWidth + 15);
	}
	else
	{
		PAN_KILL(CurrentData->Panel,panID);
		return AFUNC_BADGLOBAL;
	}

	// Now open up the panel ... if we need to
	if (mdl.FrameCount() > 1 
		|| mdl.TagCount() > 0 
		|| CurrentData->LoadSkinVerts >= 0)
	{
		int t = CurrentData->Panel->open(panID,PANF_CANCEL|PANF_BLOCKING);
		if (t == 0)
			return AFUNC_BADGLOBAL;

		// Now grab the data out
		GET_INT(c2,CurrentData->FrameForImport);

		if (mdl.TagCount() > 0)
			GET_INT(c3,CurrentData->AnchorTagIndex);
		
		PAN_KILL(CurrentData->Panel,panID);

		if (CurrentData->LoadSkinVerts >= 0)
		{
			GET_INT(c4,CurrentData->LoadSkinVerts);
			GET_FLOAT(c5,CurrentData->ScaleForSkinMesh);
		}
	}
	else
	{
		CurrentData->FrameForImport = 0;
	}

	return AFUNC_OK;
}
*/