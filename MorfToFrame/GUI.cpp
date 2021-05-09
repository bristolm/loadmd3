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

LWXPanelFuncs *xpanFuncs = 0;

class ScanHelper
{
	int checkPoint(LWPntID pntid);
	static int pointScanner(void *maps, LWPntID pntid);

	void seedValues(LWXPanelID xpan);

	LWItemID		curID;
	LWMeshInfoID	curMesh;

	int		done;

	// Available group names (part before the '.')
	int			foundcount;
	LWItemID	foundids[_BASE_COUNT];
	int			foundnum[_BASE_COUNT];
	char		foundlens[_BASE_COUNT][256];
	char		foundnames[_BASE_COUNT][256];

public:
	ScanHelper():
		curID(LWITEM_NULL),
		curMesh(0),
		done(0),
		foundcount(0)
	{
		memset(foundnames,0,sizeof(foundnames));
		memset(foundlens,0,sizeof(foundlens));
		memset(foundnum,0,sizeof(foundnum));
		memset(foundids,0,sizeof(foundids));
	}

	static void SeedValues(LWXPanelID pan, int cid)
	{
		ScanHelper *tmp = (ScanHelper *)xpanFuncs->getData(pan,cid);
		tmp->seedValues(pan);
	}

	int getGroupCount()
	{
		return foundcount;
	}

	LWItemID getObjectID(int i)
	{
		if (i > foundcount)
			return LWITEM_NULL;
		return foundids[i];
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

	int scanForMorphGroups(LWItemID id, LWMeshInfo *m)
	{
		if (m == 0)
			return 0;
		curMesh = m;	curID = id;
		curMesh->scanPoints(curMesh,&ScanHelper::pointScanner,this);
		curMesh = 0;	curID = LWITEM_NULL;
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
		void *vmap = curMesh->pntVLookup(curMesh,LWVMAP_MORF,name);
		if (vmap == 0)
			continue;
		if (curMesh->pntVGet(curMesh,pntid,vec) == 0)
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
			if (curID == foundids[j]
				&& strcmp(foundnames[foundcount],foundnames[j]) == 0)
				break;
		}
		if (j == foundcount)
		{
			foundids[foundcount] = curID;
			newones[foundcount] = 1;
			foundcount++;
		}
		else if (newones[j] > 0)
			newones[j]++;
	}
	foundnames[foundcount][0] = 0;
	foundlens[foundcount][0] = 0;

	// Set up our counts
	for (int fidx = 0; fidx < foundcount; fidx++)
	{
		if (newones[fidx] > 0)
		{
			foundnum[fidx] = newones[fidx];
			sprintf(&(foundlens[fidx][0]),
					"%d frames",
					newones[fidx]);
		}
	}

	return 0;
}

// These need to be in the same order as the *_template arrays
enum {
	TYPE_LENS = 0,
	TYPE_ONOFF,
	TYPE_INTS,	_TYPE_EACH_MAX = TYPE_INTS,

	TYPE_ANIMLIST,

	TYPE_SEEDBUTN,
	TYPE_SEEDTYPE,
	TYPE_SEEDVAL,
	_TYPE_MAX
};

enum {
	GROUPID_GLOBAL = 0x8000, GROUPID_ONOFF, GROUPID_INTS, GROUPID_LENS,
	ID_ANIMLIST, 
	ID_SEEDBUTTON, ID_SEEDTYPE, ID_SEEDVALUE, 
	START_IDS = 0x8100
};

#define _XID(type,num)	(START_IDS + (type * 0x100) + num)

#define STR_ChkBox_TXT			"Keyframe morfs"
#define STR_FirstFrame_TXT		"Starting at"
#define STR_LenField_TXT		"Length of Animation"
#define STR_AnimList_TXT		"Animation"
#define STR_SeedBtn_TXT			"Seed All Active Animations"
#define STR_SeedType_TXT		"Buffer Method"
#define STR_SeedVal_TXT			"Buffer Value"

// Template XPanel control array
static LWXPanelControl xctl_template[] = {
	{TYPE_LENS,		STR_LenField_TXT,	"sInfo"},
	{TYPE_ONOFF,	STR_ChkBox_TXT,		"iBoolean"},
	{TYPE_INTS,		STR_FirstFrame_TXT,	"integer"},

	{ID_ANIMLIST,	STR_AnimList_TXT,	"iPopChoice"},

	{ID_SEEDBUTTON,	STR_SeedBtn_TXT,	"vButton"},
	{ID_SEEDTYPE,	STR_SeedType_TXT,	"iChoice"},
	{ID_SEEDVALUE,	STR_SeedVal_TXT,	"integer"},

	{0}
};

