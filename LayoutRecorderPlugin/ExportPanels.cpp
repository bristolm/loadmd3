/**************************************
 *
 *  ExportPanels.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Each instance of a ExportModelWrapper represents a single target user
 *  model.  It speaks with the module  by passing information into it through 
 *  the provided LWMRBCallbackType object (m_userfuncs) and a pointer
 *  provided by the module (m_userinfo).
 *
 *  Initiation works as follows:
 *  1)  DisplacementXPanelInstance 
 *
 *  This also maintains the FrameLoop lists - lists may be shared
 *  so much as one may take the list and order of frame loops from another 
 *  ExportModelWrapper, but the selection (checkmark) of the loops is specific
 *  to an individual ExportModelWrapper
 *
 ***************************************/


/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Mystuff */
#include "sys_base.h"
extern "C"
{
#include "LayoutRecorderPlugin.h"
}

#include "ExportPanels.h"

// required by macros in lwpanel.h
static LWPanControlDesc	desc;
static LWValue ival={LWT_INTEGER},ivecval={LWT_VINT},         
		fval={LWT_FLOAT},fvecval={LWT_VFLOAT},
		sval={LWT_STRING},xpanval={LWT_XPANEL};

static const char		*BlankBuf[4];
static char				tmp[1024];

#define PREFERRED_BUTTON_WIDTH		20

#define CANVASENTRY_HEIGHT		18

#define CANVAS_WIDTH			250
#define CANVAS_ITEMSHOWN		10
#define CANVAS_HEIGHT			(CANVASENTRY_HEIGHT * CANVAS_ITEMSHOWN)

#define CANVAS_LWLEFT			(pan_x + CANVAS_WIDTH - 150)
#define CANVAS_TARGLEFT			(pan_x + CANVAS_WIDTH - 70)

static const int CTLTS_FRAMELENGTH	= 0;
static const int CTLTS_LW_FRAMESTART	= 1;

#define LISTWIDTH_QUANTUM  (CANVAS_WIDTH/6)

// statics
int						Cache<ExportModelWrapper>::NextUniqueIndex = 0;
AutoArray<ExportModelWrapper *>
						Cache<ExportModelWrapper>::All(0);


static char		*ListTitles[] = {" ","Name","Len","LW","Target",0};
static int		ListWidths[] = {LISTWIDTH_QUANTUM / 2,
								LISTWIDTH_QUANTUM * 2,
								LISTWIDTH_QUANTUM / 2,
								LISTWIDTH_QUANTUM * 2,
								LISTWIDTH_QUANTUM,
								0};

char	*ExportModelWrapper::ColumnTitles []
				= {" ",	"Name",	"Len",	"LW",	"Target",0};
float	ExportModelWrapper::ColumnWidths []
				= {0.8f,	3.2f,	0.8f,	3.2f,	2.0f,	0};

static char		*LoadFileText = "(load file)";

// To keep track of each canvas entry regardless of name
static int			UniquePanelIndex = 0;
static int			UniqueCanvasIndex = 0;

// Generic panel functions
static void panelDrawEvent(LWPanelID pan, void *inst)
{
	// Make sure the dropdown list shows the right stuff
	((ExportModelWrapper *)inst)->readyForRedraw();
}

/*
 *  There is one instance of this frame per Lightwave Instance.
 *  All control of frames and such is done within
 */
void ExportModelWrapper::ShowEmptyText()
{
	SET_STR(cFrameStrName,"",30);
	SET_STR(cFrameStrPattern,"",5);
	SET_STR(cFrameStrOffset,"",5);

	// Send the empty frame into our 'friend' and let it know
	LoopRangeStub stub(LOOP_END);

	if (m_userfuncs->loopactivity != 0)
		m_userfuncs->loopactivity(m_userinst,stub.getLoop());
}

void ExportModelWrapper::ShowText(LoopRangeStub *l)
{
	if (l == 0) {
		ShowEmptyText();
		return;
	}
	SET_STR(cFrameStrName,l->getName(),strlen(l->getName()));
	SET_STR(cFrameStrPattern,l->getPattern(),MAX_FRAMEPATTERN_FIELD);
	SET_STR(cFrameStrOffset,l->getOffsetChar(),5);

	// Send the current frame into our 'friend' and let it know
	if (m_userfuncs->loopactivity != 0)
		m_userfuncs->loopactivity(m_userinst,l->getLoop());
}

