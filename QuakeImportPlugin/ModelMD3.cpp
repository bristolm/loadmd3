extern "C"
{
#include "lwmeshes.h"
#include "lwpanel.h"
#include "lw_base.h"
#include "LoadQuakeMD.h"
}
#include <stdio.h>
#include <string.h>
#include "ModelMD3.h"

LWControl		*c1,*c2,*c3,*c4,*c5,*c6,*c7,*c8;
char			tmpbuf[1024];

const char	*MeshNames[16];
const char	*SurfNames[16];
const char	*PanelTxt[16];
/* MD3 points are in a 3DS coordinate system
 * - the polys are flipped (point order is reversed)
 * - the coordiantes are rotated:
 *		+X is front			[1  0  0]
 *		+Y is left			[0  1  0]
 *		+Z is top			[0  0  1]
 *   By the same token, lightwave likes
 *		+X is right			[0 -1  0]
 *		+Y is top			[0  0  1]
 *		+Z is front			[1  0  0]
 */

// Use these to effect all the vertex operations

MD3_Tag			*RotationTag = (MD3_Tag *)0;
MD3_Tag			*AnchorTag = (MD3_Tag *)0; 

BuildFuncs		*CurrentFunc = (BuildFuncs *)NULL;

#define			GOOD_PANEL_WIDTH	330

#ifndef PAN_SETW
#define PAN_SETW(pfunc,pan,h)	( (ival.intv.value=h,(*pfunc->set)(pan, PAN_W, &(ival.intv.value))) )
#endif 

// Rotate by inverse vector and convert to incoming object's coordinate system
void MoveVecbyOffsets(float *v)
{
	Vector<float>	result;
	Vector<float>	V(v[0],v[1],v[2]);

	// un-rotate things around the incoming model's anchor (if necesary)
	if (RotationTag)
	{
		SubtractVector(V,RotationTag->Position,V);

		RotateByINVMatrix(V,RotationTag->Rotation,result);
	}
	else
		CopyVector(V,result);

	// Rotate around the internal anchor (if necesary)
	if (AnchorTag)
	{
		RotateByMatrix(result,AnchorTag->Rotation,V);

		AddVector(V,AnchorTag->Position,result);
	}

	// Then rotate things to lightwave's coordinate system before returning
	RotateByINVMatrix(result,LW_TO_MD3_Coords,V);

	for (int i = 0; i < 3; i ++)
		v[i] = V[i];
}

int GetDataFromUser(MD3& mdl)
{	// Stash modelnames for a list
	int			i = mdl.TagCount();
	LWPanelID	panID;

	MeshNames[i +1] = 0;
	for (;i > 0; i--)
		MeshNames[i] = (mdl.TagsAtFrame(0)[i - 1].Name);
	MeshNames[0] = "None";

	// Set up a buffer for printing the filename (for display)
	const char	*PanelTxt[4];
	PanelTxt[0] = mdl.FileName;
	PanelTxt[1] = 0;

	const char	*NoTags[2];
	NoTags[0] = "--- No Tags ---";
	NoTags[1] = 0;

	
	const char *c = (const char *)NULL;
	i = 0;
	
	// Ask the user which frame
	// Ask the user which existing Tag we should anchor it to
	if( panID=PAN_CREATE(CurrentData->Panel,"Quake MD3 file Import - v" PROG_VERSION " - mbristol@tp.net") )
	{
		// Display the filename we're working with
		c1 = TEXT_CTL(CurrentData->Panel,panID,"",PanelTxt);

		// Center it
		int x = 0, y = 0;
		int CtlLeft = CON_X(c1);
		int CtlWidth = CON_W(c1);
		if (CtlWidth < GOOD_PANEL_WIDTH)
		{
			x = CtlWidth;
			CtlWidth = GOOD_PANEL_WIDTH;
			CtlLeft = CtlLeft + (CtlWidth - x)/2;

			y = CON_Y(c1);
			MOVE_CON(c1,CtlLeft,y);
		}

		// Ask for the frame to pull the structure from
		sprintf(tmpbuf,"Choose Frame (0 - %d):",mdl.FrameCount() -1);
		c2 = MINISLIDER_CTL(CurrentData->Panel,panID,tmpbuf,60,0,mdl.FrameCount() -1);
		SET_INT(c2,CurrentData->FrameForImport);

		// List of tag meshes
		if (mdl.TagCount() > 0)
			c3 = POPUP_CTL(CurrentData->Panel,panID,"Anchor with:",MeshNames);
		else
			c3 = TEXT_CTL(CurrentData->Panel,panID,"",NoTags);


		int h = PAN_GETH(CurrentData->Panel,panID);

		// "Build Surface?" checkbox and size box
		if (CurrentData->LoadSkinVerts >= 0)
		{
			c4 = BOOLBUTTON_CTL(CurrentData->Panel,panID," Build SkinMesh ");
			// Reposition it a little
			y = CON_Y(c2);
			x = 8 + CtlWidth;
			x = x - CON_W(c4);
			MOVE_CON(c4,x,y);

			// If there are no tags, just guess where to put the skin mesh thing...
			y = CON_Y(c3);
			x = CON_X(c4);

			c5 = FLOAT_CTL(CurrentData->Panel,panID,"Mesh Scale:");
			if (CurrentData->ScaleForSkinMesh < 0.0)
				CurrentData->ScaleForSkinMesh = 0.0;

			SET_FLOAT(c5,CurrentData->ScaleForSkinMesh);

			// Reposition it a little (under the button)
			MOVE_CON(c5,x,y);
		}

		PAN_SETH(CurrentData->Panel,panID,h);
		PAN_SETW(CurrentData->Panel,panID,CtlWidth + 15);
	}
	else
	{
		PAN_KILL(CurrentData->Panel,panID);
		return AFUNC_BADGLOBAL;
	}

	/* Now open up the panel */
	int t = CurrentData->Panel->open(panID,PANF_CANCEL|PANF_BLOCKING);
	if (t == 0)
		return AFUNC_BADGLOBAL;

	/* Now grab the data out */
	GET_INT(c2,CurrentData->FrameForImport);

	if (mdl.TagCount() > 0)
		GET_INT(c3,CurrentData->AnchorTagIndex);
	
	PAN_KILL(CurrentData->Panel,panID);

	if (CurrentData->LoadSkinVerts >= 0)
	{
		GET_INT(c4,CurrentData->LoadSkinVerts);
		GET_FLOAT(c5,CurrentData->ScaleForSkinMesh);
	}

	return AFUNC_OK;
}

