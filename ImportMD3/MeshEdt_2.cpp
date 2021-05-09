#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

extern "C"
{
#include "lwmeshedt.h"
#include "lwhost.h"
#include "lw_base.h"
#include "LoadQuakeMD.h"
}

#include "ModelMD3.h"

class MeshEditFuncs
	: public BuildFuncs
{
	MeshEditOp *_ME;
	char		*_CurrentSurf;

	void		addPointD(float *p);
	EDError		processTagPoly(const EDPolygonInfo *p);

public:
	MeshEditFuncs::MeshEditFuncs(MeshEditOp *me, GUIData *data) :
	BuildFuncs(data),
		_ME(me),
		_CurrentSurf(0)
	{;}

	static EDError CountPols (void *dat, const EDPolygonInfo *p);
	void findAnchor()
	{
		// Look for anchor poly
		if (_ME->polyCount(_ME->state,OPLYR_FG,EDCOUNT_SELECT) == 1)
		{	// Anchor the input value to the selected item
			_ME->polyScan(_ME->state, CountPols, this, OPLYR_SELECT);
		}
	}

	void addSkinPoint(MD3_Vert_Skin& p, const char *matl);
	void addPoly(MD3_Poly& p, int vertoffset, int srfIdx);
	int  addSurf(char *name, SURFACE_TYPES surftype);

	void addPoint(MD3_Point_Frame& p, char *group);
	void addTagPoints(MD3_Tag& p, char *group);
};

static LWMessageFuncs	*Messages = 0;

void MeshEditFuncs::addPointD(float *p)
{
	static double	dlBuf[3];

	dlBuf[0] = (double)(p[0]);
	dlBuf[1] = (double)(p[1]);
	dlBuf[2] = (double)(p[2]);

	getNextPoint() = _ME->addPoint(_ME->state,dlBuf);
}

// We really need to call these in the proper order
void MeshEditFuncs::addSkinPoint(MD3_Vert_Skin& p, const char *matl)
{
	float	vbuf[2];
	vbuf[0] = p.tex[0];
	vbuf[1] = 1.0 - p.tex[1];		// why is this upside down?

	if (strncmp(matl,"tag_",4))
	{	// Add a VMAP if this isn't a tag
		_ME->pntVMap(_ME->state,getNextPoint(),
						LWVMAP_TXUV,
						matl,2,vbuf);
	}
}

void MeshEditFuncs::addPoint(MD3_Point_Frame& p, char *group)
{
	float		ptBuf[3];

	ptBuf[0] =  ((float)p.v[0])/64;
	ptBuf[1] =  ((float)p.v[1])/64;
	ptBuf[2] =  ((float)p.v[2])/64;
	MoveVecbyOffsets(ptBuf);
	addPointD(ptBuf);
}

void MeshEditFuncs::addPoly(MD3_Poly& p, int vertoffset, int srfIdx)
{
	LWPntID		ptBuf[3];

	// Polys are flipped ... 
	ptBuf[0] = getPoint(p.vind[0] + vertoffset);
	ptBuf[1] = getPoint(p.vind[1] + vertoffset);
	ptBuf[2] = getPoint(p.vind[2] + vertoffset);

	_ME->addFace(_ME->state, _CurrentSurf, 3, ptBuf);
}

void MeshEditFuncs::addTagPoints(MD3_Tag& t, char *group)
{
	float		ptBuf[3];

	// Add a point 4 units out on the X
	ptBuf[0] =   t.Position[0] + TAG_POLY_LEN * t.Rotation[0][0];
	ptBuf[1] =   t.Position[1] + TAG_POLY_LEN * t.Rotation[0][1];
	ptBuf[2] =   t.Position[2] + TAG_POLY_LEN * t.Rotation[0][2];

	MoveVecbyOffsets(ptBuf);
	addPointD(ptBuf);

	// Add a point 2 units out on the Y
	ptBuf[0] =   t.Position[0] - TAG_POLY_WIDTH * t.Rotation[1][0];
	ptBuf[1] =   t.Position[1] - TAG_POLY_WIDTH * t.Rotation[1][1];
	ptBuf[2] =   t.Position[2] - TAG_POLY_WIDTH * t.Rotation[1][2];

	MoveVecbyOffsets(ptBuf);
	addPointD(ptBuf);

	// Add anchor position as a point
	ptBuf[0] =  t.Position[0];
	ptBuf[1] =  t.Position[1];
	ptBuf[2] =  t.Position[2];

	MoveVecbyOffsets(ptBuf);
	addPointD(ptBuf);
}

int MeshEditFuncs::addSurf(char *name, SURFACE_TYPES type)
{
	_CurrentSurf = name;
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
EDError MeshEditFuncs::CountPols (void *dat, const EDPolygonInfo *p)
{
	int		i = p->numPnts;
	MeshEditFuncs *funcs = (MeshEditFuncs *)dat;

	if ((p->flags & EDCOUNT_SELECT ) == 0)
		return 0;

	return funcs->processTagPoly(p);
}

EDError MeshEditFuncs::processTagPoly(const EDPolygonInfo *p)
{
	Prep_PointsforTag	t;

	// Request normal from Lightwave
	(void)_ME->polyNormal(_ME->state,p->pol,t.nrml);

	// Store points for the operation
	for (int i = 0; i < 3; i ++)
	{
		EDPointInfo *pnt = _ME->pointInfo(_ME->state,p->points[i]);

		t.pos[i][0] = (float)(pnt->position[0]);
		t.pos[i][1] = (float)(pnt->position[1]);
		t.pos[i][2] = (float)(pnt->position[2]);
	}

	_AnchorTag = new MD3_Tag;

	t.FillTag(_AnchorTag);
//	t.CalculateNormal();

	return EDERR_USERABORT;	// just want the first
}

int LW60_ImportMD3()
{
	int		retval	= AFUNC_OK;
	LWFileReqFunc 	*f;
	const char		*hail = "Select .MD3 File to Import into LW 6.0";
	static char		name[1024];
	char			fullname[1024];

	sprintf(name,"*.md3");

	f = (LWFileReqFunc *)LW_globalFuncs(LWFILEREQFUNC_GLOBAL,GFUSE_TRANSIENT);
	if ( !f ||
		(f(hail, name, GetLastPath(), fullname, 1024) == 0) )
		return AFUNC_BADLOCAL;
	
	if (strlen (fullname) < 5)
		return AFUNC_BADLOCAL;

	if (!IsMD3NameOK(fullname))
		return AFUNC_BADLOCAL;

	MeshEditOp *ME = ((MeshEditBegin *)(LocalFuncs))(0, 0, OPSEL_USER);
	if (!ME)
		return AFUNC_BADGLOBAL;

//	tmp = ME->pointCount(MEOp->state,0,EDCOUNT_ALL);

	// Parse file into local memory
	MD3		mdl(fullname);

	// Do Panel stuff
	GUIData *UserInput = GetDataFromUser(mdl);
	if (UserInput)
	{
		MeshEditFuncs funcs(ME,UserInput);

		retval = AFUNC_OK;

		// ===Now build the object inside Lightwave===
		funcs.findAnchor();
		retval = funcs.BuildMD3(mdl);
	}

	// Finish stuff out
	ME->done(ME->state,EDERR_NONE,0);

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