void ExportModelWrapper::SelectActiveEntry()
{	
	// Highlight the proper row
	int i = 0;
	int idx = 0;
	LoopRange *loop = getProvidedLoopList().getRangeByID(ActiveLoopID, &idx);
	LoopRangeStub *stub = getStubByID(ActiveLoopID);

	if (loop == 0)
	{
		ShowEmptyText();
		return;
	}
	else
	{
		SET_INT(cFrameLoopList,idx);
		ShowText(stub);
	}
}

/*
 * EVENT handler functions
 */

// New output file name
static void eventFile(LWControl *ctl, void *inst)
{
	GET_STR(ctl,((ExportModelWrapper *)inst)->getFile(),256);
}

// Flush the currently built object out - all new
static void eventFileClear(LWControl *ctl, ExportModelWrapper *inst)
{
//	inst->userdata->clear(inst->userdata);
}

// Dump what we have now for this one model to disk
static void eventFileSave(LWControl *ctl, ExportModelWrapper *inst)
{
	// Setup the listeners
	ObjectWrapper::initializeAll();
	LC_FrameLoop **loops = inst->startModel();

	// Do some Setup all our frames for 'listening'
	int iMinFrame = 999;
	int iMaxFrame = -999;
	int iLoopsCounted = 0;
	for (int i = 0;loops && loops[i];i++)
	{
		++iLoopsCounted;
		if (loops[i]->lwframes == 0)
			continue;
		for (int j = 0; j < loops[i]->length; j++)
		{
			if (loops[i]->lwframes[j] < iMinFrame)
				iMinFrame = loops[i]->lwframes[j];
			if (loops[i]->lwframes[j] > iMaxFrame)
				iMaxFrame = loops[i]->lwframes[j];
		}
	}

	// Store the current values so we can reset things ...
	LWInterfaceInfo	*intfInfo
			= (LWInterfaceInfo *)LW_globalFuncs( LWINTERFACEINFO_GLOBAL, GFUSE_TRANSIENT );
	int oldFirstFrame = intfInfo->previewStart;
	int oldLastFrame = intfInfo->previewEnd;
		
	char cmdbuf[255];
	if (iLoopsCounted > 0)
	{
		// Reduce the preview window
		sprintf(cmdbuf,"PreviewFirstFrame %d",iMinFrame);
		LW_cmdFunc(cmdbuf);
		sprintf(cmdbuf,"PreviewLastFrame %d",iMaxFrame);
		LW_cmdFunc(cmdbuf);
		sprintf(cmdbuf,"MakePreview",iMaxFrame);
		LW_cmdFunc(cmdbuf);
	}

	// Setup the listeners
	ObjectWrapper::initializeAll();

	// Build the model
	inst->buildModel();

	// Reset all the listeners
	ObjectWrapper::resetAll();

	// Fix the view screen
	sprintf(cmdbuf,"PreviewFirstFrame  %d",oldFirstFrame);
	LW_cmdFunc(cmdbuf);
	sprintf(cmdbuf,"PreviewLastFrame  %d",oldLastFrame);
	LW_cmdFunc(cmdbuf);
}

/* returns a string, given 0-based indexes for the list position and 
 * column index = -1 means title
 */
char *ExportModelWrapper::getTargetStartRangeText(LoopRangeStub *lTarget)
{
	if (lTarget->getFlags() & CANVAS_FLAG_NOTSELECTED)
		return "--";

	int i = 0;
	int frame = 0;
	LoopRange *lCheck = 0;
	LoopRangeStub *stub = 0;
	LoopList& list = getProvidedLoopList();
	while ((lCheck = list.getRange(i++)) != 0)
	{
		stub = getStubByID(lCheck->getCacheID());
		if (stub == lTarget)
			break;
		if ((stub->getFlags() & CANVAS_FLAG_NOTSELECTED) == 0)
			frame += lCheck->getLength();

		stub = 0;
	}

	if (stub == 0)
	{
		stub = new LoopRangeStub(lTarget->getID());
		m_loopstubs[stub->getID()] = stub;
	}

	if (lTarget->getLength() == 0)
	{
		sprintf(stub->RangeText, "--",frame);
	}
	else if (lTarget->getLength() == 1)
	{
		sprintf(stub->RangeText,"%d",frame);
	}
	else
	{
		sprintf(stub->RangeText,"%d-%d",
					frame, frame + lTarget->getLength() -1);
	}
	return stub->RangeText;
}

// LoopList choice dropdown list functions - Use the global list
static char *listNameFnLoops( void *data, int index )
{
	if (index < 0)
		return "";

	return ExportModelWrapper::getByCacheIndex(index)->getTargetTypeAndDescription();
}

static int listCountFnLoops (void *data )
{
	return ExportModelWrapper::getCount();
}

