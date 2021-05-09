#ifndef _LW_EXPORTQUAKEPANELS_H
#define _LW_EXPORTQUAKEPANELS_H

#include "stdio.h"

class CfgFile
{
	FILE					*fp;

	char					name[1024];

	char					sex[2];
	char					headoffset[32];
	char					footsteps[16];

	FrameBlock				*list;
	CfgFile					*next;

public:
	CfgFile(char *filename, FILE *openfile);

	int				Save();
	inline char		*FileName()
	{ return (name);	};

	inline FrameBlock	*List()
	{ return (list);	};

	inline CfgFile		*Next()
	{ return (next);	};
};


// Panel Manupulation functions
class CanvasEntry 
{
	char					name[64];
	CanvasEntry				*next;
	int						FPS;
	int						Loop;

	int						flags;

public:
	FrameBlock				LW;
	FrameBlock				MD3;

	CanvasEntry (char *name, int length, int lw_start, int q_start, int loop, int fps);
//	~CanvasEntry();

	void Add(CanvasEntry **list);
	void Remove(CanvasEntry **list);
	void Reseed(CanvasEntry ** list);

	inline CanvasEntry *Next()
	{ return next; }

	inline char *Name()
	{ return name; }

	inline int Flags()
	{ return flags; }

	inline void ToggleFlag(int i)
	{
		if (flags & i)
			flags &= ~i;
		else 
			flags |= i;
	}

	void ShowText();
	static void ShowEmptyText();
};

#endif //_LW_EXPORTQUAKEPANELS_H