/**************************************
 *
 *  LWOtoMD3.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *  
 *  Reads in a configuration file and converts
 *  a series of Lightave Object files to a MD3 file.  
 *
 *  MD3 file format property of id software
 *
 **************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "malloc.h" // for _set_sbh_threshold(0) - works around a MS crash bug
#include "math.h"

#ifdef WIN32
#include <crtdbg.h>
#endif

#include "ParseMD3.h"
#include "ParseLWO.h"
#include "ParseLWO2.h"
#include "LWOtoMD3.h"

/*
 *   MD3::
 *		+X is front
 *		+Y is right
 *		+Z is top
 *   lightwave likes:
 *		+X is right
 *		+Y is top
 *		+Z is front
 */

MD3					*OutputMD3	= (MD3 *)NULL;

char				*cmdline_UpdateFlag = "+u";
int					m_bUpdatingExistingMD3 = 0;

char				*cmdline_ScaleFlag = "+s";
float				m_fMD3ScaleAmount = 0;


int					framecount	= 0;
UINT32				meshcount	= 0;
UINT32				tagcount	= 0;

CfgData				*ActiveData = 0;

void PrintFileLists()
{
	fprintf(stdout,"LWO file data - %d frames\n",framecount);

	LWO_stub *l;
	UINT32	 j = 0;

	for (l = ActiveData->FileLists(); l; l = l->Next())
	{
		for (j = 0; j < l->ItemCount(); j++)
			fprintf(stdout,"  %3d: %s\n",j, l->FileName(j));
		fprintf(stdout,"============\n");
	}
}

void PrintMaterialMap(Material_Reference	*mr)
{
	for (; mr; mr = mr->Next())
	{
		fprintf(stdout,"'%s' (%s %d) : \n", 
					mr->Label(), mr->MeshName(), mr->Index());

		Surf_Reference		*si;
		LWO_ref				*lr;

		for (si = mr->ConversionList(); si; si = si->Next())
		{
			fprintf(stdout," +--LW Surface/UV: '%s'\n", si->Name());
			for (lr = si->ReferencedObjects(); lr; lr = lr->Next())
			{
				fprintf(stdout,"    +-- '%s'\t in file '%s'\t\t%d polys\t%d verts\n",
					lr->ReferencedName(),
					lr->ActiveObject() ? lr->ActiveObject()->FileName : "<SKIPPED>",
					lr->NumberOfApplicablePolygons(),
					lr->NumberOfApplicableVertices());
			}
		}
	}
	fprintf(stdout,"\n");
}

#define RENDER_OBJECT_DIR		"H:\\Render\\Quake3\\"
//#define RENDER_OBJECT_DIR		"D:\\Miketoys\\LW_Content\\PPM\\Objects\\Render\\"
#define RENDER_OBJECT_TEMPLATE	"%4.4d.lwo"

int BuildMeshMap(char *cfgfile)
{	// take the .cfg file and build a map of what mesh goes where

	Material_Reference		*mr = 0;

	ActiveData = new CfgData;

	if ((framecount = ProcessCfgFile(cfgfile, ActiveData)) == 0)
		return 0;

	meshcount = ActiveData->Meshes()->Count();
	tagcount = ActiveData->Tags()->Count();

	//We might be updating an existing MD3 file - try and load the one we are building
	if (m_bUpdatingExistingMD3)
	{
		OutputMD3 = new MD3(ActiveData->Name());
	// It it exists, and everything but the frame count is the same, figure we are updating
		if (OutputMD3										&&
			OutputMD3->isLoaded()							&& 
			OutputMD3->MeshCount()		== meshcount		&&
			OutputMD3->TagCount()		== tagcount			&&
			OutputMD3->PointCount()		== ActiveData->Meshes()->Head()->PointCount()	&&
			OutputMD3->PolyCount()		== ActiveData->Meshes()->Head()->PolyCount()			)
		{
			fprintf(stdout,"   -- Existing .MD3 '%s' seems valid\n", ActiveData->Name());

/*
			if (OutputMD3->FrameCount() != framecount)
			{	// we'll have to change the number of frames ...
				fprintf(stdout,"   -- changing framecount from %d to %d\n", 
							OutputMD3->FrameCount(), framecount);
				OutputMD3->ChangeFrameCount(framecount);
			}
*/
			OutputMD3->UpdateName(ActiveData->Name());
		}
		else
		{
			fprintf(stdout,"ERROR: Cannot update.MD3 '%s'\n", ActiveData->Name());
			if (!OutputMD3 || !OutputMD3->isLoaded())
			{
				fprintf(stdout,"   -- MD3 was not loadable - overwriting ...\n");
				if (OutputMD3)
				{
					delete OutputMD3;
					OutputMD3 = 0;
				}
				m_bUpdatingExistingMD3 = 0;
			}
			else
			{	// If stuff doesn't match up, let the user decide
				fprintf(stdout,"   -- MD3 was loaded, but something didn't match up:\n");
				fprintf(stdout,"       Meshes: %d \t%d\n",OutputMD3->MeshCount(),meshcount);
				fprintf(stdout,"       Tags:   %d \t%d\n",OutputMD3->TagCount(),tagcount);
				fprintf(stdout,"       Points: %d \t%d\n",OutputMD3->PointCount(),
												ActiveData->Meshes()->Head()->PointCount());
				fprintf(stdout,"       Polys:  %d \t%d\n",OutputMD3->PolyCount(),
												ActiveData->Meshes()->Head()->PolyCount());

				return 0;
			}
		}
	}

	if (!OutputMD3)
	{	// We didn't load anything in ...make a fresh one
		OutputMD3 = new MD3();
		OutputMD3->UpdateName(ActiveData->Name());

		// Now seed the meshes with names
		UINT32 j = 0;
		for (mr = ActiveData->Meshes()->Head();mr; mr=mr->Next())
		{
			OutputMD3->Mesh(j) = MD3_Mesh(mr->MeshName());
			++j;
		}

//		OutputMD3 = new MD3(ActiveData->Name(),
//							framecount,	tagcount,meshcount,
//							m);

	}
	return 1;
}

int LoadFiles(UINT32 framenum)
{
	LWO_stub		*l, *lw;
	UINT32			j = 0;

	j = 0;
	for (l = ActiveData->FileLists(); (lw = l); l = lw->Next() )
	{	// Either this is the same file over and over ...
		if (lw->LoadFileByFrame(framenum) == 0)
			return 0;
	}

	return 1;
}

int CheckCurrentState(Material_Reference *mr)
{
	for (; mr; mr = mr->Next())
	{
		if (!mr->Validate(OutputMD3))
			return 0;
	}

	return 1;
}

/*
.. "Spherical coordinates giving the direction of the vertex normal.
	They are both unsigned byte values.  The first one is the inclination,
	and the second the rotation in the horizontal plane ..."

  	pitch   = [0] * 2 * PI / 255;
  	heading = [1] * 2 * PI / 255;
*/
class VertexScratch
{
	static double		_env_conv;
public:
	int				Count;
	Vector<float>	Normal;

	VertexScratch():Count(0) {;}

	void Fill(MD3_Point_Frame &mp)
	{	
		// Convert from our coordinate system to MD3
		Vector<float> v = LW_TO_MD3_Coords * Normal;

//		RotateByMatrix(Normal,LW_TO_MD3_Coords,v);
		v /= Count;

		mp.setNormal(v);
	}
};

double VertexScratch::_env_conv = (255 / 2 / 3.14159);

