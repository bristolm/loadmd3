/**************************************
 *
 *  ExportObj.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Definition for base objects used by the
 *  Layout recorder plugin
 *  LoopList, LoopRange
 *
 **************************************/

#include "lw_base.h"
#include "ExportObj.h"

static char				tmp[1024];

int						Cache<LoopRange>::NextUniqueIndex = 0;
AutoArray<LoopRange *>	Cache<LoopRange>::All(0);
LoopRange				LoopRange::BadRange("","");

// ---------------------------
// Given a string, process it and figure out the real length
//
// Format is:
//    ,   separates ranges
//    -   range span
void LoopRange::setPattern(char *c)
{
	strncpy(m_pattern,(const char *)c, sizeof(m_pattern) -1);
	rebuild();
}

static int _fill(int start, int end, int *frames, int *fridx)
{
	if (start == end)
	{
		frames[(*fridx)++] = start;
		return 1;
	}

	int count = 0;
	int iStep = (end < start) ? -1 : 1;
	for (int i = start; i != end; i += iStep)
	{
		frames[(*fridx)++] = i;
		++count;
	}
	return count;
}

void LoopRange::rebuild()
{
	sprintf(m_soffset,"%d",m_offset);

	int fridx = -1;
	static int fr[1024];
	memset(fr,0,sizeof(fr));

	// If this is a simple pattern ...
	if (isSimplePattern())
	{
		m_length = atoi(m_pattern);

		// Setup the int array
		fridx = m_length;
		if (m_length != 0)
		{
			for (int i = 0; i < m_length; i++)
			{
				fr[i] = i;
			}
		}
	}
	else
	{
		fridx = 0;
		int iCur = -1, iLast = -1;
		char *lastchar = 0, *p = 0;
		for (p = &(m_pattern[0]); *p; p++)
		{
			if (*p == ',')
			{	// end this current span
				if (iLast != -1)
				{
					_fill(iLast,iCur,&(fr[0]),&fridx);
					iLast = -1;
				}

				// Add the current one

				if (iCur != -1)
				{
					fr[fridx++] = iCur;
					iCur = -1;
				}
			}
			else if (*p == '-')
			{
				if (iLast != -1)
				{	// This is like 0-5-0 - add the next span
					_fill(iLast,iCur,&(fr[0]),&fridx);
				}
				iLast = iCur;
				iCur = -1;
			}
			else if (*p < '0' || *p > '9')
			{	// ignore letters
				continue;
			}
			else
			{	// just part of a number
				if (iCur == -1)	iCur = 0;
				iCur *= 10;
				iCur += (*p - '0');
			}
		}

		// end this current span
		if (iLast != -1)
		{
			_fill(iLast,iCur,&(fr[0]),&fridx);
			iLast = -1;
		}

		// Add the current one
		if (iCur != -1)
		{
			fr[fridx++] = iCur;
			iCur = -1;
		}

		m_length = fridx;
	}

	// Write the 'length' buffer
	sprintf(m_slength,"%d",m_length);

	// Make the int array
	if (lwframes != 0)
		delete lwframes;
	lwframes = 0;
	if (m_length > 0)
	{
		lwframes = new int[m_length];
		for (int i = 0; i < fridx; i++)
		{
			lwframes[i] = fr[i] + m_offset;
		}
	}

	// Build the range display field text for the table
	m_slwrange[0] = 0;
	if (m_length == 0)
	{
		sprintf(m_slwrange,"(%d)",m_offset);
		return;
	}
	if (m_length == 1)
	{
		sprintf(m_slwrange,"%d",m_offset);
		return;
	}
	if (isSimplePattern())
	{
		sprintf(m_slwrange,"%d-%d",m_offset,
					m_offset + m_length - 1);
		return;
	}

	int ridx = 0;
	for (char *p = &(m_pattern[0]); *p && ridx < 15;)
	{	
		if (*p < '0' || *p > '9' || m_offset == 0)
			m_slwrange[ridx++] = *p++;
		else
		{	// get the number, bump it
			char tmp[8];
			int iVal = 0;
			while (*p >= '0' && *p <= '9')
			{
				iVal *= 10;
				iVal += (*p++ - '0');
			}
			iVal += m_offset;
			sprintf(tmp,"%d",iVal);
			strcpy(m_slwrange + ridx,tmp);
			ridx += strlen(tmp);
		}
	}
	if (ridx +1 > 15)
	{
		m_slwrange[ridx++] = '.';
		m_slwrange[ridx++] = '.';
		m_slwrange[ridx++] = '.';
	}
	m_slwrange[ridx++] = 0;
}

