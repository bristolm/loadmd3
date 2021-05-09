extern "C"
{
#include "lwhost.h"
#include "lwobjimp.h"
#include "lw_base.h"
#include "LoadQuakeMD.h"
}
#include <stdio.h>
#include <string.h>
#include "ModelMD3.h"

// Just shields to cover void datatypes
static LWMessageFuncs	*Messages = 0;

static unsigned char surfacechunk[_SURFACE_MAX +1][20] =
{
	{'C','O','L','R',
		0,14,
		0x3f,0x00,0x00,0x00,
		0x3f,0x00,0x00,0x00,
		0x3f,0x00,0x00,0x00,
		0,0},
	{'C','O','L','R',
		0,14,
		0x3F,0x70,0xF0,0xFA,
		0x3F,0x48,0xC8,0xCD,
		0x00,0x00,0x00,0x00,
		0,0},
	 {0}
};

class ObjImpFuncs
	: public BuildFuncs
{
	LWObjectImport	*_OI;
	char			*_SurfaceNames[255];
	void _addPoint(LWFVector pt, char *group);

public:
	ObjImpFuncs::ObjImpFuncs(LWObjectImport *oi, GUIData *data) :
	BuildFuncs(data),
		_OI(oi)
	{_SurfaceNames[0] = 0;}

	void addSkinPoint(MD3_Vert_Skin& p, const char *matl);
	void addPoly(MD3_Poly& p, int vertoffset, int srfIdx);
	int addSurf(char *name, SURFACE_TYPES surftype);

	void addPoint(MD3_Point_Frame& p, char *group);
	void addTagPoints(MD3_Tag& p, char *group);
};

// We really need to call these in the proper order
void ObjImpFuncs::addSkinPoint(MD3_Vert_Skin& p, const char *matl)
{
	float	vbuf[2];
	vbuf[0] = p.tex[0];
	vbuf[1] = 1.0 - p.tex[1];		// why is this upside down?

	if (strncmp(matl,"tag_",4))
	{	// Add a VMAP too if this isn't a tag
		_OI->vmap(_OI->data,LWVMAP_TXUV,2,matl);
		_OI->vmapVal(_OI->data,getNextPoint(),vbuf);
	}
}

void ObjImpFuncs::_addPoint(LWFVector pt, char *group)
{
	if (group)
	{
		_OI->vmap(_OI->data,
					LWVMAP_MORF,3,	// type, dimension
					group);
		_OI->vmapVal(_OI->data,getNextPoint(),pt);
	}
	else
	{
		getNextPoint() = _OI->point(_OI->data,pt);
	}
}

void ObjImpFuncs::addPoint(MD3_Point_Frame& p, char *group)
{
	LWFVector		ptBuf;

	ptBuf[0] =  ((float)(p.v[0]))/64;
	ptBuf[1] =  ((float)(p.v[1]))/64;
	ptBuf[2] =  ((float)(p.v[2]))/64;

	MoveVecbyOffsets(ptBuf);
	_addPoint(ptBuf,group);
}

void ObjImpFuncs::addPoly(MD3_Poly& p, int vertoffset, int srfIdx)
{
	LWPolID		polyid = 0;
	LWPntID 	ptBuf[3];

	ptBuf[0] = getPoint(p.vind[0] + vertoffset);
	ptBuf[1] = getPoint(p.vind[1] + vertoffset);
	ptBuf[2] = getPoint(p.vind[2] + vertoffset);

	polyid = _OI->polygon(_OI->data,LWPOLTYPE_FACE,0, 3, ptBuf);

	_OI->polTag(_OI->data, polyid,  LWPTAG_SURF, _SurfaceNames[srfIdx]);
}

void ObjImpFuncs::addTagPoints(MD3_Tag& t, char *group)
{
	LWFVector		ptBuf;

	// Add a point 4 units forward (on the MD3 X)
	ptBuf[0] =   t.Position[0] + TAG_POLY_LEN * t.Rotation[0][0];
	ptBuf[1] =   t.Position[1] + TAG_POLY_LEN * t.Rotation[0][1];
	ptBuf[2] =   t.Position[2] + TAG_POLY_LEN * t.Rotation[0][2];

	MoveVecbyOffsets(ptBuf);
	_addPoint(ptBuf,group);

	// Add a point 2 units right (on the MD3 -Y)
	ptBuf[0] =   t.Position[0] - TAG_POLY_WIDTH * t.Rotation[1][0];
	ptBuf[1] =   t.Position[1] - TAG_POLY_WIDTH * t.Rotation[1][1];
	ptBuf[2] =   t.Position[2] - TAG_POLY_WIDTH * t.Rotation[1][2];

	MoveVecbyOffsets(ptBuf);
	_addPoint(ptBuf,group);

	// Add anchor position as a point
	ptBuf[0] =  t.Position[0];
	ptBuf[1] =  t.Position[1];
	ptBuf[2] =  t.Position[2];

	MoveVecbyOffsets(ptBuf);
	_addPoint(ptBuf,group);
}

int ObjImpFuncs::addSurf(char *name, SURFACE_TYPES surftype)
{
	int srfIdx = 0;
	for (;_SurfaceNames[srfIdx];	srfIdx++)
	{
		if (strcmp(name,_SurfaceNames[srfIdx]) == 0)
			return srfIdx;
	}

	if (_SurfaceNames[srfIdx] == 0)
	{	// New one
		_SurfaceNames[srfIdx] = name;
		_SurfaceNames[srfIdx +1] = 0;
	}

	_OI->surface(_OI->data,
				name,(const char *)0,
				20,(void *)(surfacechunk[surftype]));

	return srfIdx;
}

int LW60_LoadinMD3()
{
	int retval = AFUNC_OK;

	LWObjectImport *OI = (LWObjectImport *)LocalFuncs;
	OI->result = LWOBJIM_FAILED;

	if (! IsMD3NameOK(OI->filename))
	{
		SetFailureStuff(OI,"Bad Filename: ",(char *)OI->filename);
		OI->result = LWOBJIM_NOREC ;
		return retval;
	}

	LWFVector	pivot = {0.0f};

	// Parse file into local memory
	MD3		mdl(OI->filename);

	// Do Panel stuff
	GUIData *UserInput =  GetDataFromUser(mdl);
	if (!UserInput)
	{
		return AFUNC_BADAPP;
	}

	// ===Now build the object inside Lightwave===
	OI->layer(OI->data, 0,0);
	OI->pivot(OI->data, pivot );

	//OI->numPoints(OI->data,TotalPntCount);
	ObjImpFuncs funcs(OI,UserInput);

	retval = funcs.BuildMD3(mdl);
	if (retval != AFUNC_OK)
	{
		return retval;
	}

	FindLastPathfromName(OI->filename);

	OI->result = LWOBJIM_OK;
	OI->done(OI->data);
	OI->failedLen = 0;

	return retval;
}
