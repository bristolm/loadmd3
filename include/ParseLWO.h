/**************************************
 *
 *  parselwo.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wrapper of NewTek's LWO file format
 *
 *  LWO file format property of NewTek, Inc.
 *
 **************************************/

#ifndef  _PARSE_LWO_H_
#define  _PARSE_LWO_H_

#include "sys_math.h"
#include "ParseLWOcommon.h"

namespace LW3D
{
	class  LWOv1 : public LWO
	{
		const char *fileID()
		{
			return "LWOB";
		}
		const char *fileType()
		{
			return "Lightwave Object";
		}

		void ParseVerts(IFF::Chunk& b);
		void ParsePolys(IFF::Chunk& b);
		void ParseSurfs(IFF::Chunk& b);

	public:
		LWOv1(const char *filename);

		void ProcessNextChunkfromFile(IFF::Chunk& b);
	};
}

#endif // _PARSE_LWO_H_
