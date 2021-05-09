/**************************************
 *
 *  ExportPanels.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Definition for Wrapper object 
 *  One of thes surrounds each of the eventual
 *  export targets
 *
 **************************************/

#ifndef _LW_EXPORTPANEL_H
#define _LW_EXPORTPANEL_H

#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "lwrender.h"
#include "lwdisplce.h"
#include "lwmeshes.h"
#include "lwsurf.h"
#include "lwserver.h"
#include "lwpanel.h"

#include "sys_extra.h"

#include "ListenChannel.h"
#include "lw_ctleditlistbox.h"

#define MAX_GROUPSPERPANEL	16

#define DEFAULT_MODEL_NAME "--No Model--"

typedef enum
{
	COL_ACTIVE,
	COL_NAME,
	COL_LEN,
	COL_LWFRAMES,
	COL_TARGET
} LOOPCOL_TYPE;

class LoopRangeStub
{
	LC_FrameLoop innards;

	int		m_id;
	int		m_flags;

	char	m_srange[16];

	LoopRange *getRef()
	{
		if (m_id < 0)
			return &LoopRange::BadRange;
		LoopRange *loop = LoopRange::getByCacheID(m_id);
		if (loop == 0)
			return &LoopRange::BadRange;
		return LoopRange::getByCacheID(m_id);
	}

public:
	char	RangeText[16];
	LoopRangeStub(int id, int flags = 0):
		m_id(id),m_flags(flags)
	{
		memset(&innards,0,sizeof(innards));
	}

	int getID()				{ return m_id;}

	char *getName()			{ return getRef()->getName(); }
	int  getLength()		{ return getRef()->getLength(); }
	char *getLengthChar()	{ return getRef()->getLengthChar(); }

	int  getFlags()			{ return m_flags; }
	void setFlags(int i)	{ m_flags = i; }

	int  getOffset()		{ return getRef()->getOffset();}
	void setOffset(int i)	{ getRef()->setOffset(i); }

	char *getPattern()		{ return getRef()->getPattern(); }

	char *getRangeChar()	{ return getRef()->getRangeChar();}
	char *getOffsetChar()	{ return getRef()->getOffsetChar();}

	LC_FrameLoop *getLoop()
	{
		strcpy(innards.Name,getName());
		innards.ID = m_id;
		innards.state = NO_STATE;
		innards.length = getLength();
		innards.lwstart = getOffset();
		innards.lwframes = getRef()->getFrames();
		
		return &innards;
	}
};

// Class to hold each instance
class ExportModelWrapper: public Cache<ExportModelWrapper>
{	// scratch space for the long name
	char			m_typeandnamebuffer[128];

	// These are friends so we cann allow certain thigns to operated
	// based on aboslute index only

	friend void UpdateEntireEnvironment(void *target);

	// Per instance variables ....
	LWControl		*cLoopLists;
	LWControl		*cModelFile;
	LWControl		*cFrameLoopList;
	LWControl		*cFrameBtnAdd,*cFrameBtnDel, *cFrameBtnUpd;
	LWControl		*cFrameStrName, *cFrameStrPattern, *cFrameStrOffset;
	LWControl		*cFrameBtnUp,*cFrameBtnDown;
	LWControl		*cUserXPanel;
	LWXPanelID		m_userXPanelID;

	LWPanelID		m_panelID;

	// User-supplied information
	LWMRBExportType		*m_userinfo;
	LWMRBCallbackType	*m_userfuncs;
	void				*m_userinst;
	void				*m_userbuildingobject;	// used during construction
	LC_FrameLoop		**m_useractiveloops;	// used during construction

	int		m_refs;					// num of things using me
	int		m_flags;				// Flags for ... whatever
	
	LoopList					m_loops;		// my loop list
	AutoArray<LoopRangeStub *>	m_loopstubs;	// selected loops
	int							m_listref;		// ID of list referrer

	char						m_modelfile[256];// file for our info

	// Private so only we can create/delete them
	ExportModelWrapper(int idx = getCacheSize());
	~ExportModelWrapper();

	ExportModelWrapper* getLoopListProvider();
	LC_FrameLoop **getActiveFrameLoops(int ignoremask);

public:
	ObjectWrapper	*ChannelGroup[MAX_GROUPSPERPANEL];
	int				ActiveLoopID;

	// For the column values
	static char	*ColumnTitles [] ;
	static float	ColumnWidths [];

	// Input/Output
	XCALL_(static LWError) LOAD(LWInstance inst,const LWLoadState *load);
	void LOAD(const LWLoadState *load);
	XCALL_(static LWError) SAVE(LWInstance inst, const LWSaveState *save);
	void SAVE(const LWSaveState *save);
	XCALL_(static const char *) DESCLN(LWInstance inst);

	int		getUniqueIndex()	{return getCacheID();}
	int		getFlags()			{return m_flags;}
	void	setFlags(int i)		{m_flags = i;}

	int		getRefCount()		{return m_refs;}

	char	*getFile()			{return m_modelfile;}
	void	setFile(char *n = 0)
	{
		if (n)
			strcpy(m_modelfile,n);
		else
			m_modelfile[0] = 0;
	}

	// Loop List functions (potentially remote)
	LoopList&	getProvidedLoopList();		// remote
	void		setListProviderIndex(int i)	// set index for remote
	{
		m_listref = i;
		SelectActiveEntry();
		::UpdateEntireEnvironment(0);
	}

	// Local Range functions
	LoopRangeStub	*getStubByID(int id)
	{
		if (id < 0)
			return 0;

		LoopRangeStub *stub = m_loopstubs[id];
		if (!stub)
		{
			stub = new LoopRangeStub(id);
			m_loopstubs[id] = stub;
		}

		return stub;
	}

	// Value display functions
	void	ShowEmptyText();
	void	ShowText(LoopRangeStub *l);
	void	SelectActiveEntry();
	char	*getTargetStartRangeText(LoopRangeStub *l);

	// 'get' functions
	ObjectWrapper	*getGroup(int idx)
	{
		return (idx < MAX_GROUPSPERPANEL) ? ChannelGroup[idx] : 0;
	}

	// These are used by XPanels 
	int		getIndexByType(char *type = 0);
	static int getCountByType(char *type);
	static	ExportModelWrapper *getByIndexByType(int idx, char *type);

	// Object registration (public construct/descruct)
	void	acquireRef(LWItemID item, LWMRBExportType *funcs);
	void	releaseRef(LWItemID item);

	// Canvas entry manipulation
	void	DeleteActiveCanvasEntry();
	void	BuildNewCanvasEntry();
	void	UpdateCanvasEntry();
	void	BumpActiveUp();
	void	BumpActiveDown();

	// Panel setup
	void	initPanel();
	LWError openPanel();
	void	readyForRedraw();

	// Panel maintenance
	static void eventPanelOpen(LWPanelID panID, void *data);
	static void eventPanelClose(LWPanelID panID, void *data);
	static void eventObjList(LWControl *ctl, ExportModelWrapper *inst);

	// Model construction functions
	void			processVertex(LWDisplacementAccess *da);
	LWError			beginFrame(LWFrame iFrame, LWTime tTime);
	LC_FrameLoop	**startModel();
	void			buildModel();

	// Data retrieval from user data functions
	char	*getTargetType();
	char	*getTargetDescription();
	char	*getTargetTypeAndDescription();
};

// Columns hav a reference to the ExportPanel
// and calls back into it to get what it needs
class ExportListColumn : public LWCTL_Column
{
	ExportModelWrapper	*m_target;

	LoopRangeStub	*getTargetStub(int row)
	{
		LoopRange *loop = m_target->getProvidedLoopList().getRange(row);
		if (loop == 0) {
			return 0;
		}
		LoopRangeStub *stub = m_target->getStubByID(loop->getCacheID());

		return stub;
	}

protected:
	ExportModelWrapper		*getTarget()	{return m_target;}

	// These are to be filled in by the sub class
	virtual LOOPCOL_TYPE	getType()						= 0;
	virtual char			*getValue(LoopRangeStub *stub)	= 0;

	virtual void			doEvent(LoopRangeStub *stub)	{;}

public:
	ExportListColumn(ExportModelWrapper *inst)
		:m_target(inst)
	{;}

	float	width( void )
	{	return m_target->ColumnWidths[getType()];	}

	char *value( int row )	
	{
		if (row == -1)
			return m_target->ColumnTitles[getType()];
		LoopRangeStub *stub = getTargetStub(row);
		if (stub == 0)
			return 0;
		return getValue(stub);
	}

	void	event( int row )
	{
		LoopRangeStub *stub = getTargetStub(row);
		if (stub == 0)
			return;

		doEvent(stub);
	}
};

// Individual column objects
class ExportListColumn_Active : public ExportListColumn
{
protected:
	LOOPCOL_TYPE	getType()	{return COL_ACTIVE;}

	char	*getValue(LoopRangeStub *stub);
	void	doEvent(LoopRangeStub *stub);
};

class ExportListColumn_Name : public ExportListColumn
{
protected:
	LOOPCOL_TYPE	getType()	{return COL_NAME;}

	char	*getValue(LoopRangeStub *stub);
	void	doEvent(LoopRangeStub *stub);
};

class ExportListColumn_Len : public ExportListColumn
{
protected:
	LOOPCOL_TYPE	getType()	{return COL_LEN;}

	char	*getValue(LoopRangeStub *stub);
	void	doEvent(LoopRangeStub *stub);
};

class ExportListColumn_LWFrames : public ExportListColumn
{
protected:
	LOOPCOL_TYPE	getType()	{return COL_LWFRAMES;}

	char	*getValue(LoopRangeStub *stub);
	void	doEvent(LoopRangeStub *stub);
};

class ExportListColumn_Target : public ExportListColumn
{
protected:
	LOOPCOL_TYPE	getType()	{return COL_TARGET;}

	char	*getValue(LoopRangeStub *stub);
	void	doEvent(LoopRangeStub *stub);
};

// Local extension to the EditLoopList
class ExportList : public LWCTL_EditLoopList
{

};

#endif //_LW_EXPORTPANEL_H