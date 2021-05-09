/**************************************
 *
 *  CheckMD3.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  File investigator tool
 *  This started as simply a MD3 file tool,
 *  But it now does so much more ...
 *
 **************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "ParseMD2.h"
#include "ParseMD3.h"
#include "ParseMD4.h"
#include "ParseMDS.h"
#include "ParseLWO.h"
#include "ParseLWO2.h"
#include "ParseUnrealSkl.h"

char	tmp[1024];

static int verboselevel = 0;

int processFOO(char *filename)
{	// Right now, means 'convert MD3 to MD4'
	filename[strlen(filename) - 4] = 0;

	char md3name[3][64];
	sprintf(md3name[0],"%s.md3",filename);
	sprintf(md3name[1],"%s_1.md3",filename);
	sprintf(md3name[2],"%s_2.md3",filename);

	char md4name[64];
	sprintf(md4name,"%s.md4",filename);

	fprintf(stdout, "%s --> %s \n",md3name,md4name);

	MD4 md4;

	// Add one frame into the MD4 ...
	MD4_Frame& frame = md4.Frame(0);

	// ... with some bones
	MD4_Bone& bone = frame.Bone(0);
	MD4_Bone& bone2 = frame.Bone(1);
//	MD4_Bone& bone3 = frame.Bone(2);
//	MD4_Bone& bone4 = frame.Bone(3);

	MD3 md3base(md3name[0]);
	if (!md3base.isLoaded())
	{
		fprintf(stdout,"MD3 basefile '%s' seems invalid\n",md3name);
		return -1;
	}


	// .. and let's add the LODs
	int z = 0;
	for (; z < 3; z++) {
		MD3 md3(md3name[z]);
		if (!md3base.isLoaded())
		{
			md3 = md3base;
		}

		fprintf(stdout,"MD3 file '%s' for LOD %d\n",
					md3.ModelName(), z);		

		MD4_LOD& lod = md4.LOD(z);

		// one surface per mesh
		for (UINT32 i = 0; i < md3.MeshCount(); i++)
		{
			MD3_Mesh& mesh = md3.Mesh(i);
			MD4_Surface& surf = lod.Surface(i);

			surf.Name((char *)mesh.Name());

			// Add some fake bone refs
			surf.BoneRef(0) = 0;
			surf.BoneRef(1) = 1;

			// Add all polygons from the incoming MD3
			UINT32 j = 0;
			for (j = 0; j < mesh.PolyCount(); j++)
			{
				MD3_Poly& ply3 = mesh.Poly(j);
				MD4_Poly& ply4 = surf.Polygon(j);

	//			ply4.vind[0] = 0;
	//			ply4.vind[1] = 1;
	//			ply4.vind[2] = 2;
				ply4.vind[0] = ply3.vind[0];
				ply4.vind[1] = ply3.vind[1];
				ply4.vind[2] = ply3.vind[2];
			}

			// Add all vertex and skin info
			for (j = 0; j < mesh.PointCount(); j++)
			{
				MD3_Point_Frame& vtx3 = mesh.PointsAtFrame(0)[j];
				MD3_Vert_Skin&   skn3 = mesh.SkinPoint(j);

				MD4_Vertex&		 vtx4 = surf.Vertex(j);

				// Add a fake weights referencing the fake bones.
				int k = 0;
				for (k = 0; k < 2; k ++) {
					MD4_Weight& weight = vtx4.Weight(k);
					weight.boneIndex = k;
					weight.boneWeight = 1;

					weight.offset[0] = 0;
					weight.offset[1] = 0;
					weight.offset[2] = 0;
				}

				// Move the values over
				vtx4.v[0] = vtx3.v[0];
				vtx4.v[1] = vtx3.v[1];
				vtx4.v[2] = vtx3.v[2];

	//			vtx4.normal[0] = vtx3.normal[0];
	//			vtx4.normal[1] = vtx3.normal[1];
	//			vtx4.normal[2] = vtx3.normal[2];

				vtx4.uv.tex[0] = 0;//skn3.tex[0];
				vtx4.uv.tex[1] = 1;//skn3.tex[1];
			}
		}
	}

	md4.WritetoDisk(md4name);

	return 1;
}

int compareMD3(MD3& a, MD3& b)
{
	UINT32 i = 0, j = 0;

	char *diff = "* ";
	char *same = "  ";

	char *mark = same;

	// Header
	fprintf(stdout, "%sNames: \t%25s\tvs.\t%25s\n",mark,
		a.ModelName()[0] == 0 ? "[NONE]" : a.ModelName(),
		b.ModelName()[0] == 0 ? "[NONE]" : b.ModelName());

	// Frames
	fprintf(stdout, "%sFrames:\t%25d\tvs.\t%25d\n",mark,a.FrameCount(),b.FrameCount());
	for (i = 0; i < a.FrameCount(); i++)
	{
		fprintf(stdout, "%s       \t%25.3f\tvs.\t%25.3f\n",mark,
					a.Frame(i).Position,b.Frame(i).Position);
		fprintf(stdout, "%s    X = \t%12.3f,%12.3f\tvs.\t%12.3f,%12.3f\n",mark,
					a.Frame(i).Mins[0] / 64,a.Frame(i).Maxs[0] / 64,
					b.Frame(i).Mins[0] / 64,b.Frame(i).Maxs[0] / 64);
		fprintf(stdout, "%s    Y = \t%12.3f,%12.3f\tvs.\t%12.3f,%12.3f\n",mark,
					a.Frame(i).Mins[1] / 64,a.Frame(i).Maxs[1] / 64,
					b.Frame(i).Mins[1] / 64,b.Frame(i).Maxs[1] / 64);
		fprintf(stdout, "%s    Z = \t%12.3f,%12.3f\tvs.\t%12.3f,%12.3f\n",mark,
					a.Frame(i).Mins[2] / 64,a.Frame(i).Maxs[2] / 64,
					b.Frame(i).Mins[2] / 64,b.Frame(i).Maxs[2] / 64);
	}

	// Tags
	fprintf(stdout, "%sTags:  \t%25d\tvs.\t%25d\n",mark,a.TagCount(),b.TagCount());

	// Meshes
	fprintf(stdout, "%sMeshes:\t%25d\tvs.\t%25d\n",mark,a.MeshCount(),b.MeshCount());

	for (i = 0; i < a.MeshCount(); i++)
	{
		fprintf(stdout, "%s       \t%25s\tvs.\t%25s\n","","---","---");
		MD3_Mesh&	mesh_a = a.Mesh(i);
		MD3_Mesh&	mesh_b = b.Mesh(i);

		fprintf(stdout, "%sNames: \t%25s\tvs.\t%25s\n",mark,
						mesh_a.Name(),mesh_b.Name());

		fprintf(stdout, "%sVerts: \t%25d\tvs.\t%25d\n",mark,
						mesh_a.PointCount(),mesh_b.PointCount());

		fprintf(stdout, "%sPolys: \t%25d\tvs.\t%25d\n",mark,
						mesh_a.PolyCount(),mesh_b.PolyCount());

		UINT32 c_a = mesh_a.SkinCount();
		UINT32 c_b = mesh_b.SkinCount();
		UINT32 c_max = c_a > c_b ? c_a : c_b;

		for (j = 0; j < c_max; j++)
		{
			fprintf(stdout,"%sSkin:  \t%25s\tvs.\t%25s\n",mark,
					j < c_a ? mesh_a.Skin(j).Name : "",
					j < c_b ? mesh_b.Skin(j).Name : "");
		}

		c_a = mesh_a.PolyCount();
		c_b = mesh_b.PolyCount();
		c_max = c_a > c_b ? c_a : c_b;
		for (j = 0; j < c_max; j++)
		{
			fprintf(stdout,"%sPolys: \t%7d,%7d,%7d  \tvs.\t%7d,%7d,%7d\n",mark,
					j < c_a ? mesh_a.Poly(j).vind[0] : -1,
					j < c_a ? mesh_a.Poly(j).vind[1] : -1,
					j < c_a ? mesh_a.Poly(j).vind[2] : -1,
					j < c_b ? mesh_b.Poly(j).vind[0] : -1,
					j < c_b ? mesh_b.Poly(j).vind[1] : -1,
					j < c_b ? mesh_b.Poly(j).vind[2] : -1);
		}

		c_a = mesh_a.SkinPointCount();
		c_b = mesh_b.SkinPointCount();
		c_max = c_a > c_b ? c_a : c_b;
		for (j = 0; j < c_max; j++)
		{
			fprintf(stdout,"%sVMap:  \t%12.5f,%12.5f\tvs.\t%12.5f,%12.5f\n",mark,
					j < c_a ? mesh_a.SkinPoint(j).tex[0] : -1,
					j < c_a ? mesh_a.SkinPoint(j).tex[1] : -1,
					j < c_b ? mesh_b.SkinPoint(j).tex[0] : -1,
					j < c_b ? mesh_b.SkinPoint(j).tex[1] : -1);
		}

		c_a = mesh_a.PointCount();
		c_b = mesh_b.PointCount();
		c_max = c_a > c_b ? c_a : c_b;
		for (j = 0; j < c_max; j++)
		{
			fprintf(stdout,"%sEnvmap:\t%12d,%12d\tvs.\t%12d,%12d\n",mark,
				j < c_a ? mesh_a.PointsAtFrame(0)[j].normal[0] : -1,
				j < c_a ? mesh_a.PointsAtFrame(0)[j].normal[1] : -1,
				j < c_b ? mesh_b.PointsAtFrame(0)[j].normal[0] : -1,
				j < c_b ? mesh_b.PointsAtFrame(0)[j].normal[1] : -1);
		}
	}

	return 1;
}

int processMD2(char *filename)
{
	MD2		Mdl(filename);

	if (!Mdl.isLoaded())
	{
		fprintf(stdout,"MD2 file '%s' seems invalid\n",filename);
		return -1;
	}

	UINT32 i = 0;

	// start
	fprintf(stdout,"\nQuake MD2 file Analyzer - v" PROG_VERSION " - mbristol@bellatlantic.net\n\n");

	// Header
	fprintf(stdout, "%s\n[file=\t%s]\n",Mdl.ModelName(),filename);
	fprintf(stdout, "%d Frame%s\n",Mdl.FrameCount(),Mdl.FrameCount() != 1 ? "s" : "");

	// Mesh data
	fprintf(stdout,"\tMesh:\n\t  Frames = %d\n\t  Points = %d/%d\n\t  Polys  = %d\n",
					Mdl.FrameCount(), Mdl.PointCount(), Mdl.SkinPointCount(), Mdl.PolyCount());

	// Skins
	for (i = 0; i < Mdl.SkinCount(); i++)
		fprintf(stdout,"\tSkin %2d: %s\n",i,Mdl.Skin(i).Name);

	return 1;
}

int processMD3(char *filename)
{
	MD3		Mdl(filename);

	if (!Mdl.isLoaded())
	{
		fprintf(stdout,"MD3 file '%s' seems invalid\n",filename);
		return -1;
	}

	UINT32 i = 0, j = 0;

	// start
	fprintf(stdout,"\nQuake MD3 file Analyzer - v" PROG_VERSION " - mbristol@bellatlantic.net\n\n");

	// Header
	fprintf(stdout, "%s\n[file=\t%s]\n",
			Mdl.ModelName()[0] == 0 ? "[NONE]" : Mdl.ModelName(),
			filename);
	fprintf(stdout, "%d Frame%s\n",Mdl.FrameCount(),Mdl.FrameCount() != 1 ? "s" : "");

	if (verboselevel > 0)
	{
		for (i = 0; i < Mdl.FrameCount(); i++)
		{
	fprintf(stdout, "\t%.3d - Scale: %3.3f\n",i,
				Mdl.Frame(i).Radius);
	fprintf(stdout, "\t  at  %3.3f,%3.3f,%3.3f\n",
				Mdl.Frame(i).Position[0],
				Mdl.Frame(i).Position[1],
				Mdl.Frame(i).Position[2]);
	fprintf(stdout, "\t MIN: %3.3f,%3.3f,%3.3f\n",
				Mdl.Frame(i).Mins[0],
				Mdl.Frame(i).Mins[1],
				Mdl.Frame(i).Mins[2]);
	fprintf(stdout, "\t MAX: %3.3f,%3.3f,%3.3f\n",
				Mdl.Frame(i).Maxs[0],
				Mdl.Frame(i).Maxs[1],
				Mdl.Frame(i).Maxs[2]);
		}
	}

	// Tags
	fprintf(stdout, "Tags \t(%d) x%d frames\n",Mdl.TagCount(),Mdl.FrameCount());
	MD3_FrameTags&	tag = Mdl.TagsAtFrame(0);
	
  	for (i = 0; i < Mdl.TagCount(); i++)
		fprintf(stdout," \t%s\n",tag[i].Name);

	// Meshes
	fprintf(stdout, "Meshes\t(%d)\n",Mdl.MeshCount());
	for (i = 0; i < Mdl.MeshCount(); i++)
	{
		MD3_Mesh&	mesh = Mdl.Mesh(i);
		fprintf(stdout,"\t%s\n\t  Frames = %d\n\t  Points = %d\n\t  Polys  = %d\n",
					mesh.Name(), mesh.FrameCount(), mesh.PointCount(), mesh.PolyCount());

		for (j = 0; j < mesh.SkinCount(); j++)
			fprintf(stdout,"\t  - %s\n",mesh.Skin(j).Name);
	}

	// Frames


	return 1;
}

int processMD4(char *filename)
{
	MD4 Mdl(filename);

	if (!Mdl.isLoaded())
	{
		fprintf(stdout,"MD3 file '%s' seems invalid\n",filename);
		return -1;
	}

	// Header
	char tmp[64];
	Mdl.TypeName(tmp);
	fprintf(stdout, "%s - '%s'\n",tmp,Mdl.FileName);
	fprintf(stdout, "%d LODs\n",Mdl.LODCount());
	fprintf(stdout, "%d Frames\n",Mdl.FrameCount());
	fprintf(stdout, "%d Global Bones\n",Mdl.Frame(0).BoneCount());

	// LODs
	for (UINT32 i = 0; i < Mdl.LODCount(); i++)
	{
		MD4_LOD& lod = Mdl.LOD(i);
		fprintf(stdout, "LOD #%d: %d Surfaces: \n",i,lod.SurfaceCount());

		for (UINT32 j = 0; j < lod.SurfaceCount(); j++)
		{
			MD4_Surface& surf = lod.Surface(j);
			fprintf(stdout, " - %s - %d Polys, %d Verts, %d Bone Refs \n",surf.Name(),
					surf.PolygonCount(), surf.VertexCount(), surf.BoneRefCount());

			if (verboselevel == 0)
				continue;

			for (UINT32 k = 0; k < surf.VertexCount(); k++) {
				MD4_Vertex& vtx = surf.Vertex(k);
				fprintf(stdout, "     - Vertex #%d: %d Weights\n",k,vtx.WeightCount());
			}
		}
	}

	return 1;
}

int processMDS(char *filename)
{
	MDS Mdl(filename);

	if (!Mdl.isLoaded())
	{
		fprintf(stdout,"MD3 file '%s' seems invalid\n",filename);
		return -1;
	}

	// Header
	char tmp[64];
	Mdl.TypeName(tmp);
	fprintf(stdout, "%s - '%s'\n",tmp,Mdl.FileName);
	fprintf(stdout, "%d LODs\n",Mdl.LODCount());
	fprintf(stdout, "%d Frames\n",Mdl.FrameCount());

	fprintf(stdout, "%d Global Bones\n",Mdl.BoneCount());
	if (verboselevel != 0)
	{
		for (UINT32 j = 0; j < Mdl.BoneCount(); j++)
		{
			// Check each for Tagginess
			char *taginess = "     ";
			for (UINT32 k = 0; k < Mdl.TagCount(); k++)
			{
				if (Mdl.Tag(k).BoneIndex != j)
				{
					continue;
				}
				taginess = "[TAG]";
				break;
			}
			fprintf(stdout, "\t Bone #%2d: %s '%s' (parent=%d)\n",
						j,taginess,Mdl.Bone(j).Name,Mdl.Bone(j).ParentBoneIndex);
		}
	}
	fprintf(stdout, "%d Specialized as Tags\n\n",Mdl.TagCount());

	// LODs
	for (UINT32 i = 0; i < Mdl.LODCount(); i++)
	{
		MDS_LOD& lod = Mdl.LOD(i);
		fprintf(stdout, "LOD #%d: %d Surfaces: \n",i,lod.SurfaceCount());

		for (UINT32 j = 0; j < lod.SurfaceCount(); j++)
		{
			MDS_Surface& surf = lod.Surface(j);
			fprintf(stdout, " - %s - %d Polys, %d Verts, %d Bones Referenced \n",surf.Name(),
					surf.PolygonCount(),
					surf.VertexCount(),
					surf.BoneRefCount());
		}
	}

	return 1;}

int processLWO(char *filename)
{
	LW3D::LWO *Lwo = new LW3D::LWOv2(filename);

	if (!Lwo->isValid())
	{
		delete Lwo;
		Lwo = new LW3D::LWOv1(filename);
	}

	if (!Lwo->isValid())
	{
		fprintf(stdout,"LWO file '%s' seems invalid\n",filename);
		return -1;
	}

	// Header
	fprintf(stdout, "%s - '%s'\n",Lwo->FileType(),Lwo->FileName);
	fprintf(stdout, "%d Vert%s\n",Lwo->VertexCount(),Lwo->VertexCount() != 1 ? "ices" : "ex");
	fprintf(stdout, "%d Face%s\n",Lwo->PolyCount(),Lwo->PolyCount() != 1 ? "s" : "");

	// Surfaces
	fprintf(stdout, "%d Surface%s: \n",Lwo->SurfaceCount(),Lwo->SurfaceCount() != 1 ? "s" : "");
	UINT32 i = 0;
	for (i = 0;i < Lwo->SurfaceCount(); i ++)
	{	// first clear the point counting buffer
		LW3D::Surface& s = Lwo->getSurface(i);
		fprintf(stdout,"\t%s\n\t  Faces = %d\n\t  Verts = %d\n",
							s.name,	s.PolygonCount(), 0);
	}

	fprintf(stdout, "%d UV Map%s: \n",Lwo->UVMapCount(),Lwo->UVMapCount() != 1 ? "s" : "");
	for (i = 0;i < Lwo->UVMapCount(); i ++)
	{	// first clear the point counting buffer
		LW3D::Surface & s = Lwo->getVMap(i);
		fprintf(stdout,"\t%s\n\t  Faces = %d\n\t  Verts = %d\n",
							s.name,	s.PolygonCount(), 0);
	}

	delete Lwo;

	return 1;
}

int processUnrealSkel(char *filename)
{	// This file can be Animation, or it can be Skeltal ...
	UnrealSkeletalModel		model(filename);

	// start
	fprintf(stdout,"Unreal Tournament [(c) EpicGames] Skeletal model format checker \n");
	fprintf(stdout,"- v" PROG_VERSION " - mbristol@bellatlantic.net\n");

	// Header
	fprintf(stdout, "%s\n[file=\t%s]\n",model.getName(),filename);

	// Stuff ...
	fprintf(stdout, "Frame%s:\t\t%d\n",model.FrameCount() != 1 ? "s" : "",model.FrameCount());
	fprintf(stdout, "Point%s:\t\t%d\n",model.PointCount() != 1 ? "s" : "",model.PointCount());
	fprintf(stdout, "Triangle%s:\t%d\n",model.TriangleCount() != 1 ? "s" : "",model.TriangleCount());
	fprintf(stdout, "UV Wedge%s:\t%d\n",model.WedgeCount() != 1 ? "s" : "",model.WedgeCount());
	fprintf(stdout, "Weight%s:\t%d\n",model.WeightCount() != 1 ? "s" : "",model.WeightCount());

	unsigned int i = 0;

	// Materials
	fprintf(stdout, "Material%s:\t%d\n",model.MaterialCount() != 1 ? "s" : "",
										model.MaterialCount());
	for (i = 0; i < model.MaterialCount(); i++)
	{
		int wdgcount = 0;
		for (unsigned int j = 0; j < model.WedgeCount(); j++)
		{
			if (model.Wedge(j).MatIndex == i)
				++wdgcount;
		}
		UNSKEL_VMaterial& mat = model.Material(i);
	// Dump out name and flags
	fprintf(stdout, "\t%-2d [%4.0d Wedges]:\t%s\n",i,wdgcount,mat.MaterialName);
	if ((mat.PolyFlags & 0x7) == MTT_NormalTwoSided)
	fprintf(stdout, "\t\t\t\t Normal, Two Sided\n");
	else if ((mat.PolyFlags & 0x7) == MTT_Translucent)
	fprintf(stdout, "\t\t\t\t Translucent\n");
	else if ((mat.PolyFlags & 0x7) == MTT_Masked)
	fprintf(stdout, "\t\t\t\t Masked\n");
	else if ((mat.PolyFlags & 0x7) == MTT_Modulate)
	fprintf(stdout, "\t\t\t\t Modulat...ing ?\n");
	else if (mat.PolyFlags & MTT_Placeholder)
	fprintf(stdout, "\t\t\t\t Weapon Placeholder\n");
	else
	fprintf(stdout, "\t\t\t\t Normal, One sided\n");

	if (mat.PolyFlags & MTT_Unlit)
	fprintf(stdout, "\t\t\t\t  + Unlit\n");
	if (mat.PolyFlags & MTT_Flat)
	fprintf(stdout, "\t\t\t\t  + Flat Polygon\n");
	if (mat.PolyFlags & MTT_Alpha)
	fprintf(stdout, "\t\t\t\t  + Alpha Mapped\n");
	if (mat.PolyFlags & MTT_Environment)
	fprintf(stdout, "\t\t\t\t  + Environmental Mapping\n");
	if (mat.PolyFlags & MTT_NoSmooth)
	fprintf(stdout, "\t\t\t\t  + No Smoothing\n");
	}


	// Bones
	fprintf(stdout, "Bone%s:\t%d\n",model.BoneCount() != 1 ? "s" : "",
									model.BoneCount());
	for (i = 0; i < model.BoneCount(); i++)
	{
		UNSKEL_FNamedBoneBinary& bone = model.Bone(i);
		int iMappedPoints = 0;
		for (unsigned int j = 0; j < model.WeightCount(); j++)
		{
			if (model.Weight(j).BoneIndex == i)
			{
				++iMappedPoints;
			}
		}
	fprintf(stdout, "\t%2.2d:\t%-12s \n\t\t  Parent=%2.2d, %d kid%s, %d points affected\n",
					i, bone.Name, bone.ParentIndex, bone.NumChildren,
					(bone.NumChildren != 1) ? "s" : "",iMappedPoints);
	fprintf(stdout, "\t\t  Rest Loc:\t(%3.3f, %3.3f, %3.3f)\n"
					"\t\t  Rest Dir:\t(%3.3f, %3.3f, %3.3f) %3.3f\n",
					bone.BonePos.Position[0],bone.BonePos.Position	[1],
					bone.BonePos.Position[2],
							bone.BonePos.Orientation.X, bone.BonePos.Orientation.Y,
							bone.BonePos.Orientation.Z, bone.BonePos.Orientation.W);
/*
	FQuat& q = bone.BonePos.Orientation;

	Matrix<float>	Mrot = (Quaternion<float>(q.X,q.Y,q.Z,q.W)).getMatrix();
	Vector<float>	Vrot = (~LW_TO_UNSKEL_Coords) * (~Mrot).getEuler();

	static double TO_RAD = (acos(-1) / 180);
	fprintf(stdout, "\t\t    LW HPB:\t(%3.3f, %3.3f, %3.3f)\n",
							-Vrot[1]/TO_RAD,
							-Vrot[0]/TO_RAD,
							-Vrot[2]/TO_RAD);
*/	}

	// Animations
	fprintf(stdout, "Animation Sequence%s:\t%d\n",
							model.AnimationCount() != 1 ? "s" : "",
							model.AnimationCount());
	for (i = 0; i < model.AnimationCount(); i++)
	{
		UNSKEL_Animation&	anim = model.Animation(i);
	fprintf(stdout, "\t'%s' (of '%s')\n\t  %d+%d Keyframe%s * %d Bones = %d Keyframe%s [%d%s]\n",
												anim.Name, anim.Group,
												anim.FirstRawFrame, anim.NumRawFrames,
												(anim.NumRawFrames != 1 ? "s" : ""),
												anim.TotalBones,
												anim.KeyFrameCount(),
												(anim.KeyFrameCount() != 1 ? "s" : ""),
												anim.StartBone,
												(anim.RootInclude == 0) ? " no root" : ""
												);

	fprintf(stdout, "\t  Keystuff:\t[%d %d %3.3f]\n\t  Times:\t[%3.3f, %3.3f]\n",
			anim.KeyCompressionStyle, anim.KeyQuotum, anim.KeyReduction,
			anim.AnimRate,anim.TrackTime);

		// Heck, dump out the Quats
		if (verboselevel == 0)
			continue;

		for (unsigned int j = 0; j < anim.TotalBones; j++)
		{
			for (unsigned int k = 0; k < anim.NumRawFrames; k++)
			{
				UNSKEL_VQuatAnimKey& key = anim.KeyFrame((k * anim.TotalBones) + j);
	fprintf(stdout, "\t [%3d:%-3d] %3.3fx (%3.3f, %3.3f, %3.3f) \t(%3.3f, %3.3f, %3.3f)%3.3f\n",
					j, k,	key.Time,
							key.Position[0], key.Position[1], key.Position[2],
							key.Orientation.X, key.Orientation.Y,
							key.Orientation.Z, key.Orientation.W);
			}
		}
	}

	return 1;
}

