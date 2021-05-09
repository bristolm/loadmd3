/**************************************
 *
 *  ExportObj.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Definition for base objects used by the
 *  Layout recorder plugin
 *  LoopList, LoopRange
 *
 **************************************/


#ifndef _LW_EXPORTOBJ_H
#define _LW_EXPORTOBJ_H

#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "lwrender.h"
#include "lwdisplce.h"
#include "lwmeshes.h"
#include "lwsurf.h"
#include "lwserver.h"
#include "lwpanel.h"
#include "lwmrbxprt.h"

#include "sys_extra.h"

#define MAX_LOOP_NAME	128

// Frame list display variables
#define CANVAS_FLAG_NOTSELECTED	0x1
#define MAX_FRAMEPATTERN_FIELD	4096

// Panel Manupulation functions
class LoopList;

class LoopRange : public Cache<LoopRange>
{
	char	m_name[MAX_LOOP_NAME];	// Loop name
	char	m_pattern[MAX_FRAMEPATTERN_FIELD];		// Frame 'look'

	int		m_offset;				// Offset
	int		m_length;				// Calculated length

	char	m_soffset[8];
	char	m_slength[8];
	char	m_slwrange[32];
	char	m_stargetrange[16];

	int		*lwframes;

	void	rebuild();

public:
	static LoopRange				BadRange;

	LoopRange (char *name, int len, int seedidx ):
	Cache<LoopRange>(seedidx),
		m_offset(0),
		lwframes(0)
	{
		strcpy(m_name,name);
		m_length = len;
		sprintf(m_slength,"%d",m_length);
		setPattern(m_slength);
	};

	LoopRange (char *name, char *pattern ):
		m_offset(0),
		lwframes(0)
	{
		strcpy(m_name,name);
		setPattern(pattern);
	};

	// If it's all numbers, it's simple
	int isSimplePattern()
	{
		// If this is all numbers, we interpret it as 0-(num -1)
		char *p = 0;
		for (p = &(m_pattern[0]); *p; p++)
		{
			if (*p < '0' || *p > '9')
				break;
		}
		return (*p == 0) ? 1 : 0;
	}

	void setPattern(char *c);
	char *getPattern()		{ return m_pattern;}

	char *getName()			{ return m_name; }

	int  getLength()		{ return m_length; }
	void setLength(int i)
	{
		m_length = i;
		rebuild();
	}
	char *getLengthChar()
	{
		return m_slength;
	};

	int  getOffset()		{ return m_offset; }
	void setOffset(int i)
	{
		m_offset = i;
		rebuild();
	}
	char *getOffsetChar()	{ return m_soffset; }

	int *getFrames()		{ return lwframes;};

	char *getRangeChar()	{ return m_slwrange; }
	char *getLWRangeChar()
	{
		sprintf(m_slwrange,"%d",m_offset);
		return m_slwrange;
	};
};

class LoopList
{
	char					m_name[MAX_LOOP_NAME];
	AutoArray<LoopRange *>	m_loops;

public:
	LoopRange	*ActiveLoop;

	LoopList():
		m_loops(0),
		ActiveLoop(0)
	{;}

	~LoopList()
	{	// This is safe, since all ranges are in only one list
		for (int i = 0; m_loops[i]; i++)
			delete m_loops[i];
	}

	int getRangeCount();
	LoopRange	*getRangeByID(int id, int *slot = 0);
	LoopRange	*getRange(int idx)
	{
		return m_loops[idx];
	}

	void add(LoopRange *l, LoopRange *after = 0);
	void remove(LoopRange *l);
	void moveUp(LoopRange *l);
	void moveDown(LoopRange *l);
};

/*
class CfgFile
{
	FILE					*fp;

	char					name[1024];

	char					sex[2];
	char					headoffset[32];
	char					footsteps[16];

	FileStub				Stub;
public:
	CfgFile(char *filename, FILE *openfile);

	int				Save();
	inline char		*FileName()
	{ return (name);	};

	FileStub& getFileStub()
	{ return (Stub); }
};
*/

#endif //_LW_EXPORTOBJ_H