int BuildMD3(MD3& mdl, int TotalPntCount)
{
	int		i = 0, j = 0, k = 0;
	int		doSkin	= 0;
	// Setup rotation/translation stuff
	if (mdl.TagCount() > 0 && CurrentData->AnchorTagIndex > 0 )
		RotationTag = &(mdl.TagsAtFrame(CurrentData->FrameForImport)[CurrentData->AnchorTagIndex -1]);
	else
		RotationTag = (MD3_Tag *)NULL;

	// Add the mesh(es) points in (may be more than one)
	do
	{
		for (i = 0;
			 i < mdl.MeshCount();
			 i ++)
		{	// Add the Skin vertex points in if we need to (same order as the others)
			k = (int)(mdl.Mesh(i).PointCount());
			for (j = 0; j < k; j ++)
			{	// Add the real mesh points first, THEN the skin points
				if (doSkin)
					CurrentFunc->addSkinPoint(mdl.Mesh(i).SkinPoint(j),
												mdl.Mesh(i).Name());
				else
					CurrentFunc->addPoint(mdl.Mesh(i).PointsAtFrame(CurrentData->FrameForImport)[j]);

				--TotalPntCount;
			}
		}
		if (doSkin == 0)
			doSkin	= (CurrentData->LoadSkinVerts != 0);
		else
			--doSkin;

	} while (doSkin);

	// Then add all the extra tag points in 
	if (mdl.TagCount() > 0)
	{
		j = mdl.TagCount();
		for (i = 0; i < j; i ++)
		{
			CurrentFunc->addTag(mdl.TagsAtFrame(CurrentData->FrameForImport)[i]);
			TotalPntCount -= 3;
		}
	}

	// Now link up the polys - real polys first
	doSkin	= (CurrentData->LoadSkinVerts > 0);		// NOT if we're doing VMAPS
	int	vertoffset = 0;
	do
	{
		k = (int)(mdl.MeshCount());
		for (i = 0;	 i < k; i++)
		{
			int	srfIdx = CurrentFunc->addSurf((char *)mdl.Mesh(i).Name(), SURFACE_DEFAULT);
			UINT32 l = mdl.Mesh(i).PolyCount();
			
			for (j = 0;j < l; j++)
				CurrentFunc->addPoly(mdl.Mesh(i).Poly(j),vertoffset,srfIdx);

			vertoffset += mdl.Mesh(i).PointCount();
		}
	} while (doSkin--);	// And then the skin polys if we need to

	// And then the tag polys
	if (mdl.TagCount() > 0)
	{
		k = (int)(mdl.TagCount());
		for (i = 0;	 i < k; i++)
		{
			int			srfIdx = CurrentFunc->addSurf(mdl.TagsAtFrame(0)[i].Name, SURFACE_TAG);

			MD3_Poly	p;
			p.vind[0] = 0;
			p.vind[1] = 1;
			p.vind[2] = 2;

			CurrentFunc->addPoly(p,vertoffset,srfIdx);
			vertoffset += 3;
		}
	}

	// Reset offset structures
	RotationTag = (MD3_Tag *)NULL;
	AnchorTag = (MD3_Tag *)0;
	return AFUNC_OK;
}

//serv_w.obj server.lib libcmtd.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib