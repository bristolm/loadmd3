/**************************************
 *
 *  parsemd2.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for id software's MD2 file format
 *
 *  MD2 file format property of id software
 *
 **************************************/

#include <stdio.h>
#include <math.h>
#include "string.h"
#include "ParseMD2.h"

static char	tmp[5];
static char *id_ID = "IDP2";

#define CHECK_FILE_POS(f,i,x)											\
	if (i != x) {														\
		fprintf(stderr,"--Cursor invalid: %d != expected %d\n",			\
					i, x);												\
		fseek(fp,(long)x,SEEK_SET);  }

// Static values
MD2_Frame		MD2_Frame::INVALID=			MD2_Frame();
MD2_Skin		MD2_Skin::INVALID =			MD2_Skin();
MD2_Poly		MD2_Poly::INVALID =			MD2_Poly();
MD2_Point_Frame	MD2_Point_Frame::INVALID =	MD2_Point_Frame();
MD2_FramePoints	MD2_FramePoints::INVALID =	MD2_FramePoints();
MD2_Point_Skin	MD2_Point_Skin::INVALID =	MD2_Point_Skin();

Matrix<float>	LW_TO_MD2_Coords(Vector<float>(0,-1,0),
								 Vector<float>(0,0,1),
								 Vector<float>(1,0,0));

// ==================
// From id software q3data - light normal indices
#define NUMVERTEXNORMALS	162

#pragma warning( disable : 4305 )
float	avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};
// ==================

// Find the closest of the canned normals
void MD2_Point_Frame::setNormal(Vector<float> &v)
{	
	float maxdot = -999999.0;
	int maxdotindex = -1;

	for (int j=0 ; j<NUMVERTEXNORMALS ; j++)
	{
		float   dot =	v[0] * avertexnormals[j][0] 
						+ v[1] * avertexnormals[j][1] 
						+ v[2] * avertexnormals[j][2];

		if (dot > maxdot)
		{
			maxdot = dot;
			maxdotindex = j;
		}
	}

	innards.lightnormalindex = maxdotindex;
}

// MD2 functions
char *MD2::TypeName(char *buf)
{
	if (Type() == 0)
		buf[0] = 0;
	else
	{
		*((UINT32 *)buf) = Type();
		buf[4] = 0;
	}

	return buf;
}

int MD2::isLoaded()
{
	return (strcmp(TypeName(tmp),id_ID) == 0);
}

// Ready for export
UINT32 MD2::UpdateHeaderSizes()
{
	UINT32 cur = 0;

	cur = sizeof(Header);

	Header.Skin_Start = cur;
	Header.Skin_num = Skins.Next();
	cur += (sizeof(MD2Skin) * Header.Skin_num);

	Header.TexVec_Start = cur;
	Header.TexVec_num = SkinPoints.Next();
	cur += (sizeof(MD2Vert_Skin) * Header.TexVec_num);

	Header.Triangle_Start = cur;
	Header.Triangle_num = Polys.Next();
	cur += (sizeof(MD2Poly) * Header.Triangle_num);

	Header.Frame_Start = cur;
	Header.Frame_num = FramePoints.Next();
	Header.Vertex_num = FramePoints[0].Next();
	Header.Frame_size = sizeof(MD2Frame)
			+ (sizeof(MD2Point_Frame) * Header.Vertex_num );
	cur += (Header.Frame_size * Header.Frame_num);

	Header.Strips_Start = cur;	
	Header.Strips_num = GL_Strips.Next();
	cur += (sizeof(MD2StripItem) * Header.Strips_num);

	Header.FileSize = cur;

	return Header.FileSize;
}

MD2::MD2():
	Frames(MD2_Frame::INVALID),
	Polys(MD2_Poly::INVALID),
	Skins(MD2_Skin::INVALID),
	FramePoints(MD2_FramePoints::INVALID),
	SkinPoints(MD2_Point_Skin::INVALID),
	GL_Strips(0),
	FileName()
{
	memcpy((void *)(&(Header.ID)),(void *)id_ID,4);
	Header.Version  = 8;

	UpdateHeaderSizes();
}

MD2::MD2(const char *filename):
	Frames(MD2_Frame::INVALID),
	Polys(MD2_Poly::INVALID),
	Skins(MD2_Skin::INVALID),
	FramePoints(MD2_FramePoints::INVALID),
	SkinPoints(MD2_Point_Skin::INVALID),
	GL_Strips(0),
	FileName(filename)
{
	UINT32	j = 0,nextchunk = 0;
	int		partsdone = 0, i = 0;

	if ((fp = fopen(filename,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open existing '%s'\n",filename);
		goto RETURN;
	}

	// ALWAYS refer to things through the Header at this point

	// Read in header
	nextchunk = sizeof(MD2_Header);
	i = fread((void *)&Header,nextchunk,1,fp);
	if (!isLoaded())
		goto RETURN;

	// Read in skin data
	fseek(fp,Header.Skin_Start,SEEK_SET);
	nextchunk = Header.Skin_num;
	if (nextchunk > 0)
	{
		for (j = 0; j < nextchunk;j++)
			i = fread(&Skins[j],sizeof(MD2Skin),1,fp);
	}

	// Read in Polygon data
	fseek(fp,Header.Triangle_Start,SEEK_SET);
	nextchunk = Header.Triangle_num;
	for (j = 0; j < nextchunk;j++)
		i = fread(&Polys[j],sizeof(MD2Poly),1,fp);

	// Read in skin verts
	fseek(fp,Header.TexVec_Start,SEEK_SET);
	nextchunk = Header.TexVec_num;
	if (nextchunk > 0)
	{
		for (j = 0; j < nextchunk;j++)
		{
			MD2_Point_Skin& skpnt = SkinPoints[j];
			i = fread(&(skpnt.innards),sizeof(MD2Point_Skin),1,fp);

			// Fill in our surrogate values
			skpnt.tex[0] = ((float)skpnt.innards.tex[0] / Header.Skin_width);
			skpnt.tex[1] = ((float)skpnt.innards.tex[1] / Header.Skin_height);
		}
	}

	// Vertex data Start point
	// - In the file this is really [FrameCount()][MD2_Frame + MD3_Point_Frame * PointCount()]
	fseek(fp,Header.Frame_Start,SEEK_SET);
	nextchunk = Header.Vertex_num;
	for (j = 0; j < Header.Frame_num; j++)
	{
		i = fread(&Frames[j],sizeof(MD2Frame),1,fp);
		for (UINT32 k = 0; k < nextchunk; k++)
		{
			MD2_Point_Frame& pntfr = FramePoints[j][k];
			i = fread(&(pntfr.innards),sizeof(MD2Point_Frame),1,fp);

			// Fill in our surrogate values
			for (int didx = 0;didx < 3; didx++)
			{
				float f = (float)(pntfr.innards.v[didx]);
				f *= Frames[j].Scale[didx];
				f += Frames[j].Translate[didx];

				pntfr.v[didx] = f;
			}
		}
	}

	// Read in strip data
	fseek(fp,Header.Strips_Start,SEEK_SET);
	nextchunk = Header.Strips_num;
	for (j = 0; j < nextchunk;j++)
		i = fread(&GL_Strips[j],sizeof(MD2StripItem),1,fp);

//	fprintf(stderr,"Read in %d - expected %d\n",cur,Header.FileSize);

RETURN:
	if (fp) fclose(fp);
	fp = 0;
};

void MD2::WritetoDisk(const char *file)
{
	if (file == (const char *)NULL)
	{
		if (FileName[0] == 0)
			return;
		file = FileName;
	}

	if ((fp = fopen(file,"wb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open '%s' for write\n",file);
			return;
	}

	setvbuf( fp, NULL, _IOFBF, 0x8000 ) ;

	// Build GL instructions
	BuildGLStrips();

	// Just recheck all the header values
	UpdateHeaderSizes();

	// Write out header
	int		cur = 0;
	UINT32	nextchunk = sizeof(MD2_Header);
	cur += fwrite((void *)&Header,1,nextchunk,fp);

	UINT32	i = 0, j = 0, k = 0;
	// Write out skins
	nextchunk = SkinCount(); 
	if (nextchunk != 0)
	{
		for (UINT32 j =0; j < nextchunk; j++) {
			i = fwrite(&Skins[j],sizeof(MD2Skin),1,fp);
		}
	}

	// Write out skin verts
	nextchunk = SkinPointCount();
	if (nextchunk != 0)
	{
		for (j = 0; j < nextchunk; j++)
		{	
			MD2_Point_Skin& skpnt = SkinPoints[j];

			// Update the output value
			skpnt.innards.tex[0] = skpnt.tex[0] * Header.Skin_width;
			skpnt.innards.tex[1] = skpnt.tex[1] * Header.Skin_height;

			i = fwrite(&(skpnt.innards),sizeof(MD2Point_Skin),1,fp);
		}
	}

	// Write out triangles
	nextchunk = PolyCount();
	for (j = 0; j < nextchunk; j++) {
		i = fwrite(&Polys[j],sizeof(MD2Poly),1,fp);
	}

	// Write out frame data
	nextchunk = FrameCount();
	for (j = 0; j < nextchunk; j++)
	{
		MD2_Frame& fr = Frames[j];

		// Figure out the min/max ranges
		float mins[3] = {999.0f,999.0f,999.0f};
		float maxs[3] = {-999.0f,-999.0f,-999.0f};
		unsigned int pntcount = PointCount();
		for (k = 0; k < pntcount; k++)
		{
			MD2_Point_Frame& frpnt = FramePoints[j][k];
			for (int d = 0; d < 3; d++)
			{
				if(frpnt.v[d] < mins[d])
					mins[d] = frpnt.v[d];
				if(frpnt.v[d] > maxs[d])
					maxs[d] = frpnt.v[d];
			}
		}

		for (int d = 0; d < 3; d++)
		{
			fr.Scale[d] = (maxs[d] - mins[d]) / 255.0f;
			fr.Translate[d] = mins[d];
		}

		// Write data
		i = fwrite(&Frames[j],sizeof(MD2Frame),1,fp);

		for (k = 0; k < pntcount; k++)
		{
			// figure out the 'real' vertex values
			MD2_Point_Frame& frpnt = FramePoints[j][k];
			for (int d = 0; d < 3; d++)
			{
				float f = (frpnt.v[d] - fr.Translate[d]) / fr.Scale[d];
				if (f < 0)		f = 0.0f;
				if (f > 255)	f = 255.0f;

				frpnt.innards.v[d] = (UINT8)f;
			}

			// Figure out the normals
			i = fwrite(&(frpnt.innards),sizeof(MD2Point_Frame),1,fp);
		}
	}

	// Write out GL command data
	nextchunk = Header.Strips_num;
	for (j = 0; j < nextchunk; j++) {
		i = fwrite(&GL_Strips[j],sizeof(MD2StripItem),1,fp);
	}

	fclose(fp);
	fp = 0;
}

// From code released by id software
// models.c::BuildGlCmds  

/*
================
StripLength
================
*/

// the command list holds counts, s/t values, and xyz indexes
// that are valid for every frame

int	MD2::StripLength (GL_Work& work, int starttri, int startv)
{
	int			m1, m2;
	int			st1, st2;
	int			j;
	int			k;
	int			stripcount;

	work.used[starttri] = 2;

	MD2_Poly& last = Polys[starttri];

	work.strip_xyz[0] = last.vertex[(startv)%3];
	work.strip_xyz[1] = last.vertex[(startv+1)%3];
	work.strip_xyz[2] = last.vertex[(startv+2)%3];

	work.strip_st[0] = last.texvec[(startv)%3];
	work.strip_st[1] = last.texvec[(startv+1)%3];
	work.strip_st[2] = last.texvec[(startv+2)%3];

	work.strip_tris[0] = starttri;
	stripcount = 1;

	m1 = last.vertex[(startv+2)%3];
	st1 = last.texvec[(startv+2)%3];
	m2 = last.vertex[(startv+1)%3];
	st2 = last.texvec[(startv+1)%3];

	// look for a matching triangle
	UINT32 polycount = PolyCount();
nexttri:
	for (j=starttri+1 ; j< polycount; j++)
	{
		MD2_Poly& check = Polys[j];
		for (k=0 ; k<3 ; k++)
		{
			if (check.vertex[k] != m1)
				continue;
			if (check.texvec[k] != st1)
				continue;
			if (check.vertex[ (k+1)%3 ] != m2)
				continue;
			if (check.texvec[ (k+1)%3 ] != st2)
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (work.used[j])
				goto done;

			// the new edge
			if (stripcount & 1)
			{
				m2 = check.vertex[ (k+2)%3 ];
				st2 = check.texvec[ (k+2)%3 ];
			}
			else
			{
				m1 = check.vertex[ (k+2)%3 ];
				st1 = check.texvec[ (k+2)%3 ];
			}

			work.strip_xyz[stripcount+2] = check.vertex[ (k+2)%3 ];
			work.strip_st[stripcount+2] = check.texvec[ (k+2)%3 ];
			work.strip_tris[stripcount] = j;
			stripcount++;

			work.used[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (j=starttri+1 ; j<polycount; j++)
		if (work.used[j] == 2)
			work.used[j] = 0;

	return stripcount;
}

/*
===========
FanLength
===========
*/
int	MD2::FanLength (GL_Work& work, int starttri, int startv)
{
	int		m1, m2;
	int		st1, st2;
	int		j;
	int		k;
	int		stripcount;

	work.used[starttri] = 2;

	MD2_Poly& last = Polys[starttri];

	work.strip_xyz[0] = last.vertex[(startv)%3];
	work.strip_xyz[1] = last.vertex[(startv+1)%3];
	work.strip_xyz[2] = last.vertex[(startv+2)%3];

	work.strip_st[0] = last.texvec[(startv)%3];
	work.strip_st[1] = last.texvec[(startv+1)%3];
	work.strip_st[2] = last.texvec[(startv+2)%3];

	work.strip_tris[0] = starttri;
	stripcount = 1;

	m1 = last.vertex[(startv+0)%3];
	st1 = last.texvec[(startv+0)%3];
	m2 = last.vertex[(startv+2)%3];
	st2 = last.texvec[(startv+2)%3];


	// look for a matching triangle
	UINT32 polycount = PolyCount();
nexttri:
	for (j=starttri+1; j<polycount ; j++)
	{
		MD2_Poly& check = Polys[j];
		for (k=0 ; k<3 ; k++)
		{
			if (check.vertex[k] != m1)
				continue;
			if (check.texvec[k] != st1)
				continue;
			if (check.vertex[ (k+1)%3 ] != m2)
				continue;
			if (check.texvec[ (k+1)%3 ] != st2)
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (work.used[j])
				goto done;

			// the new edge
			m2 = check.vertex[ (k+2)%3 ];
			st2 = check.texvec[ (k+2)%3 ];

			work.strip_xyz[stripcount+2] = m2;
			work.strip_st[stripcount+2] = st2;
			work.strip_tris[stripcount] = j;
			stripcount++;

			work.used[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (j=starttri+1 ; j<polycount ; j++)
		if (work.used[j] == 2)
			work.used[j] = 0;

	return stripcount;
}

/*
================
BuildGLCmds

Generate a list of trifans or strips
for the model, which holds for all frames
================
*/
void MD2::BuildGLStrips(void)
{
	unsigned int	i, j, k;
	int		startv;
	float	s, t;
	int		len, bestlen, besttype;
	int		best_xyz[1024];
	int		best_st[1024];
	int		best_tris[1024];
	int		type;

	GL_Strips = AutoArray<MD2_StripItem>(0);


	//
	// build tristrips
	//
	unsigned int polycount = PolyCount();
	GL_Work work;
	memset (&work, 0, sizeof(work));

	for (i=0 ; i<polycount ; i++)
	{
		// pick an unused triangle and start the trifan
		if (work.used[i])
			continue;

		bestlen = 0;
		for (type = 0 ; type < 2 ; type++)
//	type = 1;
		{
			for (startv =0 ; startv < 3 ; startv++)
			{
				if (type == 1)
					len = StripLength (work, i, startv);
				else
					len = FanLength (work, i, startv);

				if (len > bestlen)
				{
					besttype = type;
					bestlen = len;
					for (j=0 ; j<bestlen+2 ; j++)
					{
						best_st[j] = work.strip_st[j];
						best_xyz[j] = work.strip_xyz[j];
					}
					for (j=0 ; j<bestlen ; j++)
						best_tris[j] = work.strip_tris[j];
				}
			}
		}

		// mark the tris on the best strip/fan as used
		for (j=0 ; j<bestlen ; j++)
			work.used[best_tris[j]] = 1;

		if (besttype == 1)
			GL_Strips[GL_Strips.Next()] = (UINT32)(bestlen+2);
		else
			GL_Strips[GL_Strips.Next()] = (UINT32)(-(bestlen+2));

		for (j=0 ; j<bestlen+2 ; j++)
		{
			// emit a vertex into the reorder buffer
			k = best_st[j];

			// emit s/t coords into the commands stream
			s = SkinPoints[k].tex[0];
			t = SkinPoints[k].tex[1];

			GL_Strips[GL_Strips.Next()] = *(UINT32 *)&s;
			GL_Strips[GL_Strips.Next()] = *(UINT32 *)&t;
			GL_Strips[GL_Strips.Next()] = *(UINT32 *)&(best_xyz[j]);
		}
	}
	GL_Strips[GL_Strips.Next()] = 0;		// end of list marker
}
