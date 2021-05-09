extern "C"
{
#include "lwmod.h"
#include "LWPlugBase.h"
#include "LoadQuakeMD.h"
}
#include <stdio.h>
#include <string.h>
#include "ModelMD3.h"

// Just shields to cover void datatypes
static MessageFuncs	*Messages = 0;
static ObjectImport	*OI = 0;

static unsigned char surfacechunk[_SURFACE_MAX +1][20] =
{
	{'C','O','L','R',
		0,4,
		200,200,200,0},
	{'C','O','L','R',
		0,4,
		255,255,0,0},
	{0}
};

static void AddMD3SkinPoint(MD3_Point_Skin *p, const char *matl)
{
	float		ptBuf[3];

	// Center at 0, inflate to 40x40, and flip vertically
	ptBuf[0] =  (p->tex[0] * CurrentData->ScaleForSkinMesh) - (CurrentData->ScaleForSkinMesh/2);
	ptBuf[1] =  (p->tex[1] * -CurrentData->ScaleForSkinMesh) + (CurrentData->ScaleForSkinMesh/2);
	ptBuf[2] =  -CurrentData->ScaleForSkinMesh;

	OI->points(OI->data,1,ptBuf);
}


static void AddMD3Point(MD3_Point_Frame *p)
{
	float		ptBuf[3];

	ptBuf[0] =  ((float)p->v[0])/64;
	ptBuf[1] =  ((float)p->v[1])/64;
	ptBuf[2] =  ((float)p->v[2])/64;

	MoveVecbyOffsets(ptBuf);

	OI->points(OI->data,1,ptBuf);
}

static void AddMD3Polygon(MD3_Poly *p, int vertoffset, int srfIdx)
{
	UINT16		ptBuf[3];

	ptBuf[0] = p->vind[0] + vertoffset;
	ptBuf[1] = p->vind[1] + vertoffset;
	ptBuf[2] = p->vind[2] + vertoffset;

	OI->polygon(OI->data,3,srfIdx, OBJPOLF_FACE, ptBuf);
}

static void AddMD3TagPoints(MD3_Tag *t)
{
	float		ptBuf[3];

	// Add a point 4 units forward (on the MD3 X)
	ptBuf[0] =   t->Position[0] + TAG_POLY_LEN * t->Rotation[0][0];
	ptBuf[1] =   t->Position[1] + TAG_POLY_LEN * t->Rotation[0][1];
	ptBuf[2] =   t->Position[2] + TAG_POLY_LEN * t->Rotation[0][2];

	MoveVecbyOffsets(ptBuf);
	OI->points(OI->data,1,ptBuf);

	// Add a point 2 units right (on the MD3 -Y)
	ptBuf[0] =   t->Position[0] - TAG_POLY_WIDTH * t->Rotation[1][0];
	ptBuf[1] =   t->Position[1] - TAG_POLY_WIDTH * t->Rotation[1][1];
	ptBuf[2] =   t->Position[2] - TAG_POLY_WIDTH * t->Rotation[1][2];

	MoveVecbyOffsets(ptBuf);
	OI->points(OI->data,1,ptBuf);

	// Add anchor position as a point
	ptBuf[0] =  t->Position[0];
	ptBuf[1] =  t->Position[1];
	ptBuf[2] =  t->Position[2];

	MoveVecbyOffsets(ptBuf);
	OI->points(OI->data,1,ptBuf);
}
static int AddMD3Surf(char *name, SURFACE_TYPES type)
{
	int	surfNew = 0;
	int	srfIdx = OI->surfIndex(OI->data,name, &surfNew);
	{	// add surface chunk
		OI->surfData(OI->data,name,20,(void *)(surfacechunk[type]));
	}

	return srfIdx;
}

static BuildFuncs		BF = {AddMD3SkinPoint, AddMD3Point, AddMD3Polygon, AddMD3TagPoints, AddMD3Surf};

static int ParseMD3FromFile()
{
	int		retval	= AFUNC_OK;

	CurrentFunc		= &BF;

	// Parse file into local memory
	MD3			*mdl = new MD3(OI->filename);

	// Do Panel stuff
	if ((retval = GetDataFromUser(mdl)) != AFUNC_OK)
		return retval;

	// ===Now build the object inside Lightwave===
	OI->begin(OI->data,NULL);

	// State the number of points ...
	int TotalPntCount = (mdl->TagCount() * 3) + mdl->PointCount();

	// Make sure to duble the mesh points if we're adding skin ..
	if (CurrentData->LoadSkinVerts == 1)
		TotalPntCount += mdl->PointCount();

	OI->numPoints(OI->data,TotalPntCount);

	return (BuildMD3(mdl, TotalPntCount));
}

int LW56_LoadinMD3(BuildData *myData)
{
	int retval = 0;

	CurrentData	= myData;
	Messages	= (MessageFuncs *)(CurrentData->Message);
	OI			= (ObjectImport *)(CurrentData->Funcs);

	OI->result	= OBJSTAT_FAILED;

	if(!myData->Panel)
	{
		(*Messages->error)("Unable to activate global "PANEL_SERVICES_NAME, "     please add plugin lwpanels.p" );
		return AFUNC_OK;
	}

	if (! IsMD3NameOK(OI->filename))
	{
		SetFailureStuff(OI,"Bad Filename: ",(char *)OI->filename);
		OI->result = OBJSTAT_NOREC;
		return AFUNC_OK;
	}

	if ((retval = ParseMD3FromFile()) != AFUNC_OK)
	{
		OI->result = OBJSTAT_ABORTED;
		return retval;
	}

	FindLastPathfromName(OI->filename);

	OI->result = OBJSTAT_OK;
	OI->done(OI->data);
	OI->failedLen = 0;
	return AFUNC_OK;
}
