/* Original code comments:

Copyright (C) Matthew 'pagan' Baranowski & Sander 'FireStorm' van Rossen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  NOTES:
  Wrapper of id software's MD3 file format
  Altered significantly by
     Michael Bristol mbristol@bellatlantic.net

*/

#include "sys_math.h"
#include "sys_extra.h"

using namespace MRB;

#ifndef _ID_MD3_H_
#define _ID_MD3_H_

#define ID_HEADER_MD3	'3PDI'	// big endian: 'IDP3'

#define MAX_MD3SKINNAME 64
#define MD3_MAX_LODS		3
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	32		// per model
#define MD3_MAX_TAGS		16		// per frame

typedef struct
{
	UINT32	ID;					//id of file, always ID_HEADER_MD3
	UINT32	Version;			//i suspect this is a version number, always 15
	char	Name[68];			//sometimes left Blank...always 64 chars

	UINT32	Frame_num;			//number of frames
	UINT32	Tag_num;			//number of 'tags'
	UINT32	Mesh_num;			//number of meshes/skins 
	UINT32	MaxSkin_num;		//unknown, always zero

	UINT32	Frame_Start;		//always equal to the length of the header
	UINT32	Tag_Start;			//starting position of tag-structures
	UINT32	Mesh_Start;			//ending position of tag-structures/starting 
								//position of mesh-structures

	UINT32	FileSize;			//size of file
} MD3_Header;

#endif // _ID_MD3_H_


typedef struct
{
	float	Mins[3];			//unknown vector, minimum size? always negative
	float	Maxs[3];			//unknown vector, probably maximum size? always positive
	float	Position[3];		//new position
	float	Radius;				//For collision?  Maybe visuals.
	char	Creator[16];		//i think this is the "creator" name..						
} stMD3_Frame;

/*
 * There are (Frame_num * Tag_num) tag structures.
 * for each (Frame_num)
 *	for each (Tag_num)
 *		MD3_Tag;
 */

typedef struct
{
	char	Name[12];			//name of 'tag' as it's usually called in the md3 files
								//try to see it as a sub-mesh/seperate mesh-part
	char	unknown1[52];
	Vector<float>	Position;	//relative position of tag
	Matrix<float>	Rotation;	//3x3 rotation matrix
} stMD3_Tag;

/* Each Mesh chunk signifies the beginning of a set of structures like:
 *   Mesh chunk header
 *   for each (skin_num)
 *		MD3_Point			- name or something
 *   for each (Triangle_num)
 *		MD3_Poly			- frame triangles
 *	 for each (Vertex_num)
 *		MD3_SkinVertes		- skin vertices
 *	 for each (MeshFrame_num)
 *		for each (Vertex_num)
 *			MD3_FrameVertex		- distorted position ?
 */
typedef struct
{
	UINT32	ID;					//id, must be IDP3
	char	Name[68];			//name of mesh

	UINT32	MeshFrame_num;		//frames, variable
	UINT32	Skin_num;			//unknown, always 1 ...maybe "has skin" flag?
	UINT32	Vertex_num;			//number of vertices
	UINT32	Triangle_num;		//number of Triangles

	UINT32	Triangle_Start;		//starting position of Triangle data, relative to start of meshchunk
	UINT32	HeaderSize;			//size of header
	UINT32	TexVec_Start;		//starting position of texvector data, relative to start of meshchunk
	UINT32	Vertex_Start;		//starting position of vertex data, relative to start of meshchunk
	UINT32	ChunkSize;			//size of mesh
} MD3_MeshHeader;

typedef struct
{
	char	Name[MAX_MD3SKINNAME];	//name of skin used by mesh
	int		fillup;
} stMD3_Skin;

typedef struct
{
	UINT32	vind[3];    // triangle vertices
} stMD3_Poly;

/* The simple fact here is that vertices are duplicated if the skin 
 * has been split into several pieces in one mesh.  This is unlike the 
 * MD2 format where a triangle has separate Texture and vertices indexes.
 */
typedef struct
{
	float	tex[2];	// texture coordinates
} stMD3_Vert_Skin;

/* Explanation about the 'normal' members as taken from 

	Java MD3 Model Viewer - A Java based Quake 3 model viewer.
	Copyright (C) 1999  Erwin 'KLR8' Vervaet

	.. "Spherical coordinates giving the direction of the vertex normal.
	    They are both unsigned byte values.  The first one is the inclination,
	    and the second the rotation in the horizontal plane ..."

  		pitch   = [0] * 2 * PI / 255;
  		heading = [1] * 2 * PI / 255;

  */
typedef struct
{
	INT16  v[3];						//vertex coordinate
	INT8   normal[2];					//environment mapping normal 
} stMD3_Vert_Frame;

typedef stMD3_Vert_Frame stMD3_Point_Frame;