static void eventLoops(LWControl *ctl, ExportModelWrapper *inst)
{
	int index = 0;
	GET_INT(ctl,index);

	// Loops are provided by a given model
	ExportModelWrapper *wrapper = ExportModelWrapper::getByCacheIndex(index);

	inst->setListProviderIndex(wrapper->getCacheID());
	inst->SelectActiveEntry();

	::UpdateEntireEnvironment(inst);
}

// Frame List Multilist functions
static char *listNameFnMultilist( void *data, int index, int column )
{
	ExportModelWrapper *inst = (ExportModelWrapper *)data;
	if (index < 0) {
		return ListTitles[column];
	}

	char *c = 0;
	LoopRange *loop = inst->getProvidedLoopList().getRange(index);
	if (loop == 0) {
		return c;
	}
	LoopRangeStub *stub = inst->getStubByID(loop->getCacheID());
	if (stub == 0) {
		return c;
	}

	int i = 0;
	switch (column) {
	case COL_ACTIVE:
		c = (stub->getFlags() & CANVAS_FLAG_NOTSELECTED) == 0
			? LW_USECHECKMARK : "";
		break;
	case COL_NAME:
		c = loop->getName();
		break;
	case COL_LEN:
		c = loop->getLengthChar();
		break;
	case COL_LWFRAMES:
		c = stub->getRangeChar();
		break;
	case COL_TARGET:		// count all the frames up to this one
		c = inst->getTargetStartRangeText(stub);
		break;
	}

	return c;
}

// Column Value reply functions
char *ExportListColumn_Active::getValue(LoopRangeStub *stub)
{	return (stub->getFlags() & CANVAS_FLAG_NOTSELECTED) == 0
						? LW_USECHECKMARK : "";	}
char *ExportListColumn_Name::getValue(LoopRangeStub *stub)
{	return stub->getName();}

char *ExportListColumn_Len::getValue(LoopRangeStub *stub)
{	return stub->getLengthChar();}

char *ExportListColumn_LWFrames::getValue(LoopRangeStub *stub)
{	return stub->getRangeChar();}

char *ExportListColumn_Target::getValue(LoopRangeStub *stub)
{	return getTarget()->getTargetStartRangeText(stub);}

// Column Event functions
void ExportListColumn_Active::doEvent(LoopRangeStub *stub)
{
	int flags = stub->getFlags();
	if (flags & CANVAS_FLAG_NOTSELECTED)
		stub->setFlags(flags & ~CANVAS_FLAG_NOTSELECTED);
	else
		stub->setFlags(flags | CANVAS_FLAG_NOTSELECTED);
}

static int listCountFnMultilist( void *data )
{
	if (data == 0)
	{
		return 0;
	}
	ExportModelWrapper *inst = (ExportModelWrapper *)data;
	return inst->getProvidedLoopList().getRangeCount();
}

static int listColwidthFnMultilist( void *data, int index )
{
	return ListWidths[index];
}

void eventMultilist(LWControl *ctl, ExportModelWrapper *inst)
{
	int r = 0,c = 0,i = 0;
	GET_IVEC(ctl,r,c,i);

	LoopRange *loop = inst->getProvidedLoopList().getRange(r);
	inst->ActiveLoopID = loop->getCacheID();

	LoopRangeStub *stub = inst->getStubByID(loop->getCacheID());
	inst->ShowText(stub);

	if (c == COL_ACTIVE) {	// flag/unlag
		int flags = stub->getFlags();
		if (flags & CANVAS_FLAG_NOTSELECTED)
			stub->setFlags(flags & ~CANVAS_FLAG_NOTSELECTED);
		else
			stub->setFlags(flags | CANVAS_FLAG_NOTSELECTED);
	}

	::UpdateEntireEnvironment(inst);

	// Set focus on this control

}

// Button functions
// Add an entry
static void eventFrameBtnAdd(LWControl *ctl, ExportModelWrapper *inst)
{
	inst->BuildNewCanvasEntry();
}

void ExportModelWrapper::BuildNewCanvasEntry()
{
	char	Name[30];
	GET_STR(cFrameStrName,Name,30);

	char	Pattern[5];
	GET_STR(cFrameStrPattern,Pattern,5);

	char	Offset[5];
	GET_STR(cFrameStrOffset,Offset,5);

	// Add it after the active one or the last one
	LoopRange *after = 0;
	if (ActiveLoopID >= 0)
	{
		after = getProvidedLoopList().getRangeByID(ActiveLoopID);
	}

	if (Name[0] && Pattern[0] && Offset[0])
	{
		// The Len field can have characters now.
		LoopRange *newone = new LoopRange(Name,Pattern);

		getProvidedLoopList().add(newone,after);
		ActiveLoopID = newone->getCacheID();

		LoopRangeStub *stub = getStubByID(ActiveLoopID);
		stub->setOffset(atoi(Offset));

		SelectActiveEntry();
	}

	::UpdateEntireEnvironment(0);
}