int ProcessConversion()
{
	int		result = 0;
	int  i = 0, m = 0;

	fprintf(stdout,"TAGS *** material to tag  tree ***\n");
	PrintMaterialMap(ActiveData->Tags());

	fprintf(stdout,"MESH *** material to mesh tree ***\n");
	PrintMaterialMap(ActiveData->Meshes());

	// Set up a workspace for figuring the point normal values
	VertexScratch	**vxScratch = new VertexScratch *[meshcount];

	UINT32 z = 0;
	Material_Reference *mr = mr = ActiveData->Meshes()->Head();
	for (;mr;mr=mr->Next(), z++)
	{
		vxScratch[z] = new VertexScratch [mr->PointCount()];
	}

	// Cycle through all frames one at a time
	try {
	for (i = 0; i < framecount; i++	)
	{	// For each frame, suck in the appropriate LWO files
		if ((result = LoadFiles(i)) != 0)
		{	// A little sanity checking - the assumption is that in each set of input
			// frames, the points, faces, and surfaces will not only have the same counts,
			// but also the SAME ORDER.
			if ((result = CheckCurrentState(ActiveData->Tags())) != 1)
				return result;

			if ((result = CheckCurrentState(ActiveData->Meshes())) != 1)
				return result;
		}

		// Just because a file does notexist, doesn't mean we are not going to use it -
		// we'll just 'skip over' that frame's data in the final file

		double	mins[3] = {999,999,999};
		double	maxs[3] = {0,0,0};
		// For each mesh, store the locations of things for this frame
		// First - Do tag stuff
		Material_Reference		*mr = ActiveData->Tags()->Head();
		MD3_FrameTags&			tags = OutputMD3->TagsAtFrame(i);

		for (;mr; mr = mr->Next())
		{	// TAG:  Store MD3 tag positions and orientations for this frame
			strcpy(tags[mr->Index()].Name,mr->MeshName());
			
			Prep_PointsforTag	t;
			LWO_ref				*prf	= mr->ConversionList()->ReferencedObjects();

			LWO				*lwobj	= prf->ActiveObject();
			if (!lwobj)			// This file doesn't exist - skip over filling in this data.
				continue;

			UINT32 z = 0;

			LW3D::Polygon&		lp = lwobj->getPolygon(prf->OriginalPolygonIndex(0));
			for (z = 0; z < 3; z++)
			{
				LW3D::Vertex& v = lwobj->getVertex(lp.pntindices[z]);

				t.pos[z][0] = v.pos[0] * m_fMD3ScaleAmount;
				t.pos[z][1] = v.pos[1] * m_fMD3ScaleAmount;
				t.pos[z][2] = v.pos[2] * m_fMD3ScaleAmount;
			}

			lwobj->GetPolyNormal(prf->OriginalPolygonIndex(0),t.nrml);
			//t.CalculateNormal();

			// Which tag this is is indicated by the meshidx value
			t.FillTag(&tags[mr->Index()]);
		}

		// Some variables
		unsigned int j = 0;

		// Then - Set up Polygon and Vertex stuff for meshes
		Material_Reference	*si = ActiveData->Meshes();
		for (int idxmesh = 0;si; si = si->Next(), idxmesh++)
		{	// MESH:  Store polygon indexes, skin vertex location, and
			//		  per-frame vertex locations.  The first 2 are only done once

			// MESH:  Store MD3 Mesh Vertex indices for each polygon
			//        This is only done once for each mesh.

			// MESH:  Store Vertex skin location.  This is only done once for each mesh

			MD3_Mesh&			mesh	= OutputMD3->Mesh(si->Index());
			MD3_FramePoints&	frpnts	= mesh.PointsAtFrame(i);
			Surf_Reference		*cv		= (Surf_Reference *)NULL;
			LWO_ref				*rf		= (LWO_ref *)NULL;
			LWO					*lwobj	= 0;

			if (i == 0)
			{	// Add image names if it was requested ...
				if (si->GetImage(0))
				{
					if (m_bUpdatingExistingMD3)
					{	// Wipe out the old skins if we're updating
						mesh.ClearSkins();
					}

					const char *img = 0;
					for (int z = 0;	(img = si->GetImage(z)) != 0; z++)
						mesh.AddSkin(img);
				}

				// Replace the skinmaps if it was requested . FIXME.
/**
				if (si->FlaggedForMappingSkins())
				{
					if (mesh.SkinPoint(0) || m_bUpdatingExistingMD3)
					{
						mesh.SetMax(0);
					}
				}
				if (!mesh->SkinPoint(0))
				{
					MD3_Point_Skin *sk = new MD3_Point_Skin[mesh->SkinPointCount()];
					memset(sk,0,sizeof(MD3_Point_Skin) * mesh->SkinPointCount());
					mesh->SetSkinPoints(sk);
				}
*/
			}

			// Each LWO has independant numbers, but the MESH is cumulative.
			// Therefore, anytime the LWO Refers to an index, that value needs
			// to be padded by the cumulative number of things that have come before
			UINT32			poly_offset	= 0, vert_offset = 0;

			for (cv = si->ConversionList(); cv; cv = cv->Next())
			{	// For each surface that makes up this mesh ...
				for (	rf = cv->ReferencedObjects();
						rf; 
						vert_offset += rf->NumberOfApplicableVertices(),
						poly_offset += rf->NumberOfApplicablePolygons(), rf = rf->Next())
				{	// ...and each object using that surface ...

					//
					// SETUP DATA
					//
					if (i == 0)
					{	// first time only - Use the 'base' object to setup skin points
						lwobj = rf->BaseObject();

						for (j = 0; j < rf->NumberOfApplicablePolygons();j++)
						{
							MD3_Poly&	   mp = mesh.Poly(poly_offset + j);
							LW3D::Polygon&  lp = lwobj->getPolygon(rf->OriginalPolygonIndex(j));

							for (m = 0; m < 3; m ++)
							{	// Fill in the point indexes for each polygon (done once)
								mp.vind[m] = rf->TranslatedVertexIndex(lp.pntindices[m]) + vert_offset;

								// Add to the count we're keeping for the env stuff below
								vxScratch[idxmesh][mp.vind[m]].Count ++;
								
								// Snag the skinpoint (so it exists) and quit if we don't care
								MD3_Vert_Skin&	skpnts = mesh.SkinPoint(mp.vind[m]);
								if (!si->FlaggedForMappingSkins())
									continue;

								// Fill in the UV map location for each vertex (done once)
								// (This is not all that efficient, but it works for now)
								LW3D::UV&	uv =  lwobj->VertexUVmap(lp.pntindices[m]);

								// We need to add a provision for searching through specific maps
								if (uv == LW3D::UV())
									continue;

								skpnts.tex[0] = uv.uv[0];
								skpnts.tex[1] = 1 - uv.uv[1];

								// quick boundary check
								for (int xx = 0; xx < 2; xx++)
								{
									if (skpnts.tex[xx] > 1)
										skpnts.tex[xx] = 1;
									else if (skpnts.tex[xx] < 0)
										skpnts.tex[xx] = 0;
								}
							}
						}
					}

					//
					// PER FRAME DATA
					//

					// If a loaded file doesn't exist - skip over this stuff.
					if (!(lwobj = rf->ActiveObject()))	
						continue;
	
					// Figure out the position of each vertex
					for (j = 0; j < lwobj->VertexCount(); j++)
					{	// ... store the position of each vertex (done each frame)
						if (rf->TranslatedVertexIndex(j,1) == INVALID_VERTEX_IDX)
							continue;

						LW3D::Vertex&			v = lwobj->getVertex(j);
						MD3_Point_Frame&	tmp_fp = frpnts[rf->TranslatedVertexIndex(j) + vert_offset];

						// Convert from LWO coordinates to MD3 coordinates
						Vector<float>		v_tmp = LW_TO_MD3_Coords * v.pos;
//						RotateByMatrix(v.pos,LW_TO_MD3_Coords,v_tmp);

						// Store values, and set ranges for min/max values
						for (m = 0; m < 3; m ++)
						{
							v_tmp[m] *= m_fMD3ScaleAmount;

							if (v_tmp[m] < mins[m])
								mins[m] = v_tmp[m];
							if (v_tmp[m] > maxs[m])
								maxs[m] = v_tmp[m];

							tmp_fp.v[m] = (INT16)(v_tmp[m] * 64);
						}
					}

					// Figure out the environment mappings for each vertex
					for (j = 0; j < rf->NumberOfApplicablePolygons(); j++)
					{	
//						LW3D::Polygon&	lp = lwobj->Polygon(rf->OriginalPolygonIndex(j));
						MD3_Poly&	mp		= mesh.Poly(poly_offset +j);

						double			nrml[3];
						lwobj->GetPolyNormal(rf->OriginalPolygonIndex(j),nrml);

						Vector<float>		v_nrml;
						for (m = 0; m < 3; m ++)
							v_nrml[m] = nrml[m];

						for (m = 0; m < 3; m ++)
						{
							// Just store the normals now - when the frame is done, we'll finish this
							VertexScratch&	tmp_vxs = vxScratch[idxmesh][mp.vind[m]];
							tmp_vxs.Normal += v_nrml;
						}
					}
				}
			}

			// Move the env directions into the frame buffer and clear for the next frame
			for (j = mesh.PointCount(); j-- > 0;)
			{
				VertexScratch&	tmp_vxs = vxScratch[idxmesh][j];
				tmp_vxs.Fill(frpnts[j]);
				tmp_vxs.Normal = 0;
			}
		}

		// Store MD3 overall frame values (min.max) for this frame
		MD3_Frame&	frm = OutputMD3->Frame(i);

		for (j = 0; j < 3; j++)
		{
			frm.Maxs[j] = maxs[j];
			frm.Mins[j] = mins[j];
			frm.Position[j] = 0;
		}
		frm.Radius = 1;

		char tmp[64];
		sprintf(tmp,"lwotomd3 %s",PROG_VERSION);
		tmp[15] = 0;
		strcpy(frm.Creator,tmp);
	}
	}catch (LWO_ref_ERROR& e)
	{
		// need to delete vxScratch
		fprintf(stderr,"\nFATAL: %s: '%s' (%d)\n",
			e.Message(), e.Object()->FileName, e.Index());
		return 0;
	}

	// need to delete vxScratch
	delete [] vxScratch;

	return 1;
}

