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
#include "MorfToFrame.h"
}
#include "lwxpanel.h"
#include "lwrender.h"
#include "lwenvel.h"

#define _BASE_COUNT		256

class ScanHelper
{
	int checkPoint(LWPntID pntid);
	static int pointScanner(void *maps, LWPntID pntid);

	LWMeshInfoID	mesh;

	int		done;

	// Available group names (part before the '.')
	int		foundcount;
	char	foundlens[_BASE_COUNT][256];
	char	foundnames[_BASE_COUNT][256];

public:
	ScanHelper():
		mesh(0),
		done(0),
		foundcount(0)
	{
		memset(foundnames,0,sizeof(foundnames));
	}

	char *getGroup(int i)
	{
		if (i > foundcount)
			return 0;
		return &(foundnames[i][0]);
	}
	char *getGroupLen(int i)
	{
		if (i > foundcount)
			return 0;
		return &(foundlens[i][0]);
	}
	int scan(LWMeshInfo *m)
	{
		if (m == 0)
			return 0;
		mesh = m;
		mesh->scanPoints(mesh,&ScanHelper::pointScanner,this);
		return foundcount;
	}
	int getGroupCount()
	{
		return foundcount;
	}
};

int ScanHelper::pointScanner (void *maps, LWPntID pntid)
{
	return ((ScanHelper*)maps)->checkPoint(pntid);
}

int ScanHelper::checkPoint (LWPntID pntid)
{
	int newones[_BASE_COUNT] = {0};

	LWFVector vec;
	LWObjectFuncs *objFunc =
			(LWObjectFuncs *)LW_globalFuncs( LWOBJECTFUNCS_GLOBAL, GFUSE_TRANSIENT );
	int mcount = objFunc->numVMaps(LWVMAP_MORF);
	for (int midx = 0; midx < mcount; midx++)
	{
		const char *name = objFunc->vmapName(LWVMAP_MORF,midx);

		// See if this point has it
		void *vmap = mesh->pntVLookup(mesh,LWVMAP_MORF,name);
		if (vmap == 0)
			continue;
		if (mesh->pntVGet(mesh,pntid,vec) == 0)
			continue;

		// Copy the name so we can ruin it later
		strcpy(foundnames[foundcount],name);

		// Strip to a potential '.'
		for (char *c = &(foundnames[foundcount][0]); *c; c++)
		{
			if (*c != '.')
				continue;
			*c = 0;
			break;
		}

		// see if we know this one
		for (int j = 0; j < foundcount; j++)
		{
			if (strcmp(foundnames[foundcount],
					foundnames[j]) == 0)
			break;
		}
		if (j == foundcount)
		{
			newones[foundcount] = 1;
			foundcount++;
		}
		else if (newones[foundcount] > 0)
			newones[foundcount]++;
	}
	foundnames[foundcount][0] = 0;

	for (int fidx = 0; fidx < foundcount; fidx++)
	{
		if (newones[foundcount] == 0)
			continue;
		sprintf(foundlens[fidx],
					"%d frames at ",
					newones[foundcount]);
	}

	return 0;
}

enum {	TABS_MAIN = 0x8000,
		STARTID_INTS = 0x8100,
		STARTID_ONOFF = 0x8200,
		GROUPID_START = 0x8300};

// Open a panel with a requestor on it
// Requestor has a 'choose' and offset for each morf set
// It's dynamic that way
GUIData *Get_BuildData(LWLayoutGeneric *local)
{
	int i = 0;
	LWXPanelFuncs *xpanFuncs = 
			(LWXPanelFuncs *)LW_globalFuncs( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT  );
	LWItemInfo *itemInfo = 
			(LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	// Find all the valid Morf map 'groups'
	ScanHelper *maps = new ScanHelper();
	LWObjectInfo *objInfo =
			(LWObjectInfo *)LW_globalFuncs( LWOBJECTINFO_GLOBAL, GFUSE_TRANSIENT );

	// Get selected item
    LWInterfaceInfo *intInfo =
			(LWInterfaceInfo *)LW_globalFuncs( LWINTERFACEINFO_GLOBAL, GFUSE_TRANSIENT );
	for (i = 0; intInfo->selItems[i]; i++)
	{
		const LWItemID SelID = intInfo->selItems[0];
		if (itemInfo->type(SelID) != LWI_OBJECT)
			continue;
		maps->scan(objInfo->meshInfo(SelID,0));
	}

	// Find the appropriate channels
/*
	LWChanGroupID groups[256];
	LWChanGroupID group = itemInfo->chanGroup(SelID);

	LWChannelInfo *chanInfo =
			(LWChannelInfo *)LW_globalFuncs( LWCHANNELINFO_GLOBAL, GFUSE_TRANSIENT );
	while (group != (LWChanGroupID)0)
	{
		const char *gname = chanInfo->groupName(group);
		LWChannelID chan = chanInfo->nextChannel(group,0);
		while (chan != (LWChannelID)0)
		{
			const char *cname = chanInfo->channelName(chan);
			chan = chanInfo->nextChannel(0,chan);
		}
		group = chanInfo->nextGroup(0,group);
	}
*/

	char *choices[3] = {"off","on",0};

	// Tab for each group?


	// 1 row for each xctl
	LWXPanelControl xctl[_BASE_COUNT * 2] = {0};
	int ctls = 0;
	LWXPanelDataDesc xdata[_BASE_COUNT * 2] = {0};
	int descs = 0;
	LWXPanelHint xhint[_BASE_COUNT * 16] = {XpEND};
	int hints = 0;

/*
		// Model group
		XpGROUP_(GROUPID_MODEL),
			XpH(ID_MODEL_NAME),
			XpH(ID_CHK_MODEL_NAME),
			XpH(ID_ANCHOR_NAME),
		XpEND,
		XpLABEL(GROUPID_MODEL,"Model Setup"),
*/


	int j = 0;
	for (i = 0; i < maps->getGroupCount(); i++)
	{
		int xid = 0;

		// Group for each
		xid = GROUPID_START + i;
		xhint[hints++] = XPTAG_GROUP;
		xhint[hints++] = XpH(xid);

		// Add selection buttons w/ name
		xid = STARTID_ONOFF + i;
		xctl[ctls].cid = xid;
		xctl[ctls].label = "Keyframe morfs";
		xctl[ctls].ctrlclass = "iBoolean";
		++ctls;

		xdata[descs].vid = xid;
		xdata[descs].name = "off";
		xdata[descs].datatype = "integer";
		++descs;

		xhint[hints++] = XpH(xid);

		// Add int selector
		xid = STARTID_INTS + i;
		xctl[ctls].cid = xid;
		xctl[ctls].label = "First frame";
		xctl[ctls].ctrlclass = "integer";
		++ctls;

		xdata[descs].vid = xid;
		xdata[descs].name = "off";
		xdata[descs].datatype = "integer";
		++descs;

		xhint[hints++] = XpH(xid);

		// end and label the group
		xhint[hints++] = XpEND;

		xhint[hints++] = XPTAG_LABEL;
		xhint[hints++] = XpH(GROUPID_START + i);
		xhint[hints++] = XpH(maps->getGroup(i));
		xhint[hints++] = XpEND;
	}

	// Designate them as tabs
/*
		XpTABS_(TABS_MAIN),
			XpH(GROUPID_MODEL),
			XpH(GROUPID_CFG),
		XpEND,
*/
	xhint[hints++] = XPTAG_TABS;
	xhint[hints++] = XpH(TABS_MAIN);
	for (i = 0; i < maps->getGroupCount(); i++)
	{
		xhint[hints++] = XpH(GROUPID_START + i);
	}
	xhint[hints] = XpEND;

	LWXPanelID xpanid = xpanFuncs->create( LWXP_FORM, xctl );
	if ( !xpanid ) return 0;

	GUIData *data = new GUIData;
	memset(data,0,sizeof(GUIData));

	// Init the form
	xpanFuncs->hint( xpanid, 0, xhint );
	xpanFuncs->describe( xpanid, xdata, 0, 0 );

	int temp = 0;
	for (i = 0; i < maps->getGroupCount(); i++)
	{
		xpanFuncs->formSet( xpanid, STARTID_INTS +i, &temp );
		xpanFuncs->formSet( xpanid, STARTID_ONOFF +i, &temp );
	}

	// Open modally
	xpanFuncs->post(xpanid);

	// get values
//	data->frame = *(int *)xpanFuncs->formGet( xpanid, ID_FRAME );
//	data->makemorphs = *(int *)xpanFuncs->formGet( xpanid, ID_MAKEMORPH );

	// Kill the XPanel
	xpanFuncs->destroy(xpanid);

	return data;
}