#ifdef WIN32
#include <crtdbg.h>
#endif

void usage(char *name)
{
	fprintf(stdout,"File info utility v"PROG_VERSION"\n");
	fprintf(stdout,"%s [+v] <.md3|.lwo|.psk|.psa|.md4 file>\n",name);
}

int main (int argc, char *argv[])
{
#ifdef WIN32
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |
	_CRTDBG_LEAK_CHECK_DF);
#endif

	if (argc < 2)
	{
		usage(argv[0]);
		return -1;
	}

	int i = 1;
	if (strcmp(argv[i],"+v") == 0)
	{
		verboselevel = 1;
		++i;
		if (argc < 3)
		{
			usage(argv[0]);
			return -1;		
		}
	}

	char *file1 = argv[i];
	size_t len = strlen(file1);
	char *cExt = file1 + len - 4;
	
	if (strcmp(cExt,".foo") == 0)
	{
		processFOO(file1);
	}
	else if (cExt[0] == '.' &&
			 (cExt[1] == 'M' || cExt[1] == 'm') &&
			 (cExt[2] == 'D' || cExt[2] == 'd') )
	{
		if (cExt[3] == '4')
		{
			processMD4(file1);
		}
		else if (cExt[3] == '3')
		{
			processMD3(file1);
		}
		else if (cExt[3] == '2')
		{
			processMD2(file1);
		}
		else if (cExt[3] == 'S' || cExt[3] == 's')
		{
			processMDS(file1);
		}
		else
		{
			fprintf(stdout,"Unknown Quake extention on '%s' ??\n",file1);
		}
	}

	else if (cExt[0] == '.' &&
			 (cExt[1] == 'L' || cExt[1] == 'l') &&
			 (cExt[2] == 'W' || cExt[2] == 'w') &&
			 (cExt[3] == 'O' || cExt[3] == 'o') )
	{
		processLWO(file1);
	}
	else if (cExt[0] == '.' &&
			 (cExt[1] == 'P' || cExt[1] == 'p') &&
			 (cExt[2] == 'S' || cExt[2] == 's') &&
			 (cExt[3] == 'K' || cExt[3] == 'k' ||
			  cExt[3] == 'A' || cExt[3] == 'a') )
	{
		processUnrealSkel(file1);
	}
	else
	{
		fprintf(stdout,"Don't recognize extension on '%s'\n",file1);

		return -1;
	}

	if (argc < (3 + verboselevel))
		return 1;

	char *file2 = argv[++i];
	len = strlen(file2);

	if ((strcmp(file2 + len - 4,".md3") == 0) ||
		(strcmp(file2 + len - 4,".MD3") == 0)	)
		processMD3(file2);
	else if
	   ((strcmp(file2 + len - 4,".md2") == 0) ||
		(strcmp(file2 + len - 4,".MD2") == 0)	)
		processMD2(file2);
	else if
	   ((strcmp(file2 + len - 4,".lwo") == 0) ||
		(strcmp(file2 + len - 4,".LWO") == 0)	)
		processLWO(file2);
	else
	{
		fprintf(stdout,"Don't recognize extension on '%s'\n",file2);

		return -1;                     
	}

	fprintf(stdout,"\n==== DIFFERENCES == %s = %s =\n\n",
				file1, file2);

	MD3	a = MD3(file1);
	MD3	b = MD3(file2);

	compareMD3(a,b);

	return 1;
}