// Template XPanel data array
static LWXPanelDataDesc xdata_template[] = {
	{TYPE_LENS,		STR_LenField_TXT,	"string"},
	{TYPE_ONOFF,	STR_ChkBox_TXT,		"integer"},
	{TYPE_INTS,		STR_FirstFrame_TXT,	"integer"},

	{ID_ANIMLIST,	STR_AnimList_TXT,	"integer"},

	{ID_SEEDBUTTON,	STR_SeedBtn_TXT,	"integer"},
	{ID_SEEDTYPE,	STR_SeedType_TXT,	"integer"},
	{ID_SEEDVALUE,	STR_SeedVal_TXT,	"integer"},

	{0}
};

// Fill in seed values for the controls
void ScanHelper::seedValues(LWXPanelID xpan)
{
	// Find out how we are counting ... 
	int seedType = *(int *)xpanFuncs->formGet(xpan, ID_SEEDTYPE);
	int seedVal = *(int *)xpanFuncs->formGet(xpan, ID_SEEDVALUE);

	int didone = 0;
	int nextval = 0;

	// For each one, see if it counts, and set the value
	for (int i = 0; i < foundcount; i++)
	{
		if (*(int *)xpanFuncs->formGet(xpan, _XID(TYPE_ONOFF,i)) == 0)
			continue;

		// if this is the first one, use it's frame as the start
		if (didone == 0)
		{
			nextval = *(int *)xpanFuncs->formGet( xpan, _XID(TYPE_INTS,i));
			didone = 1;
		}
		else
		{	// set this one
			xpanFuncs->formSet( xpan, _XID(TYPE_INTS,i), &nextval );
		}

		// advance the nextval
		nextval += foundnum[i];
		if (seedType)
		{	// roundup buffer
			if (seedVal > 1)
			{
				int moddiff = nextval % seedVal;
				nextval = nextval - moddiff + seedVal;
			}
		}
		else
		{	// abs buffer
			nextval += seedVal;
		}
	}

}

// Seed type values
static char *SeedTypes[] = {"Absolute","Round-Up",0};

// Open a panel with a requestor on it
// Requestor has a 'choose' and offset for each morf set
// It's dynamic that way
FrameInstructions **Get_BuildData(LWLayoutGeneric *local)
{
	int i = 0;

	xpanFuncs = 
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
		const LWItemID SelID = intInfo->selItems[i];
		if (itemInfo->type(SelID) != LWI_OBJECT)
			continue;
		maps->scanForMorphGroups(SelID,objInfo->meshInfo(SelID,0));
	}

	LWXPanelControl xctl[_BASE_COUNT * 2] = {0};
	int ctls = 0;

	LWXPanelDataDesc xdata[_BASE_COUNT * 2] = {0};
	int datas = 0;

	// setup the stacks
	LWXPanelHint stacks[_TYPE_MAX][_BASE_COUNT +4] = {XpEND};

	stacks[TYPE_INTS][0] = XPTAG_STACK;
	stacks[TYPE_INTS][1] = XpH(GROUPID_INTS);
	stacks[TYPE_INTS][2] = XpH(ID_ANIMLIST);
	stacks[TYPE_INTS][3] = XpEND;

	stacks[TYPE_ONOFF][0] = XPTAG_STACK;
	stacks[TYPE_ONOFF][1] = XpH(GROUPID_ONOFF);
	stacks[TYPE_ONOFF][2] = XpH(ID_ANIMLIST);
	stacks[TYPE_ONOFF][3] = XpEND;

	stacks[TYPE_LENS][0] = XPTAG_STACK;
	stacks[TYPE_LENS][1] = XpH(GROUPID_LENS);
	stacks[TYPE_LENS][2] = XpH(ID_ANIMLIST);
	stacks[TYPE_LENS][3] = XpEND;

	int stackidx = 4;

