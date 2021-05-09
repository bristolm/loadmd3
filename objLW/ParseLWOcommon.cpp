/**************************************
 *
 *  parselwocommon.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Common header for LWO-based file formats
 *
 *  LWO file format property of NewTek, Inc.
 *
 **************************************/

#include <stdio.h>
#include <stdlib.h>

#include "ParseLWOcommon.h"

#include "string.h"
#include "math.h"

using namespace IFF;
using namespace LW3D;

UINT32 Chunk::getSome(unsigned char *buff, FILE *fp, UINT32 len)
{
	memset(buff,0,len);

	if (fread(buff,sizeof(unsigned char),len,fp) != len)
        return(0);

	buff[len] = '\0';
	totalread += len;
	
	return len;
}

UINT32 Chunk::Read(FILE *fp, const ChunkFunction *funcs)
{
	if (getSome(buffer,fp,8) < 8)
		return 0;

	// FIXME:  The size (bytes 5,6,7,8) is in network order
#if defined(WIN32) || defined(DJGPP)
	Swap4(buffer +4);
#endif

	UINT32 chunksize = Size();

	if (chunksize & 0xFF000000)
	{	// assume this is too big and try swapping it - LWOB files saved by 6.0 seem
		// to do this
		Swap4(buffer +4);
		chunksize = Size();
	}

	// parse it out regardless if we care or not, then return
//	fprintf(stderr,"CHUNK - '%c%c%c%c' - %d bytes (of %d)\n",
//			*buffer, *(buffer +1), *(buffer +2), *(buffer+3),
//			 chunksize, TotalDataRead());	

	if (bufsize < chunksize +8)
	{	// We need more space
		chunksize +=8;
		while (bufsize < chunksize)
			bufsize += bufinc;

		unsigned char	*tmp = new unsigned char[bufsize];
		memcpy(tmp,buffer,9);

		delete buffer;
		buffer = tmp;

		chunksize -=8;
	}
	
	if (getSome(buffer + 8,fp,chunksize) != chunksize)
		return 0;

	return *((UINT32 *)buffer);
}

Chunk::Chunk(size_t szbuf) :
	 bufsize(szbuf),
	 bufinc(szbuf),
	 totalread(0)
{
	buffer = new unsigned char[szbuf];
}

// LWO_Object
void LWO::GetPolyNormal(UINT32 polyidx, double d[])			// fill in a polygon's normal
{
	int i = 0;
	Polygon&	p = getPolygon(polyidx);
	Vertex		v[3];

	for (i = 0; i < 3;i++)
		v[i] = getVertex(p.pntindices[i]);

	Matrix<float> M(getVertex(p.pntindices[0]).pos,
					getVertex(p.pntindices[1]).pos,
					getVertex(p.pntindices[2]).pos);
	Vector<float> V = M.getNormal();

	d[0] = V[0];
	d[1] = V[1];
	d[2] = V[2];
}

LWO::LWO(const char *filename):
		invalidUV(UV()),
		valid(0),
		work(0x1000),
		Verts(),
		Polys(),
		Surfs(),
		VMaps(),
		FileName(filename)
{
	if ((fp = fopen(filename,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open '%s'\n",filename);
		goto RETURN;
	}

	// FORM <4 bytes size> LWOB|LWO2

	if (work.getSome(tmp,fp,12) < 0)
		goto RETURN;

	if (strncmp((const char *)tmp,IFF_FILE_ID,4) != 0)		// FORM
		goto RETURN;

	return;

RETURN:
	valid = 0;
	if (fp)
		fclose(fp);
	fp = 0;
}

void LWO::ParsefromFile()
{
	if (fp == 0)
		return;

	if (strncmp((const char *)tmp +8,FileID(),4) != 0)	// LWOB
		goto RETURN;

//	fprintf(stderr,"Reading %s file '%s'\n",FileType(),FileName);

	while(work.Read(fp,0))
		ProcessNextChunkfromFile(work);

	// REPLACE
	//Verts.LockMax();
	//Polys.LockMax();
	//Surfs.LockMax();
	//VMaps.LockMax();

	valid = 1;
	return;

RETURN:
	valid = 0;
	if (fp)
		fclose(fp);
	fp = 0;
	return;
}
		
LWO::~LWO()
{
	if (fp)
		fclose(fp);
	fp = 0;
}