// Update an entry
static void eventFrameBtnUpd(LWControl *ctl, ExportModelWrapper *inst)
{
	inst->UpdateCanvasEntry();
}

void ExportModelWrapper::UpdateCanvasEntry()
{
	LoopRange *loop = getProvidedLoopList().getRangeByID(ActiveLoopID);
	if (loop == 0)
		return;
	LoopRangeStub *stub = getStubByID(loop->getCacheID());
	if (stub == 0)
		return;

	char	Name[30];
	GET_STR(cFrameStrName,Name,30);

	char	Pattern[MAX_FRAMEPATTERN_FIELD];
	GET_STR(cFrameStrPattern,Pattern,MAX_FRAMEPATTERN_FIELD);

	char	Offset[5]; 
	GET_STR(cFrameStrOffset,Offset,5);

	if (Name[0] && Pattern[0] && Offset[0])
	{
		strcpy(loop->getName(),Name);

		loop->setPattern(Pattern);
		stub->setOffset(atoi(Offset));
	}
	SelectActiveEntry();
	::UpdateEntireEnvironment(0);
}

// Delete an entry
static void eventFrameBtnDel(LWControl *ctl, ExportModelWrapper *inst)
{
	inst->DeleteActiveCanvasEntry();
}

void ExportModelWrapper::DeleteActiveCanvasEntry()
{
	int slot = 0;
	LoopRange *loop = getProvidedLoopList().getRangeByID(ActiveLoopID, &slot);
	if (loop == 0)
		return;

	// Send the empty frame into our 'friend' and let it know
	LoopRangeStub stub(loop->getCacheID());
	LC_FrameLoop *l = stub.getLoop();
	l->state = ACTIVITY_DELETED;

	if (m_userfuncs->loopactivity != 0)
		m_userfuncs->loopactivity(m_userinst,l);

	LoopList& list = getProvidedLoopList();

	// now remove the item
	list.remove(loop);

	// Move the active one to the next one
	loop = list.getRange(slot);
	ActiveLoopID = loop ?
		 loop->getCacheID() : slot == 0 ?
			 0 : list.getRange(slot -1)->getCacheID();

	SelectActiveEntry();
	::UpdateEntireEnvironment(0);
}

// Move an entry up
static void eventFrameBtnUp(LWControl *ctl, ExportModelWrapper *inst)
{
	inst->BumpActiveUp();
}

void ExportModelWrapper::BumpActiveUp()
{
	LoopList& list = getProvidedLoopList();
	LoopRange *l = list.getRangeByID(ActiveLoopID);
	if (l == 0)
		return;

	list.moveUp(l);

	SelectActiveEntry();
	::UpdateEntireEnvironment(0);
}

// Move an entry down
static void eventFrameBtnDown(LWControl *ctl, ExportModelWrapper *inst)
{
	inst->BumpActiveDown();
}
void ExportModelWrapper::BumpActiveDown()
{
	int slot = 0;
	LoopList& list = getProvidedLoopList();
	LoopRange *l = list.getRangeByID(ActiveLoopID,&slot);
	if (l == 0 )
		return;

	list.moveDown(l);

	SelectActiveEntry();
	::UpdateEntireEnvironment(0);
}

// Key strokes
static void eventPanelKey( LWPanelID panel, ExportModelWrapper *inst, LWDualKey key )
{
	LoopRange *loop = inst->getProvidedLoopList().getRangeByID(inst->ActiveLoopID);
	LoopRangeStub *stub = inst->getStubByID(inst->ActiveLoopID);

	switch (key)
	{
	// Move the loop
	case LWDK_PAGEUP:
		inst->BumpActiveUp();
		break;
	case LWDK_PAGEDOWN:
		inst->BumpActiveDown();
		break;

	// Change the length of the loop
	case LWDK_SC_RIGHT:
		if (loop)
			loop->setLength(loop->getLength() +1);
		break;
	case LWDK_SC_LEFT:
		if (loop && loop->getLength() > 1)
			loop->setLength(loop->getLength() -1);
		break;

	// Change the start lwframe of the loop
	case LWDK_SC_UP:
		if (stub)
			stub->setOffset(stub->getOffset() -1);
		break;
	case LWDK_SC_DOWN:
		if (stub)
			stub->setOffset(stub->getOffset() +1);
		break;
	}
	inst->SelectActiveEntry();
	::UpdateEntireEnvironment(inst);
}

#define CHECKED_NAME(ent)		\
		sprintf(ent->Name,"%s%s",LW_USECHECKMARK,itemInfo->name(ent->ID));
#define UNCHECKED_NAME(ent)		\
		sprintf(ent->Name,"%s%s",LW_NO_CHECKMARK,itemInfo->name(ent->ID));


/*
 * Run through all the plugs attached to this object
 * return an index if you find one, 0 if you don't
 */
static int findServerIndex(LWItemID item, const char *classname, const char *name)
{
	int idx = 1;			// Base 1 index
	const char *c = 0;
	for (;(c = LW_itemInfo->server(item,classname,idx)) != 0;idx++)
	{
		if (strncmp(c,LWMRBPREFIX,strlen(LWMRBPREFIX)) == 0)
		{
			return idx;
			break;
		}
	}

	return 0;
}

// LWListenPanelData NEEDS to have the XPanel filled in already
void ExportModelWrapper::initPanel()
{	// If we've already done this, don't do it again!
	if ((m_panelID = PAN_CREATE(LW_panelFuncs,"Construction Panel - v" PLUGINREV)) != 0)
	{
		PAN_SETDRAW(LW_panelFuncs, m_panelID, panelDrawEvent);

		BlankBuf[0] = "";
		BlankBuf[1] = 0;

		// Add an XPanel to interface with our 'type'
		m_userXPanelID = m_userfuncs->newxpanel(m_userinst);
		if (m_userXPanelID != (LWXPanelID) 0)
		{
			LWPanControlDesc  xdesc;
			xdesc.xpanel.type = LWT_XPANEL;
			xdesc.xpanel.width = CANVAS_WIDTH * 2;
			xdesc.xpanel.xpan = m_userXPanelID;
			cUserXPanel = XPANEL_CTL(LW_panelFuncs,
										m_panelID,
										m_userinfo->modeltype,
										m_userXPanelID);
			m_userinst = LW_xpanFuncs->getData(m_userXPanelID,0);
		}

/*
		// File selector - loop file - fill with default value
		cModelFile = FILE_CTL(LW_panelFuncs,
								m_panelID,
								"Frame List File NAme: ",
								40);
		CON_SETEVENT(cModelFile,eventFile,this);
*/
		// List of available loop templates
		cLoopLists = CUSTPOPUP_CTL(LW_panelFuncs, m_panelID ,"Frame List Provider:",
									CANVAS_WIDTH - 40,
									listNameFnLoops,
									listCountFnLoops);
		CON_SETEVENT(cLoopLists,eventLoops,this);
		CON_SETUSERDATA(cLoopLists,this);

		// Display area for frame loops
		cFrameLoopList = MULTILIST_CTL(LW_panelFuncs, m_panelID , "",
									CANVAS_WIDTH +17, 10,
									listNameFnMultilist,
									listCountFnMultilist,
									listColwidthFnMultilist
									);
		CON_SETUSERDATA(cFrameLoopList,this);
		CON_SETEVENT(cFrameLoopList,eventMultilist,this);

		// Setup buttons
		cFrameBtnAdd = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"+", PREFERRED_BUTTON_WIDTH);
		CON_SETEVENT(cFrameBtnAdd,eventFrameBtnAdd,this);

		cFrameBtnUpd = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"=", PREFERRED_BUTTON_WIDTH);
		CON_SETEVENT(cFrameBtnUpd,eventFrameBtnUpd,this);

		cFrameBtnUp = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"/\\", PREFERRED_BUTTON_WIDTH);
		CON_SETEVENT(cFrameBtnUp,eventFrameBtnUp,this);

		cFrameBtnDown = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"\\/", PREFERRED_BUTTON_WIDTH);
		CON_SETEVENT(cFrameBtnDown,eventFrameBtnDown,this);

		cFrameBtnDel = WBUTTON_CTL(LW_panelFuncs,m_panelID ,"X", PREFERRED_BUTTON_WIDTH);
		CON_SETEVENT(cFrameBtnDel,eventFrameBtnDel,this);

		// Setup test and string fields
		cFrameStrName = STR_CTL(LW_panelFuncs,m_panelID ,"",17);
		cFrameStrPattern = STR_CTL(LW_panelFuncs,m_panelID ,"",22);
		cFrameStrOffset = STR_CTL(LW_panelFuncs,m_panelID ,"@",5);

		LWControl *ctls[4] = {cFrameStrName,cFrameStrPattern,cFrameStrOffset,0};

		// Rearrange things
		int i = 0, j =0, k = 0, x = 0, y = 0;

		if (cUserXPanel)
		{
			MOVE_CON(cUserXPanel, 0, 0);
			y = CON_H(cUserXPanel);
		}

		// Dropdown list
		x = 5;
		MOVE_CON(cLoopLists,			x,		y += 5);
		y += CON_H(cLoopLists);

#ifndef CON_SETEVENT
#define CON_SETEVENT(ctl,f,d) ( (ival.intv.value=(int)d,(*ctl->set)(ctl,CTL_USERDATA,&ival)), (ival.intv.value=(int)f,(*ctl->set)(ctl,CTL_USEREVENT,&ival)) )
#endif

		// [update button] [name,lw,len field] [add]
		y += 2;
		MOVE_CON(cFrameBtnUpd,	x,				y);

		x += ListWidths[0];

		MOVE_CON(cFrameStrName,	x,				y);
		x += CON_W(cFrameStrName) -3;

		MOVE_CON(cFrameStrPattern,	x,				y);
		x += CON_W(cFrameStrPattern);

		MOVE_CON(cFrameStrOffset,x,				y);
		x += CON_W(cFrameStrOffset) -3;
		x += 2;
		
		MOVE_CON(cFrameBtnAdd,	x,				y);

		// Loop List below
		x = CON_X(cFrameStrName);
		y = CON_Y(cFrameStrName);
		y += CON_H(cFrameStrName);

		x = CON_X(cLoopLists);
		MOVE_CON(cFrameLoopList,		x,		y);

		// Buttons [up, down] down the side ... [delete]
		x += CON_W(cFrameLoopList);
		x += 1;

		MOVE_CON(cFrameBtnUp,	x,				y);
		y += CON_HOTH(cFrameBtnUp);
		MOVE_CON(cFrameBtnDown,	x,				y);
		y += CON_HOTH(cFrameBtnDown);

		y = CON_Y(cFrameLoopList);
		y += CON_H(cFrameLoopList);
		y -= CON_H(cFrameBtnDel);

		MOVE_CON(cFrameBtnDel,	x,				y);

		x += PREFERRED_BUTTON_WIDTH;
		y += CON_H(cFrameBtnDel);

		PAN_SETH(LW_panelFuncs,m_panelID ,	y);
		PAN_SETW(LW_panelFuncs,m_panelID ,	x);

		PAN_SETDATA(LW_panelFuncs,m_panelID,this);
		PAN_SETKEYS(LW_panelFuncs,m_panelID,eventPanelKey);

		// preset the loop dropdown
		SET_INT(cLoopLists,ExportModelWrapper::getByCacheID(m_listref)->getCacheIndex());
	}
}

int ExportModelWrapper::getCountByType(char *type)
{
	int iCount = 0;

	int i = getCacheSize();
	while (i-- > 0)
	{
		ExportModelWrapper *wrap = getByCacheID(i);
		if (wrap == 0)
			continue;
		if (strcmp(wrap->getTargetType(),type))
			continue;
		++iCount;
	}

	return iCount;
}

ExportModelWrapper *ExportModelWrapper::getByIndexByType(int idx, char *type) // 0 base
{
	if (idx < 0)
		return new ExportModelWrapper();

	for (int i = 0; i < getCacheSize(); i++)
	{
		ExportModelWrapper *wrap = getByCacheID(i);

		if (wrap == 0)
			continue;

		if (strcmp(wrap->getTargetType(),type))
			continue;

		if (idx-- == 0)
			return wrap;
	}
	return new ExportModelWrapper();
}

// The Array isn't 'clean' - keep track ...
int ExportModelWrapper::getIndexByType(char *type)	// 0 base
{
	if (type == 0)
		type = m_userinfo->modeltype;
	int idx = 0;
	for (int i = 0; i < getCacheSize(); i++)
	{
		ExportModelWrapper *wrap = getByCacheID(i);
		if (wrap == 0)
			continue;

		if (wrap == this)
			return idx;

		if (strcmp(wrap->getTargetType(),type))
			continue;

		++idx;
	}

	return -1;
}

void ExportModelWrapper::acquireRef(LWItemID item, LWMRBExportType *info)
{	// At load time, we pretend ...
	if (item == LWITEM_NULL)
		return;

	// Increase my references - if it's the first, stash a location func pointer too
	if (m_refs++ == 0)
	{
		if (!m_userinfo)
			m_userinfo = info;
		if (!m_userfuncs)
			m_userfuncs = (LWMRBCallbackType *)LW_globalFuncs(m_userinfo->globalcallback,GFUSE_ACQUIRE );
	}

	// Stash the group
	int i = 0;
	for (;ChannelGroup[i] != 0;i++)
		;

	ChannelGroup[i++] = ObjectWrapper::aquireWrapper(item);
	ChannelGroup[i] = 0;
}