/*
		// Model group
		XpGROUP_(GROUPID_MODEL),
			XpH(ID_MODEL_NAME),
			XpH(ID_CHK_MODEL_NAME),
			XpH(ID_ANCHOR_NAME),
		XpEND,
		XpLABEL(GROUPID_MODEL,"Model Setup"),
*/

	char **animnames = new char*[maps->getGroupCount() +1];
	int j = 0;
	for (i = 0; i < maps->getGroupCount(); i++)
	{
		// Add in the control data
		for (j = 0; j <= _TYPE_EACH_MAX;j++)
		{
			xctl[ctls].cid =		_XID(xctl_template[j].cid,i);
			xctl[ctls].label =		xctl_template[j].label;
			xctl[ctls].ctrlclass =	xctl_template[j].ctrlclass;

			// Put it into the stacks
			stacks[xctl_template[j].cid][stackidx + i] = XpH(xctl[ctls].cid);

			ctls++;
		}

		// Add in the desc data
		for (j = 0; j <= _TYPE_EACH_MAX;j++)
		{
			xdata[datas].vid =		_XID(xdata_template[j].vid,i);
			xdata[datas].name =		xdata_template[j].name;
			xdata[datas].datatype =	xdata_template[j].datatype;
			datas++;
		}

		// remember the name
		animnames[i] = maps->getGroup(i);
	}
	animnames[i] = 0;

	// And the string list
	xctl[ctls++] = xctl_template[TYPE_ANIMLIST];
	xdata[datas++] = xdata_template[TYPE_ANIMLIST];

	// Add in a button, type buttons, and an int selector
	xctl[ctls++] = xctl_template[TYPE_SEEDBUTN];
	xdata[datas++] = xdata_template[TYPE_SEEDBUTN];

	xctl[ctls++] = xctl_template[TYPE_SEEDTYPE];
	xdata[datas++] = xdata_template[TYPE_SEEDTYPE];

	xctl[ctls++] = xctl_template[TYPE_SEEDVAL];
	xdata[datas++] = xdata_template[TYPE_SEEDVAL];

	// Let's do a dropdown list with the names of the animations
	LWXPanelHint xhint [] = {
		XpLABEL(0,"MorfToFrame - build "BUILD_VERSION1(PROG_PATCH_VER)),
		XpGROUP_(GROUPID_GLOBAL),
			XpH(ID_SEEDBUTTON),
			XpH(ID_SEEDTYPE),
			XpH(ID_SEEDVALUE),
			XpH(ID_ANIMLIST),
		XpEND,

		XpMIN(ID_SEEDVALUE,0),
		XpBUTNOTIFY(ID_SEEDBUTTON,ScanHelper::SeedValues),
		XpSTRLIST(ID_SEEDTYPE,SeedTypes),

		// try a divider
		XpDIVADD(ID_SEEDVALUE),

		// --------------------------------

		// pull in the dynamic stuff
		XpSTRLIST(ID_ANIMLIST,animnames),

		XpCALL(stacks[TYPE_LENS]),
		XpLABEL(GROUPID_LENS,STR_LenField_TXT),

		XpCALL(stacks[TYPE_ONOFF]),
		XpLABEL(GROUPID_ONOFF,STR_ChkBox_TXT),

		XpCALL(stacks[TYPE_INTS]),
		XpLABEL(GROUPID_INTS,STR_FirstFrame_TXT),

		XpEND
	};

	LWXPanelID xpanid = xpanFuncs->create( LWXP_FORM, xctl );
	if ( !xpanid ) return 0;

	GUIData *data = new GUIData;
	memset(data,0,sizeof(GUIData));

	// Init the form
	xpanFuncs->hint( xpanid, 0, xhint );
	xpanFuncs->describe( xpanid, xdata, 0, 0 );

	// Init the form values
	int temp = 0;
	xpanFuncs->formSet( xpanid, 0, maps );
	xpanFuncs->formSet( xpanid, ID_ANIMLIST, &temp );
	for (i = 0; i < maps->getGroupCount(); i++)
	{
		xpanFuncs->formSet( xpanid, _XID(TYPE_INTS,i), &temp );
		xpanFuncs->formSet( xpanid, _XID(TYPE_ONOFF,i), &temp );
		xpanFuncs->formSet( xpanid, _XID(TYPE_LENS,i),
								(void *)(maps->getGroupLen(i)) );
	}
	xpanFuncs->setData( xpanid, ID_SEEDBUTTON, maps );
	xpanFuncs->formSet( xpanid, ID_SEEDTYPE, &temp );
	xpanFuncs->formSet( xpanid, ID_SEEDVALUE, &temp );

	// Open modally
	xpanFuncs->post(xpanid);

	FrameInstructions **target= new FrameInstructions *[maps->getGroupCount()];
	// get values
	j = 0;
	for (i = 0; i < maps->getGroupCount(); i++)
	{
		temp = *(int *)xpanFuncs->formGet( xpanid, _XID(TYPE_ONOFF,i) );
		if (temp)
		{
			target[j] = new FrameInstructions;
			target[j]->Object = maps->getObjectID(i);
			strcpy(target[j]->GroupName,maps->getGroup(i));

			target[j]->StartFrame = 
				*(int *)xpanFuncs->formGet( xpanid, _XID(TYPE_INTS,i) );

			++j;
		}
	}
	target[j] = 0;

	// Clean up
	delete animnames;

	// Kill the XPanel
	xpanFuncs->destroy(xpanid);

	return target;
}
