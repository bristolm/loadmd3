#include "ParseMD3.h"

typedef enum
{
	SURFACE_DEFAULT	=0,
	SURFACE_TAG,
	_SURFACE_MAX
} SURFACE_TYPES;

class BuildFuncs
{
	int		 _PointCount;
	LWPntID *_PointIDs;
	int		 _ActivePoint;

	int		 _DoneSetup;

protected:
	GUIData	*_Data;

	MD3_Tag	*_RotationTag;
	MD3_Tag	*_AnchorTag;

	void MoveVecbyOffsets(float *v);
	int  isDoneAdding()			{return _DoneSetup;}

	LWPntID& getPoint(int id)	{return _PointIDs[id];}
	LWPntID& getNextPoint()		{return _PointIDs[_ActivePoint++];}

public:
	BuildFuncs(GUIData *data):
	  _PointCount(0), _PointIDs(0), _ActivePoint(0), _DoneSetup(0),
	  _Data(data),
	  _AnchorTag(0)
	{;}

	virtual ~BuildFuncs()
	{
		if (_PointIDs)	delete _PointIDs;
	}

	int getPointCount()	{return _PointCount;}

	// Processing functions
	int BuildMD3(MD3& mdl);
	void ResetPointCounter()	{_DoneSetup = 1; _ActivePoint = 0;};

	virtual void addSkinPoint(MD3_Vert_Skin& p, const char *matl) = 0;
	virtual void addPoly(MD3_Poly& p, int vertoffset, int srfIdx) = 0;
	virtual int addSurf(char *name, SURFACE_TYPES surftype) = 0;

	virtual void addTagPoints(MD3_Tag& p, char *group) = 0;
	virtual void addPoint(MD3_Point_Frame& p, char *group) = 0;
};

extern	int			IsMD3NameOK(const char *name);
extern GUIData		*GetDataFromUser(MD3& mdl);

/*
 *   MD3::
 *		+X is front
 *		+Y is right
 *		+Z is top
 *   lightwave likes:
 *		+Z is front
 *		+X is right
 *		+Y is top
 *
 */
#define LW_RIGHT	0
#define LW_TOP		1
#define LW_FRONT	2

#define ID_FRONT	0
#define ID_TOP		1
#define ID_RIGHT	2

#define TOP
#define FRONT
#define RIGHT

// Barbie says math is hard
extern void MoveVecbyOffsets(float *v);
