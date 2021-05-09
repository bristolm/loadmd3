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
static LWObjectImport	*OI = 0;

static char		*SurfaceNames[255];
static LWPntID	*PointIDs = 0;
static int		LocalPointCount = 0;
static int		SkinPointCount = 0;

static unsigned char surfacechunk[_SURFACE_MAX +1][20] =
{
	{'C','O','L','R',
		0,14,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,0},
	{'C','O','L','R',
		0,14,
		0x3F,0x70,0xF0,0xFA,
		0x3F,0x48,0xC8,0xCD,
		0,0,0,0,
		0,0},
	 {0}
};

// We really need to call these in the proper order
static void AddMD3SkinPoint(MD3_Vert_Skin& p, const char *matl)
{
	float	vbuf[2];
	vbuf[0] = p.tex[0];
	vbuf[1] = 1.0 - p.tex[1];		// why is this upside down?

	if (strncmp(matl,"tag_",4))
	{	// Add a VMAP too if this isn't a tag
		OI->vmap(OI->data,LWVMAP_TXUV,2,matl);
		OI->vmapVal(OI->data,PointIDs[SkinPointCount++],vbuf);
	}
}


static void AddMD3Point(MD3_Point_Frame& p)
{
	LWFVector		ptBuf;

	ptBuf[0] =  ((float)(p.v[0]))/64;
	ptBuf[1] =  ((float)(p.v[1]))/64;
	ptBuf[2] =  ((float)(p.v[2]))/64;

	MoveVecbyOffsets(ptBuf);

	PointIDs[LocalPointCount++] = OI->point(OI->data,ptBuf);
}

static void AddMD3Polygon(MD3_Poly& p, int vertoffset, int srfIdx)
{
	LWPolID		polyid = 0;
	LWPntID 	ptBuf[3];

	ptBuf[0] = PointIDs[p.vind[0] + vertoffset];
	ptBuf[1] = PointIDs[p.vind[1] + vertoffset];
	ptBuf[2] = PointIDs[p.vind[2] + vertoffset];

	polyid = OI->polygon(OI->data,LWPOLTYPE_FACE,0, 3, ptBuf);

	OI->polTag(OI->data, polyid,  LWPTAG_SURF, SurfaceNames[srfIdx]);
}

static void AddMD3TagPoints(MD3_Tag& t)
{
	LWFVector		ptBuf;

	// Add a point 4 units forward (on the MD3 X)
	ptBuf[0] =   t.Position[0] + TAG_POLY_LEN * t.Rotation[0][0];
	ptBuf[1] =   t.Position[1] + TAG_POLY_LEN * t.Rotation[0][1];
	ptBuf[2] =   t.Position[2] + TAG_POLY_LEN * t.Rotation[0][2];

	MoveVecbyOffsets(ptBuf);
	PointIDs[LocalPointCount++] = OI->point(OI->data,ptBuf);

	// Add a point 2 units right (on the MD3 -Y)
	ptBuf[0] =   t.Position[0] - TAG_POLY_WIDTH * t.Rotation[1][0];
	ptBuf[1] =   t.Position[1] - TAG_POLY_WIDTH * t.Rotation[1][1];
	ptBuf[2] =   t.Position[2] - TAG_POLY_WIDTH * t.Rotation[1][2];

	MoveVecbyOffsets(ptBuf);
	PointIDs[LocalPointCount++] = OI->point(OI->data,ptBuf);

	// Add anchor position as a point
	ptBuf[0] =  t.Position[0];
	ptBuf[1] =  t.Position[1];
	ptBuf[2] =  t.Position[2];

	MoveVecbyOffsets(ptBuf);
	PointIDs[LocalPointCount++] = OI->point(OI->data,ptBuf);
}
static int AddMD3Surf(char *name, SURFACE_TYPES surftype)
{
	int srfIdx = 0;
	for (;SurfaceNames[srfIdx];	srfIdx++)
	{
		if (strcmp(name,SurfaceNames[srfIdx]) == 0)
			return srfIdx;
	}

	if (SurfaceNames[srfIdx] == 0)
	{	// New one
		SurfaceNames[srfIdx] = name;
		SurfaceNames[srfIdx +1] = 0;
	}

	OI->surface(OI->data,name,NULL,20,(void *)(surfacechunk[surftype]));

	return srfIdx;
}

static BuildFuncs		BF = {AddMD3SkinPoint, AddMD3Point, AddMD3Polygon, AddMD3TagPoints, AddMD3Surf};

static int ParseMD3FromFile()
{
	int			retval	= AFUNC_OK;
	LWFVector	pivot = {0.0f};

	CurrentFunc		= &BF;

	// Parse file into local memory
	MD3		mdl(OI->filename);

	// Do Panel stuff
	if ((retval = GetDataFromUser(mdl)) != AFUNC_OK)
		return retval;

	// ===Now build the object inside Lightwave===
	OI->layer(OI->data, 1,0);
	OI->pivot(OI->data, pivot );

	// State the number of points ...
	int TotalPntCount = (mdl.TagCount() * 3) + mdl.PointCount();

	// leave out skin points, we're doing this the new way
	PointIDs = new LWPntID [TotalPntCount+1];

	//OI->numPoints(OI->data,TotalPntCount);

	return (BuildMD3(mdl, TotalPntCount));
}

int LW60_LoadinMD3(BuildData *myData)
{
	SurfaceNames[0] = 0;
	
	int retval = AFUNC_OK;

	CurrentData	= myData;
	Messages	= (LWMessageFuncs *)(CurrentData->Message);
	OI			= (LWObjectImport *)(CurrentData->Funcs);

	OI->result = LWOBJIM_FAILED;

	if(!myData->Panel)
	{
		(*Messages->error)("Unable to activate global "PANEL_SERVICES_NAME, "     please add plugin lwpanels.p" );
		goto RETURN;
	}

	if (! IsMD3NameOK(OI->filename))
	{
		SetFailureStuff(OI,"Bad Filename: ",(char *)OI->filename);
		OI->result = LWOBJIM_NOREC ;
		goto RETURN;
	}
	retval = ParseMD3FromFile();

	if (retval != AFUNC_OK)
	{
		if (PointIDs) delete PointIDs;
		OI->result = LWOBJIM_ABORTED;
		goto RETURN;
	}

	FindLastPathfromName(OI->filename);

	OI->result = LWOBJIM_OK;
	OI->done(OI->data);
	OI->failedLen = 0;

RETURN:
	if (PointIDs)
	{
		delete PointIDs;
		PointIDs = 0;
	}
	LocalPointCount = SkinPointCount = 0;
	return retval;
}
