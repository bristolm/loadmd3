#include "ParseMD3.h"

typedef enum
{
	SURFACE_DEFAULT	=0,
	SURFACE_TAG,
	_SURFACE_MAX
} SURFACE_TYPES;

typedef struct
{	// Operation functions
	void			(*addSkinPoint)	(MD3_Vert_Skin& p, const char *matl);
	void			(*addPoint)		(MD3_Point_Frame& p);
	void			(*addPoly)		(MD3_Poly& p, int vertoffset, int srfIdx);
	void			(*addTag)		(MD3_Tag& p);
	int				(*addSurf)		(char *name, SURFACE_TYPES surftype);
} BuildFuncs;

extern	BuildFuncs	*CurrentFunc;
extern  float		SkinMeshDrawScale;

extern	int			GetDataFromUser(MD3& mdl);
extern	int			BuildMD3(MD3& mdl, int TotalPntCount);
extern	MD3_Tag		*AnchorTag;
extern	int			IsMD3NameOK(const char *name);

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