int main (int argc, char *argv[])
{
#ifdef WIN32
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |_CRTDBG_LEAK_CHECK_DF);
	_set_sbh_threshold(0);
#endif

	if (argc < 2)
	{
		fprintf(stdout,"Usage: %s <Input cfg file> [%s] [%s scale amount]\n",
					argv[0], cmdline_UpdateFlag, cmdline_ScaleFlag);

		return -1;
	}

	fprintf(stdout,"Lightwave to MD3 compiler - v" PROG_VERSION 
						" - mbristol@bellatlantic.net\n");

	m_fMD3ScaleAmount = 1.0;
	if (argc > 3)
	{
		for (int i = 2; i < argc; i++)
		{
			if (strcmp(argv[i],cmdline_UpdateFlag) == 0)
			{
				fprintf(stdout,"   + Updating %s\n",argv[1]);
				m_bUpdatingExistingMD3 = 1;
			}
			else if (strcmp(argv[i],cmdline_ScaleFlag) == 0)
			{
				if (++i == argc)
					break;

				sscanf(argv[i],"%f",&m_fMD3ScaleAmount);
				fprintf(stdout,"   + Scaling by %.1f%%\n",m_fMD3ScaleAmount * 100);
			}
		}
	}

//	OutputMD3 = new Mdl(argv[1]);
	if (!BuildMeshMap(argv[1]))
		return -1;

	if (!ProcessConversion())
		return -1;

	// Clean up
	OutputMD3->WritetoDisk();

	return 0;
}