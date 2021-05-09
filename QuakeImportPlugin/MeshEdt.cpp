#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

extern "C"
{
#include "lwmod.h"
#include "LWPlugBase.h"
#include "LoadQuakeMD.h"
}
#include "ModelMD3.h"

// Just shields to cover void datatypes
static MeshEditOp	*ME = (MeshEditOp *)NULL;

static char			*CurrentSurf = (char *)NULL;
static PntID		*PointIDs = (PntID *)NULL;
static int			LocalPointCount = 0;
static MessageFuncs	*Messages = 0;
static EDError		reterr = EDERR_NONE;
static MD3_Tag		Anchor;

static void AddMD3PointD(float *p)
{
	static double	dlBuf[3];

	dlBuf[0] = (double)(p[0]);
	dlBuf[1] = (double)(p[1]);
	dlBuf[2] = (double)(p[2]);

	PointIDs[LocalPointCount++] = ME->addPoint(ME->state,dlBuf);
}

static void AddMD3SkinPoint(MD3_Point_Skin *p, const char *matl)
{
	float		ptBuf[3];

	// Center at 0, inflate to 40x40, and flip vertically
	ptBuf[0] =  (p->tex[0] * CurrentData->ScaleForSkinMesh) - (CurrentData->ScaleForSkinMesh/2);
	ptBuf[1] =  (p->tex[1] * -CurrentData->ScaleForSkinMesh) + (CurrentData->ScaleForSkinMesh/2);
	ptBuf[2] =  -CurrentData->ScaleForSkinMesh;

	AddMD3PointD(ptBuf);
}

static void AddMD3Point(MD3_Point_Frame *p)
{
	float		ptBuf[3];

	ptBuf[0] =  ((float)p->v[0])/64;
	ptBuf[1] =  ((float)p->v[1])/64;
	ptBuf[2] =  ((float)p->v[2])/64;
	MoveVecbyOffsets(ptBuf);
	AddMD3PointD(ptBuf);
}

static void AddMD3Polygon(MD3_Poly *p, int vertoffset, int srfIdx)
{
	PntID		ptBuf[3];

	// Polys are flipped ... 
	ptBuf[0] = PointIDs[p->vind[0] + vertoffset];
	ptBuf[1] = PointIDs[p->vind[1] + vertoffset];
	ptBuf[2] = PointIDs[p->vind[2] + vertoffset];

	ME->addPoly(ME->state, CurrentSurf, 3, ptBuf);
}

static void AddMD3TagPoints(MD3_Tag *t)
{
	float		ptBuf[3];

	// Add a point 4 units out on the X
	ptBuf[0] =   t->Position[0] + TAG_POLY_LEN * t->Rotation[0][0];
	ptBuf[1] =   t->Position[1] + TAG_POLY_LEN * t->Rotation[0][1];
	ptBuf[2] =   t->Position[2] + TAG_POLY_LEN * t->Rotation[0][2];

	MoveVecbyOffsets(ptBuf);
	AddMD3PointD(ptBuf);

	// Add a point 2 units out on the Y
	ptBuf[0] =   t->Position[0] - TAG_POLY_WIDTH * t->Rotation[1][0];
	ptBuf[1] =   t->Position[1] - TAG_POLY_WIDTH * t->Rotation[1][1];
	ptBuf[2] =   t->Position[2] - TAG_POLY_WIDTH * t->Rotation[1][2];

	MoveVecbyOffsets(ptBuf);
	AddMD3PointD(ptBuf);

	// Add anchor position as a point
	ptBuf[0] =  t->Position[0];
	ptBuf[1] =  t->Position[1];
	ptBuf[2] =  t->Position[2];

	MoveVecbyOffsets(ptBuf);
	AddMD3PointD(ptBuf);
}

static int AddMD3Surf(char *name, SURFACE_TYPES type)
{
	CurrentSurf = name;
	return 0;
}

/*
 * Setup the anchor polygon's effect
 *
 * since all loading is initially done in the MD3 system,and then converted 
 * into the LightWave system for drawing, we need to set this up in the
 * MD3 system so the math is a bit more consistant when importing.
 * The Prep_PointsforTag ptructure just encapsulates things
 *
 * - We're just grabbing the first selected polyon and using its normals.
 */