// ---------------------------
LoopRange	*LoopList::getRangeByID(int id, int *slot)
{
	int found = -1;
	for (int i = 0; m_loops[i]; i++)
	{
		if (m_loops[i]->getCacheID() == id)
		{
			found = i;
			break;
		}
	}
	if (slot != 0)
		*slot = found;

	return (found < 0) ? 0 : m_loops[found];
}

/*
 * Order by Target Frame order
 */
void LoopList::add(LoopRange *l, LoopRange *after)
{	// Thread it in
	int i = 0;
	if (after != 0)
	{
		for (;m_loops[i];i++) {
			if (m_loops[i] == after)
				break;
		}
		++i;
	}


	// Make some room
	for (int j = m_loops.Next() -1; j > i; j--)
		m_loops[j] = m_loops[j -1];

	// Add it in
	m_loops[i] = l;
}

void LoopList::remove(LoopRange *l)
{
	int i = 0;
	for (;m_loops[i];i++) {
		if (m_loops[i] == l)
			break;
	}

	// delete it
	if (m_loops[i] == (LoopRange *)0)
		return;
	delete m_loops[i];

	// Move the others down
	int j = i;
	for (; m_loops[j +1];j++)
		m_loops[j] = m_loops[j +1];
	m_loops[j] = 0;
}

void LoopList::moveUp(LoopRange *l)
{
	int i = 0;
	for (;m_loops[i];i++) {
		if (m_loops[i] == l)
			break;
	}

	if (i == 0)
		return;

	LoopRange *prev = m_loops[i -1];
	m_loops[i -1] = l;
	m_loops[i] = prev;
}

void LoopList::moveDown(LoopRange *l)
{
	int i = 0;
	for (;m_loops[i];i++) {
		if (m_loops[i] == l)
			break;
	}

	if (m_loops[i] == 0)
		return;

	LoopRange *next = m_loops[i +1];
	if (next == 0)	// don't move off the end
		return;
	m_loops[i +1] = l;
	m_loops[i] = next;
}

int LoopList::getRangeCount()
{
	int i = 0;
	while (m_loops[i])
		++i;
	return i;
}

/*
 * CfgFile class functions
 */
/*
#define TOKEN_STRING	" \t\n\r"
#define TERMINATE_TOKEN(t)								\
		p = t;										\
		while (*p != ' ' && *p != '/' && *p !=13)		\
				++p;									\
		*p = 0;											\

CfgFile::CfgFile(char *filename, FILE *openfile):
	fp(openfile),
	Stub()
{
	char	*p = (char *)0, *token = (char *)0;
	char	*tokens[5];
	int		NextInputToken = -1;

	strcpy(name,filename);

	while(!feof(fp ))
	{
		p = fgets(tmp,1024,fp);
		if (ferror(fp))
			break;

		token = tmp;

		while(token = strtok(token, TOKEN_STRING))
		{	// Find the end of the token
			if (token[0] == 13)							// get next line
				break;
			else if (token[0] == '/')					// comment token
			{
				if (NextInputToken < 0)					// fully commented line
					break;
			}
			else if (NextInputToken >= 0)
			{
				tokens[++NextInputToken] = token;
				if (NextInputToken == 4)	// done with this one - add in entry
				{										
					Stub.add(new CanvasEntry(tokens[4],
									 atoi(tokens[1]),
									 atoi(tokens[0]), atoi(tokens[0]),
									 atoi(tokens[2]), atoi(tokens[3]) ));
					NextInputToken = -1;
					break;					// and get a new line
				}
			}
			else if (strcmp("sex",token) == 0)				// sex m
			{
				strcpy(sex,token + strlen(token) +1);
				break;
			}
			else if(strcmp("headoffset",token) == 0)	// headoffset 0 0 0
			{
				strcpy(headoffset,token + strlen(token) +1);
				break;
			}
			else if(strcmp("footsteps",token) == 0)		// footsteps normal
			{
				strcpy(footsteps,token + strlen(token) +1);
				break;
			}
			else
			{											// line with real info in it
				tokens[NextInputToken = 0] = token;
			}

			token += (strlen(token) +1);
		}
	}
}
*/
