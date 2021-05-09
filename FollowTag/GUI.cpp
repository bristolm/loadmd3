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
#include "FollowTag.h"
}

/* Popup choice */
int CountObjs  ( void *userdata )
{
	PlugInstance *data = (PlugInstance *)userdata;

	LWItemInfo *LW_itemInfo
		= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	int i = 0;
	LWItemID	ID = LW_itemInfo->first(LWI_OBJECT,LWITEM_NULL );
	while (ID != LWITEM_NULL)
	{	// Don't reference ourselves!
		if (ID != OWNER_ID(data))
			++i;
		ID = LW_itemInfo->next(ID );
	}

	return i +1;
}

int IndexByObjID ( PlugInstance *data, LWItemID objID )
{
	int idx = 0;
	if (objID == LWITEM_NULL)
		return idx;
	++idx;

	LWItemInfo *LW_itemInfo
		= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	LWItemID	ID = LW_itemInfo->first(LWI_OBJECT,LWITEM_NULL );
	while (ID != LWITEM_NULL)
	{	// Don't reference ourselves!
		if (ID == objID)
			break;
		if (ID != OWNER_ID(data))
			++idx;
		ID = LW_itemInfo->next(ID );
	}

	return idx;

}

LWItemID ObjIDByIndex ( PlugInstance *data, int idx )
{
	if (idx == 0)
		return LWITEM_NULL;

	--idx;
	
	LWItemInfo *LW_itemInfo
		= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	int i = 0;
	LWItemID	ID = LW_itemInfo->first(LWI_OBJECT,LWITEM_NULL );
	while (ID != LWITEM_NULL)
	{	// Don't reference ourselves!
		if (ID != OWNER_ID(data))
			++i;
		if (i > idx)
			break;
		ID = LW_itemInfo->next(ID );
	}

	return ID;
}

const char *ObjNameByIndex ( void *userdata, int idx )
{
	PlugInstance *data = (PlugInstance *)userdata;

	LWItemID ID = ObjIDByIndex(data,idx);

	if (ID == LWITEM_NULL)
		return "(none)";
	
	LWItemInfo *LW_itemInfo
		= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	return LW_itemInfo->name(ID);
}

enum { ID_OBJLIST = 0x8000, ID_TAGNAME};

#define STR_ObjList_TXT		"Target Object: "
#define STR_TagName_TXT		"Tag to move with: "

static void *xgetval( void *inst, unsigned long vid )
{
	void *v = 0;
	LWItemInfo *LW_itemInfo
		= (LWItemInfo *)LW_globalFuncs( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );

	PlugInstance *data = (PlugInstance *)inst;
	switch ( vid ) {
	// Overall values
      case ID_OBJLIST:		// want the index for this ID
		data->lasttargetidx = IndexByObjID(data,TARGET_ID(data));
		v = &(data->lasttargetidx);
		break;

      case ID_TAGNAME:
		v = &(data->tagname[0]);
		break;
	}

	return v;
}

static LWXPRefreshCode xsetval( void *inst, unsigned long vid, void *value )
{
	PlugInstance *data = (PlugInstance *)inst;

	switch ( vid ) {
	// Overall values
      case ID_OBJLIST:		// stash the ID for this index
		data->lasttargetidx = *(int *)value;
		strcpy(data->lasttargetname,
					ObjNameByIndex(data,data->lasttargetidx));
		TARGET_ID(data) = ObjIDByIndex(data,data->lasttargetidx);
		break;

      case ID_TAGNAME:
		strcpy((char *)data->tagname, (char *)value);
		break;
	}

	return LWXPRC_DFLT;
}

void FindObjectAfterLoading(PlugInstance * data)
{
	data->lasttargetidx = 0;
	for (int i = CountObjs(data) -1; i >= 0; i--)
	{
		const char *c = ObjNameByIndex(data,i);
		if (c && strcmp(c,data->lasttargetname) == 0)
		{
			TARGET_ID(data) = ObjIDByIndex(data,i);
			data->lasttargetidx = i;
			break;
		}
	}
}

// Open a panel with a filename requestor on it, and fire away
LWXPanelID GetPanel(LWInstance *inst)
{
	PlugInstance *data = (PlugInstance *)inst;

	// See if we've got a placeholder from startup
	if (data->lasttargetidx == -1)
	{
		FindObjectAfterLoading(data);
	}

	LWXPanelFuncs *LW_xpanFuncs = 
			(LWXPanelFuncs *)LW_globalFuncs( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT  );

	static LWXPanelControl xctl[] = {
		{ ID_TAGNAME,	STR_TagName_TXT,	"string" },
		{ ID_OBJLIST,	STR_ObjList_TXT,	"iPopChoice" },
		{ 0 }
	};

	static LWXPanelDataDesc xdata[] = {
		{ ID_TAGNAME,	STR_TagName_TXT,	"string" },
		{ ID_OBJLIST,	STR_ObjList_TXT,	"integer" },
		{ 0 }
	};

	LWXPanelHint xhint[] = {
		XpLABEL(0,"FollowTag - build "BUILD_VERSION1(PROG_PATCH_VER)),
		XpPOPFUNCS(ID_OBJLIST,CountObjs,ObjNameByIndex),
		XpDLOGTYPE(LWXPDLG_OKONLY),
		XpEND
	};

	LWXPanelID xpanid = LW_xpanFuncs->create( LWXP_VIEW, xctl );
	if ( !xpanid ) return 0;

	// Init the form
	LW_xpanFuncs->hint( xpanid, 0, xhint );
	LW_xpanFuncs->describe( xpanid, xdata, xgetval, xsetval );
	LW_xpanFuncs->viewInst( xpanid, data );
	LW_xpanFuncs->setData( xpanid, 0, data);
	LW_xpanFuncs->setData( xpanid, ID_OBJLIST, data);

	// Init the data
	data->xpan = xpanid;

	return xpanid;
}