static EDError CountPols (void *dat, const PolygonInfo	*p)
{
	int		i = p->numPnts;
	
	Prep_PointsforTag	t;

	if ((p->flags & PPDF_SELECT) == 0)
		return 0;

	// Request normal from Lightwave
	(void)ME->polyNormal(ME->state,p->pol,t.nrml);

	// Store points for the operation
	for (i = 0; i < 3; i ++)
	{
		PointInfo *pnt = ME->pointInfo(ME->state,p->points[i]);

		t.pos[i][0] = (float)(pnt->position[0]);
		t.pos[i][1] = (float)(pnt->position[1]);
		t.pos[i][2] = (float)(pnt->position[2]);
	}

	t.FillTag(&Anchor);
//	t.CalculateNormal();

	AnchorTag = &Anchor;

	return EDERR_USERABORT;	// just want the first
}

static BuildFuncs		BF = {AddMD3SkinPoint, AddMD3Point, AddMD3Polygon, AddMD3TagPoints, AddMD3Surf};

int LW56_ImportMD3(BuildData *myData)
{
	int		retval	= AFUNC_OK;
	FileReqFunc		*f;
	const char		*hail = "Select .MD3 File to Import into LW 6.0";
	static char		name[1024];
	char			fullname[1024];

	sprintf(name,"*.md3");

	CurrentFunc		= &BF;
	CurrentData	= myData;
	Messages	= (MessageFuncs *)(CurrentData->Message);

	if(!myData->Panel)
	{
		(Messages->error)("Unable to activate global "PANEL_SERVICES_NAME, "     please add plugin lwpanels.p" );
		return AFUNC_BADGLOBAL;
	}

	f = (FileReqFunc *)((GlobalFunc *)(CurrentData->Globals))("File Request",GFUSE_TRANSIENT);
	if ( !f ||
		(f(hail, name, GetLastPath(), fullname, 1024) == 0) )
		return AFUNC_BADLOCAL;
	
	if (strlen (fullname) < 5)
		return AFUNC_BADLOCAL;

	if (!IsMD3NameOK(fullname))
		return AFUNC_BADLOCAL;

	ME = ((MeshEditBegin *)(CurrentData->Funcs))(0, 0, OPSEL_USER);
	if (!ME)
		return AFUNC_BADGLOBAL;

	CurrentData->Funcs = ME;

//	tmp = ME->pointCount(MEOp->state,0,EDCOUNT_ALL);

	// Look for selected things
	if (ME->polyCount(ME->state,OPLYR_FG,EDCOUNT_SELECT) == 1)
	{	// Anchor the input value to the selected item
		ME->polyScan(ME->state, CountPols, (void *)&Anchor, OPLYR_SELECT);
	}

	// Parse file into local memory
	MD3			*mdl = new MD3(fullname);
	int TotalPntCount = (mdl->TagCount() * 3) + mdl->PointCount();

	// Do Panel stuff
	if ((retval = GetDataFromUser(mdl)) == AFUNC_OK)
	{
		if (CurrentData->LoadSkinVerts == 1)
			TotalPntCount += mdl->PointCount();

		// ===Now build the object inside Lightwave===
		LocalPointCount = 0;
		PointIDs = (PntID *)calloc(TotalPntCount +1,sizeof(PntID));

		retval = BuildMD3(mdl, TotalPntCount);
	}

	// Clear stuff out
	AnchorTag = (MD3_Tag *)NULL;
	if (PointIDs)
	{
		free (PointIDs);
		PointIDs = 0;
	}

	ME->done(ME->state,reterr,0);

	return retval;
}

/*
	while ((c = slf->next(c)) != (const char *)NULL)
	{
		if (strstr(c,"tag_"))
			continue;
		SurfNames[i++] = c;
	}
	SurfNames[i] = 0;

	// Stash local surfaces that might be tags in a list
	SurfaceListFuncs *slf = (SurfaceListFuncs *)CurrentData->Globals("LWM: Surface List",GFUSE_TRANSIENT);

*/