void ExportModelWrapper::releaseRef(LWItemID item)
{	// Remove this group from our list
	unsigned int idx = 0;
	if (item != (LWItemID)0)
	{
		for (;ChannelGroup[idx] != 0;idx++)
		{
			if (ChannelGroup[idx]->getID() != item)
				continue;
			ChannelGroup[idx]->releaseWrapper();
			ChannelGroup[idx] = ChannelGroup[idx +1];
			break;
		}

		for (;ChannelGroup[idx] != 0;idx++)
		{
			ChannelGroup[idx] = ChannelGroup[idx +1];
		}
	}

	// Decrease my references
	if (--m_refs < 0) {
		// Redraw things, so we pick up that this is missing
		delete this;
	}
}

LWError ExportModelWrapper::openPanel()
{
	if (m_panelID == 0)
	{
		initPanel();
	}

	// Use a non-blocking panel
	LW_panelFuncs->open(m_panelID,PANF_FRAME);

	return (LWError)0;
}

void ExportModelWrapper::readyForRedraw()
{
	// Make sure the looplist makes sense
	SET_INT(cLoopLists,getLoopListProvider()->getCacheIndex());

	SelectActiveEntry();

	// Name the panel

/*
	// setup the filename fillname
	SET_STR(cModelFile,m_modelfile,256);

	// Only allow the file to be available if we have
	// chosen ourselves in the loop list
	if (m_listref != getCacheID())
		GHOST_CON(cModelFile);
	else
		UNGHOST_CON(cModelFile);
*/}

// Process what we are fed
LWError ExportModelWrapper::beginFrame(LWFrame iFrame, LWTime tTime)
{
	for(int i = 0; i < MAX_GROUPSPERPANEL; i++)
	{	// FIXME - this might be calling the same group several times per frame
		ObjectWrapper *wrap = getGroup(i);
		if (wrap == 0)
			break;
		wrap->beginFrame(iFrame,tTime);
	}

	return (LWError)0;
}

void ExportModelWrapper::processVertex(LWDisplacementAccess *da)
{
	for(int i = 0; i < MAX_GROUPSPERPANEL; i++)
	{	// FIXME - this might be calling the same group several times per frame
		ObjectWrapper *wrap = getGroup(i);
		if (wrap == 0)
			break;
		wrap->processVertex(da);
	}
}

// Dump what we have now to disk
LC_FrameLoop **ExportModelWrapper::startModel()
{
	// Set up the frames - remember where they are even if the user doesn't want us to
	LC_FrameLoop **loops = m_useractiveloops = getActiveFrameLoops(CANVAS_FLAG_NOTSELECTED);
	LC_FrameLoop ***_loops = &loops;

	// Build the model
	m_userbuildingobject = m_userfuncs->startmodel(m_userinst,_loops);

	return *_loops;
}

void ExportModelWrapper::buildModel()
{
	if (m_userbuildingobject == 0)
		return;
	for(unsigned int i = 0; i < MAX_GROUPSPERPANEL; i++)
	{	// FIXME - this might be calling the same group twice per frame
		ObjectWrapper *wrap = getGroup(i);
		if (wrap == 0)
			break;
		m_userfuncs->buildmodel(m_userbuildingobject, wrap);
	}

	m_userfuncs->finishmodel(m_userbuildingobject);
	m_userbuildingobject = 0;
	if (m_useractiveloops)
		delete m_useractiveloops;
}

// Get a transient array of (active) LFrameLoops
LC_FrameLoop **ExportModelWrapper::getActiveFrameLoops(int ignoremask)
{
	LoopList& MyLoops = getProvidedLoopList();
	LC_FrameLoop **loops = new LC_FrameLoop *[MyLoops.getRangeCount() +1];
	int i = 0, j = 0;
	for (LoopRange *l = 0;
		 (l = MyLoops.getRange(i)) != 0;
		 i++)
	{
		LoopRangeStub *stub = getStubByID(l->getCacheID());
		if (stub->getFlags() & ignoremask)
			continue;

		loops[j] = stub->getLoop();
		++j;
	}
	loops[j] = 0;

	return loops;
}

// Get some stuff from our reference model
char *ExportModelWrapper::getTargetType()
{
	if (m_userfuncs && m_userinst)
		return m_userinfo->modeltype;
		
	return DEFAULT_MODEL_NAME ;
}

char *ExportModelWrapper::getTargetDescription()
{
	char *c = DEFAULT_MODEL_NAME;
	if (m_userfuncs && m_userinst)
		c = m_userfuncs->describe(m_userinst);

	return c;
}

char *ExportModelWrapper::getTargetTypeAndDescription()
{
	char *c = DEFAULT_MODEL_NAME;
	if (m_userfuncs && m_userinst)
	{
		sprintf(m_typeandnamebuffer,"%s: %s",
					m_userinfo->modeltype,
				m_userfuncs->describe(m_userinst));

		c = m_typeandnamebuffer;
	}

	return c;
}

ExportModelWrapper* ExportModelWrapper::getLoopListProvider()
{
	ExportModelWrapper *loopref = ExportModelWrapper::getByCacheID(m_listref);
	if (loopref == 0)
	{	// Someone deleted the list we wanted!  Revert to ours.
		m_listref		=	getCacheID();
		ActiveLoopID	= 0;
		loopref = this;
	}
	
	return loopref;
}

LoopList& ExportModelWrapper::getProvidedLoopList()	
{
	return getLoopListProvider()->m_loops;
}

// Constructor
ExportModelWrapper::ExportModelWrapper(int uniqueidx):
Cache<ExportModelWrapper>(uniqueidx),
	cLoopLists(0),
	cModelFile(0),
	cFrameLoopList(0),
	cFrameBtnAdd(0),cFrameBtnDel(0), cFrameBtnUpd(0),
	cFrameStrName(0),
	cFrameBtnUp(0),cFrameBtnDown(0),
	cUserXPanel(0),
	m_userinfo(0),
	m_userfuncs(0),
	m_userinst(0),
	m_userbuildingobject(0),
	m_useractiveloops(0),
	m_refs(0),
	m_flags(0),
	m_loops(),
	m_loopstubs(0),
	m_userXPanelID(0),
	m_panelID(0)
{
	ChannelGroup[0] = 0;

	m_listref = getCacheID();
	m_modelfile[0] = 0;
	ActiveLoopID = 0;

	// If we know that one has been added, we don't need to check for a master
	if (getCount() > 1)
		return;

	// Make sure the master plugin exists
	int idx = 1;			// Base 1 index
	const char *c = 0;
	for (;(c = LW_itemInfo->server(LWITEM_NULL,LWMASTER_HCLASS,idx)) != 0;idx++)
	{
		if (strcmp(c,MRB_HIDDEN_MASTERNAME) == 0)
		{
			return;
		}
	}
	
	// Add in a master server
	LWCommandFunc evaluate = (LWCommandFunc)LW_globalFuncs( "LW Command Interface", GFUSE_TRANSIENT );
	char	cmdbuf[255];

	sprintf(cmdbuf,"ApplyServer %s %s",LWMASTER_HCLASS,MRB_HIDDEN_MASTERNAME);
	evaluate(cmdbuf);
}

// Destructor (cleanup)
ExportModelWrapper::~ExportModelWrapper()
{	// Kill the 2 panels
	if (m_userXPanelID != 0)
		LW_xpanFuncs->destroy(m_userXPanelID);

	if (m_panelID != 0)
		PAN_KILL(LW_panelFuncs,m_panelID);

	for(int i = 0; i < MAX_GROUPSPERPANEL; i++)
	{	// FIXME - this might be calling the same group twice per frame
		if (!ChannelGroup[i])
			break;
		ChannelGroup[i]->releaseWrapper();
	}

	// Private userdata should be deleted through the XPanel's destroy function

	// Find the server and delete it if there are no more
	if (getCount() == 1)
		return;
	UniquePanelIndex = 0;		// reset this

	// Make sure the master plugin exists
	int idx = 1;			// Base 1 index
	const char *c = 0;
	for (;(c = LW_itemInfo->server(LWITEM_NULL,LWMASTER_HCLASS,idx)) != 0;idx++)
	{
		if (strncmp(c,LWMRBPREFIX,strlen(LWMRBPREFIX)) == 0)
		{
			// Add in a master server
			char cmdbuf[255];
			sprintf(cmdbuf,"RemoveServer %s %x",LWMASTER_HCLASS,idx);
			LW_cmdFunc(cmdbuf);
			break;
		}
	}

	// Delete all my stubs
	for (unsigned int u = 0; u  < m_loopstubs.Next(); u++)
	{
		if (m_loopstubs[u] == 0)
			continue;
		delete m_loopstubs[u];
		m_loopstubs[u] = 0;
	}
}

