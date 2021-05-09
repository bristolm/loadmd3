/**************************************
 *
 *  LoadCfgFile.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *  
 *  Parses a [file].cfg file for the LWOtoMD3
 *  utility - used to convert a series of LWO/LWO2
 *  files into one or more Quake MD3 model file
 *
 *  MD3 file format property of id software
 *
 **************************************/

#include <stdio.h>
#include <stdlib.h>

#include "string.h"
#include "math.h"

#include "ParseLWO.h"
#include "ParseLWO2.h"
#include "ParseMD3.h"
#include "LWOtoMD3.h"

static char			MD3_Name[256];
static char			Global_FileDir[1024];

static LWO_stub				*WorkFileList	= (LWO_stub *)NULL;
static Material_Reference	*WorkMesh		= (Material_Reference *)NULL;
static Material_Reference	*WorkTag		= (Material_Reference *)NULL;

static int					framecount = -1;

static char *extension = "lwo";
static char prefix[MAX_NAMELEN *2];

void LinkMeshesToWorkFileList();

// DEFINITIONS

typedef enum
{
	TOKEN_MD3 = 0,
	TOKEN_NAME,

	TOKEN_MESH,
	TOKEN_TAG,

	TOKEN_IMAGES,
	TOKEN_SURFS,
	TOKEN_VMAP,
	TOKEN_FILEDIR,
	TOKEN_FILELIST,
	TOKEN_SINGLE,
	TOKEN_RANGE,
	TOKEN_OPENBRACKET,
	TOKEN_CLOSEBRACKET,
	TOKEN_COMMENT,
	TOKEN_EOL,
	TOKEN_TAB,
	TOKEN_BASE,
	TOKEN_NOUPDATE,

	TOKEN_UNKNOWN	= 100,
	TOKEN_EOF		= 101
} ENUM_TOKENS;

#define _TOKENLENGTH	256
#define _CULLTOKEN(v,ch)	\
	v <<= 7;				\
	v |= (ch & 0xeF);


class cfgTokenizer;

/*
 * Configuration File Token Template class
 */
class cfgTokenTemplate
{
	friend cfgTokenizer;

	char		_tokenvalue[_TOKENLENGTH];
	ENUM_TOKENS	_tokentype;

public:
	cfgTokenTemplate(const char *name, ENUM_TOKENS type):
	  _tokentype(type)
	{
		strcpy(_tokenvalue,name);
	}

	cfgTokenTemplate(char value,ENUM_TOKENS type):
	  _tokentype(type)
	{
		_tokenvalue[0] = value;
		_tokenvalue[1] = 0;
	}

	ENUM_TOKENS Type() {return _tokentype;}
	char		*Value() {return _tokenvalue;}
};

/*
 * Configuration File Token class
 */
class cfgToken
{
	char			_value[_TOKENLENGTH];
	ENUM_TOKENS		_type;

public:
	cfgToken(cfgTokenTemplate& c, char *value = 0):
	  _type(c.Type())
	{
	  if (value && *value)
		strcpy(_value,value);
	  else
		_value[0] = 0;
	}

	inline ENUM_TOKENS	Type() {return _type;}
	inline char			*Value(){return _value;}
};

/*
 * Configuration File Tokenizer class
 */
class cfgTokenizer
{
	cfgTokenTemplate	_unknown;
	cfgTokenTemplate	_eof;
	char				_buff[_TOKENLENGTH];
	cfgTokenTemplate*	_validlist;
	int					_listsize;

public:
	cfgTokenizer(cfgTokenTemplate array[], int count):
	    _unknown((char)0,TOKEN_UNKNOWN),
		_eof((char)0,TOKEN_EOF),
		_validlist(&array[0]),
		_listsize(count)
		{;}

	cfgToken		nextToken(char **);
	cfgToken		EndOfFile();
};

cfgToken cfgTokenizer::EndOfFile()
{
	return cfgToken(_eof);
}

cfgToken cfgTokenizer::nextToken(char **buf)
{
	if (**buf == 0)
		return cfgToken(_eof);
	
	char *tok = _buff;
	char *p = *buf;
	int j = 0;

	*tok = 0;

	while (*p == 0x20)		// strip off beginning spaces
		++p;

	if (*p > 0x20)
	{	// found a character string - copy into token buffer
		while (*p > 0x20)
		{
			*tok++ = *p++;
		}
	}
	else if (*p > 0)
	{	// found an end of line or something - just one char
			*tok++ = *p++;
	}

	*buf = p;		// store cursor position
	*tok = 0;		// terminate token string

	// find the appropriate token in the template list
	for (j = 0; j < _listsize; j++)
	{
		if (strcmp(_validlist[j].Value(),_buff) == 0)
			return cfgToken(_validlist[j],_buff);
	}

	return cfgToken(_unknown,_buff);
}

int ProcessFileBase (cfgToken tok)
{
	if (tok.Value() == 0)
	{
		fprintf(stderr,"ERROR - Invalid base file name");
		return 0;
	}

	sprintf(prefix,"%s%s",Global_FileDir ? Global_FileDir : "", tok.Value());

	// Just the raw filename - one frame
	strcat(prefix,".");
	strcat(prefix,extension);

	char *p = new char[strlen(prefix) + 1];
	strcpy(p,prefix);

/*
	if (WorkFileList)
		WorkFileList = WorkFileList->InsertTail(new LWO_stub(prefix));
	else
		WorkFileList = new LWO_stub(prefix);
*/
	if (WorkFileList->SetBaseObject(p) == 0)
		delete p;
		
	return 1;
}


int ProcessFileSingle (cfgToken tok)
{
	if (tok.Value() == 0)
	{
		fprintf(stderr,"ERROR - Invalid single file name");
		return 0;
	}

	sprintf(prefix,"%s%s",Global_FileDir ? Global_FileDir : "", tok.Value());

	// Just the raw filename - one frame
	strcat(prefix,".");
	strcat(prefix,extension);
/*
	if (WorkFileList)
		WorkFileList = WorkFileList->InsertTail(new LWO_stub(prefix));
	else
		WorkFileList = new LWO_stub(prefix);
*/
	WorkFileList->StoreFileName(prefix);
		
	return 1;
}

int ProcessFileRange (cfgToken span, cfgToken start, cfgToken base)
{
	if (base.Value() == 0 || start.Value() == 0 ||
		span.Value() == 0)
	{
		fprintf(stderr,"ERROR - Invalid file range");
		return 0;
	}

	sprintf(prefix,"%s%s",Global_FileDir ? Global_FileDir : "", base.Value());

	int framestart = atoi(start.Value());
	int frameend = framestart + atoi(span.Value()) -1;

	// Determine the 'suffix' type we're looking for.
	int suffixlen = 0;
	char *p = prefix + strlen(prefix) -1;
	for (;*p == '0';p--,suffixlen++)
		;

	if (suffixlen > 0)
	{
		*(++p) = 0;
		sprintf(p,"%%%d.%dd.%s",suffixlen,suffixlen,extension);
	}
	
	// Fill in all the filenames
	int incoming_framecount = frameend - framestart;
	int frameincrement = 1;
	if (incoming_framecount < 0)
		incoming_framecount *= (frameincrement = -1);

	incoming_framecount++;

	while (1)
	{
		static char tmp[MAX_NAMELEN];
		if (suffixlen)
			sprintf(tmp,prefix,framestart);
		else
			sprintf(tmp,"%s%d.%s",prefix,framestart,extension);

		WorkFileList->StoreFileName(tmp);

		if (framestart == frameend)
			break;

		framestart += frameincrement;
	}

	return incoming_framecount;
}

/*
 * Configuration File Processor class
 */
class cfgFile
{
	char	**_pbuffer;
	char	*_buffer;
	size_t	_buffersize;

	size_t	_filesize;

public:
	static size_t FILEREAD_CHUNK;

	cfgFile(FILE *cfgFile);

	inline size_t Filesize()
	{ return _filesize; }

	virtual int ProcessCfgBuffer() = 0;

	cfgToken ParseOffComment();
	cfgToken ParseOffToken(ENUM_TOKENS tok);
	cfgToken ParseOffNextStringToken();
	cfgToken ParseOffNextToken();

protected:
	virtual cfgTokenizer	&TOKENIZER(void) = 0;
	inline char				**BUFFER()
	{ return  _pbuffer; }
};

cfgFile::cfgFile(FILE *cfgFile):
	_pbuffer(0),
	_buffer(0),
	_buffersize(FILEREAD_CHUNK),
	_filesize(0)
{
	size_t i = 0;

	_buffer = new char[_buffersize];
	_pbuffer = &_buffer;

	while ((i = fread(*(void **)_pbuffer,sizeof(char),FILEREAD_CHUNK,cfgFile)) == FILEREAD_CHUNK)
	{
		char *buf = new char[_buffersize + FILEREAD_CHUNK];
		memcpy(buf,_buffer,_buffersize);
		delete  _buffer;

		_buffer = buf;
		_pbuffer = &_buffer + _buffersize;

		_buffersize += FILEREAD_CHUNK;
	};

	// null terminate the end
	memset(_buffer + i, 0, _buffersize - i);
	_buffer[i] = 0;

	_pbuffer = &_buffer;

	_filesize = _buffersize - FILEREAD_CHUNK + i;
};

#define FIND_TOKEN_OF_TYPE(t)				\
	if (ParseOffToken(t).Type() != t) return TOKENIZER().EndOfFile();

cfgToken cfgFile::ParseOffComment()
{
	cfgToken next(ParseOffNextToken());

	while (next.Type() != TOKEN_EOF && next.Type() != TOKEN_EOL)
		next = ParseOffNextToken();

	return next;
}

cfgToken cfgFile::ParseOffToken(ENUM_TOKENS tok)
{
	cfgToken next(ParseOffNextToken());

	while (next.Type() != tok)
	{
		if (next.Type() == TOKEN_EOF)
			break;

		next = ParseOffNextToken();
	}

	return next;
}

cfgToken cfgFile::ParseOffNextStringToken()
{
	cfgToken next(ParseOffNextToken());

	while (next.Type() != TOKEN_EOF)
	{
		if (next.Type() != TOKEN_TAB)
			return next;

		next = ParseOffNextToken();
	}

	return next;
}

cfgToken cfgFile::ParseOffNextToken()
{
	cfgToken next(TOKENIZER().nextToken(_pbuffer));

	while (next.Type() == TOKEN_COMMENT)
		next = ParseOffComment();

	return next;
}

/*
 * cfgFile object specific to lwotomd3 functionality
 */
class lwotomd3_cfgFile : public cfgFile
{
	static cfgTokenTemplate ValidTokenList[];
	cfgTokenizer _tokenizer;

	int			ProcessOffMD3();
	cfgToken	ProcessOffWorkFileList();
	cfgToken	ProcessOffMeshList(cfgToken startToken);

	cfgTokenizer &TOKENIZER(void)
	{ return _tokenizer;}
public:
	lwotomd3_cfgFile(FILE *f);
	int ProcessCfgBuffer();
};

cfgTokenTemplate lwotomd3_cfgFile::ValidTokenList[] = 
{	cfgTokenTemplate("md3",			TOKEN_MD3),
	cfgTokenTemplate("name",		TOKEN_NAME),
	cfgTokenTemplate("mesh",		TOKEN_MESH),
	cfgTokenTemplate("tag",			TOKEN_TAG),
	cfgTokenTemplate("images",		TOKEN_IMAGES),
	cfgTokenTemplate("surfaces",	TOKEN_SURFS),
	cfgTokenTemplate("txuv",		TOKEN_VMAP),
	cfgTokenTemplate("filedir",		TOKEN_FILEDIR),
	cfgTokenTemplate("filelist",	TOKEN_FILELIST),
	cfgTokenTemplate("single",		TOKEN_SINGLE),
	cfgTokenTemplate("range",		TOKEN_RANGE),
	cfgTokenTemplate("base",		TOKEN_BASE),
	cfgTokenTemplate("noupdate",	TOKEN_NOUPDATE),
	cfgTokenTemplate("{",			TOKEN_OPENBRACKET),
	cfgTokenTemplate("}",			TOKEN_CLOSEBRACKET),
	cfgTokenTemplate("//",			TOKEN_COMMENT),

	cfgTokenTemplate(0x9,			TOKEN_TAB),
	cfgTokenTemplate(0xd,			TOKEN_EOL),
	cfgTokenTemplate(0xa,			TOKEN_EOL),
};

lwotomd3_cfgFile::lwotomd3_cfgFile(FILE *f):
	cfgFile(f),
	_tokenizer(ValidTokenList,(sizeof(ValidTokenList)/sizeof(cfgTokenTemplate)))
{;}

/*
 *	filelist
	{
		single <filename>
		range <fileroot> <start number> <count>
 *	}+
 */
cfgToken lwotomd3_cfgFile::ProcessOffWorkFileList()
{
	int last_framecount = framecount;
	framecount = 0;

	if (WorkFileList)
		WorkFileList = WorkFileList->InsertTail(new LWO_stub(0));
	else
		WorkFileList = new LWO_stub(0);

	cfgToken next(ParseOffToken(TOKEN_OPENBRACKET));
	// Peel off openbracket before starting
	for (;
		 next.Type() != TOKEN_CLOSEBRACKET;
		 next = ParseOffNextToken())
	{
		switch (next.Type())
		{
		case TOKEN_SINGLE:
			if (ProcessFileSingle(ParseOffNextStringToken()) == 0)
				return TOKENIZER().EndOfFile();
			break;

		case TOKEN_BASE:
			if (ProcessFileBase(ParseOffNextStringToken()) == 0)
				return TOKENIZER().EndOfFile();
			break;


		case TOKEN_RANGE:
			if (ProcessFileRange(ParseOffNextStringToken(),
									ParseOffNextStringToken(),
									ParseOffNextStringToken()) == 0 )
				return TOKENIZER().EndOfFile();
			break;

		case TOKEN_CLOSEBRACKET:
			break;

		case TOKEN_EOF:
			return TOKENIZER().EndOfFile();
			break;

		default:
			break;
		}
	}

	framecount = WorkFileList->ItemCount();

	if (last_framecount > 0)
	{
		if (framecount > 1 &&
			framecount != last_framecount)
		{
			fprintf(stderr,"ERROR - Unexpected frame counts - last = %d, this = %d\n",
				last_framecount, framecount);
			return TOKENIZER().EndOfFile();
		}
		framecount = last_framecount; 
	}

	return next;
}

/*	mesh
 *	{
		name <mesh name>
		images <image>+
		surfaces <surface>+
		txuv <UV map name>+
 *	}
 *	tag
 *	{
		name <tag name>
		surfaces <surface>+
 *	}+
 */
cfgToken lwotomd3_cfgFile::ProcessOffMeshList(cfgToken startToken)
{
	int srf_count = 0, uv_count = 0;

	fprintf(stdout,"\n");
	
	Material_Reference		*tmpMat;

	// Peel off openbracket
	FIND_TOKEN_OF_TYPE(TOKEN_OPENBRACKET)

	// find the Mesh's name token
	FIND_TOKEN_OF_TYPE(TOKEN_NAME)
	
	cfgToken meshname(ParseOffNextStringToken());
	if (meshname.Type() == TOKEN_EOF)
		return TOKENIZER().EndOfFile();
		
	if (startToken.Type() == TOKEN_TAG)
	{
		Tag_Reference *w = new Tag_Reference(meshname.Value());
		tmpMat = (WorkTag = WorkTag ? WorkTag->InsertHead(w) : w);
	}
	else
	{
		Mesh_Reference *w = new Mesh_Reference(meshname.Value());
		tmpMat = (WorkMesh = WorkMesh ? WorkMesh->InsertHead(w) : w);
	}

	cfgToken next(ParseOffNextToken());
	for (;
		 next.Type() != TOKEN_CLOSEBRACKET;
		 next = ParseOffNextToken())
	{
		switch (next.Type())
		{
		case TOKEN_IMAGES:		// images <image>+
			while ((next = ParseOffNextStringToken()).Type() != TOKEN_EOL)
			{
				if (next.Type() == TOKEN_EOF)
					break;
				fprintf(stdout,"Mesh '%s' using image %s\n",
					tmpMat->MeshName(),next.Value());
				tmpMat->AddImage(next.Value());
			}
			break;

		case TOKEN_SURFS:		// surfaces <surf>+
			while ((next = ParseOffNextStringToken()).Type() != TOKEN_EOL)
			{
				if (next.Type() == TOKEN_EOF)
					break;
				fprintf(stdout,"Mesh '%s' will filter surfaces with '%s'\n",
							tmpMat->MeshName(),next.Value());
				tmpMat->StoreMaterialAlias(next.Value());
				++srf_count;
			}
			break;

		case TOKEN_VMAP:		// txuv <vmaps>+
			fprintf(stdout,"Mesh '%s' will register UV maps\n",tmpMat->MeshName());
			tmpMat->FlagForMappingSkins();

			while ((next = ParseOffNextStringToken()).Type() != TOKEN_EOL)
			{
				if (next.Type() == TOKEN_EOF)
					break;
				fprintf(stdout,"Mesh '%s' will filter UVMaps with '%s'\n",
							tmpMat->MeshName(),next.Value());
				tmpMat->StoreSkinMapAlias(next.Value());
				++uv_count;
			}
			break;

		case TOKEN_NOUPDATE:
			tmpMat->unFlagForMappingSkins();
			break;

		case TOKEN_EOF:
			return TOKENIZER().EndOfFile();
			break;

		case TOKEN_CLOSEBRACKET:
			return next;
			break;

		default:
			break;
		}

		if (next.Type() == TOKEN_EOF)
			break;
	}

	if (srf_count == 0 && uv_count == 0)
	{	// We need something defined
		if (tmpMat->FlaggedForMappingSkins())
			tmpMat->StoreSkinMapAlias(tmpMat->MeshName());
		else
			tmpMat->StoreMaterialAlias(tmpMat->MeshName());
	}

	if (srf_count > 0 && uv_count > 0)
	{
		fprintf(stderr,"WARNING: both surfaces and UVmaps defined for mesh '%s'\n",
					tmpMat->MeshName());
	}

	return next;
}

/*
 * Parse off the MD3 definition
 *
  we're expecting:
  {
    filedir
	mesh { }
	tags { }
	filelist { }
  }

    Populate mesh and tag lists
    Populate stub lists and try to load one 'seed' file in
    Add connective tissues between them
    (possibly repeat)
 */
int lwotomd3_cfgFile::ProcessOffMD3()
{
	if (ParseOffToken(TOKEN_OPENBRACKET).Type() != TOKEN_OPENBRACKET)
		return 0;

	MD3_Name[0] = 0;
	Global_FileDir[0] = 0;

	for (cfgToken next(ParseOffNextStringToken());
		 next.Type() != TOKEN_CLOSEBRACKET;
		 next = ParseOffNextToken())
	{
		switch (next.Type())
		{
		case TOKEN_TAB:
		case TOKEN_EOL:
			continue;

		case TOKEN_NAME:
			strcpy(MD3_Name,ParseOffNextStringToken().Value());
			break;

		case TOKEN_MESH:
		case TOKEN_TAG:
			next = ProcessOffMeshList(next);
			break;

		case TOKEN_FILELIST:
			next = ProcessOffWorkFileList();
			LinkMeshesToWorkFileList();
			break;

		case TOKEN_FILEDIR:
			next = ParseOffNextStringToken();
			strcpy(Global_FileDir,next.Value());
			break;

		case TOKEN_CLOSEBRACKET:
			return 1;
			break;

		default:
			break;
		}

		if (next.Type() == TOKEN_EOF)
			return 0;
	}

	fprintf(stdout,"\n");

	return 1;
}

/* ==== Read in a cfg file ===
 *
 * The way this is going to work is as follows:
   Each surface may or may not map to a MD3 mesh - based on input file
   Each tag_ mesh is directly mapped to a tag - if the inputfile allows it

   Input file structure is like:

  md3
  {
	name <filename>
	filefir <root to input files>
	mesh
	{
		name <mesh name>
		images <image>+
		surfaces <surface>+
		txuv <UV map name>+
	}+
	tag
	{
		name <tag name>
		surfaces <surface>+
	}+
	filelist
	{
		single <filename>
		range <fileroot> <start number> <count>
	}+
  }
 *
 */
/* STATIC */
int lwotomd3_cfgFile::ProcessCfgBuffer()
{
	// first token is the md3 definition - look for it
	if (ParseOffToken(TOKEN_MD3).Type() != TOKEN_MD3)
	{
		fprintf(stderr,"ERROR - First token must define md3 block\n");
		return 0;
	}

	return (ProcessOffMD3());
}

/* ==============================
 *     END OF CLASS DEFINITIONS
 * ==============================
 */

lwotomd3_cfgFile	*INPUT_FILE = 0;
size_t cfgFile::FILEREAD_CHUNK = 1024;

void LinkMeshesToWorkFileList()
{	// We need to either populate the list of associated meshes on the fly, or we need
	// to fill it in ahead of time.  The issue is that we need to be sure that it is 
	// reproducible in case the user wants to update this mesh in the future.
	// Since we are potentially rooting through several lists of files, this could
	// become important.

	// We will assume that each surface has same number of polygons in it and those 
	// polygons are ordered in the same way.  We will also assume that the file lists
	// are provided in the same order each time ...

	// Oh hell, for now I'll just BUILD stuff.   Worry about changing later...
	LWO	*lwo = WorkFileList->BaseObject();

	if (!lwo)
	{
		fprintf(stdout,"Unable to find a valid file in the range of '%s' -> '%s'\n",
					WorkFileList->FileName(0),
					WorkFileList->FileName(WorkFileList->ItemCount() -1) );
		return;
	}

	// now try to connect the materials with the specified meshes.
	UINT32 i = 0;
	for (i = 0; i < lwo->SurfaceCount(); i ++)
	{
		char *n = new char[strlen(lwo->getSurface(i).name) +1];
		strcpy(n,lwo->getSurface(i).name);

		// Check for a match in the tags
		int					checkedmesh = 0;
		Material_Reference	*mr = WorkTag->Head();

		if (!mr)		// (or start with the meshes)
		{
			mr = WorkMesh->Head();
			checkedmesh = 1;
		}

		while (mr)
		{
			if(mr->FindSurfaceMatch(n))		// see if the name matches the aliases given before
			{
				Surf_Reference	*sr = mr->FindSurface(n);	// Is it a repeat surface?

				if (!sr)									// if not, it is a new surface
					mr->AddSurface(sr = new Surf_Reference(n));

				sr->AddLwoRef(new LWO_ref_bysurface(WorkFileList,lwo,i));
				mr->RecalculateCounts();

				fprintf(stdout," -MATCHED - Material '%s' is added to %s '%s'\n",
							n, mr->Label(), mr->MeshName());
				break;
			}

			if ((mr = mr->Next()) != 0)
				continue;

			// .. then check the Meshes
			if (!checkedmesh)
			{
				mr = WorkMesh->Head();
				checkedmesh = 1;
			}

		}

		if (!mr)	// Mesh has no match - it will be ignored
			fprintf(stdout,"IGNORED - Material '%s' does not match any defined meshes\n", n);
	}

	// now try to connect the UVmaps with the specified meshes.
	for (i = 0; i < lwo->UVMapCount(); i ++)
	{
		char *n = new char[strlen(lwo->getVMap(i).name) +1];
		strcpy(n,lwo->getVMap(i).name);

		// Check for a match in the tags
		int					checkedmesh = 0;
		Material_Reference	*mr = WorkMesh->Head();

		while (mr)
		{
			if(mr->FindSkinMapMatch(n))		// see if the name matches the aliases given before
			{
				Surf_Reference	*sr = mr->FindSurface(n);	// Is it a repeat surface?

				if (!sr)									// if not, it is a new surface
					mr->AddSurface(sr = new Surf_Reference(n));

				sr->AddLwoRef(new LWO_ref_bytxuv(WorkFileList,lwo,i));
				mr->RecalculateCounts();

				fprintf(stdout," -MATCHED - Material '%s' is added to %s '%s'\n",
							n, mr->Label(), mr->MeshName());
				break;
			}

			if ((mr = mr->Next()) != 0)
				continue;

			// .. then check the Meshes
			if (!checkedmesh)
			{
				mr = WorkMesh->Head();
				checkedmesh = 1;
			}

		}

		if (!mr)	// Mesh has no match - it will be ignored
			fprintf(stdout,"IGNORED - UVMap '%s' does not match any defined meshes\n", n);
	}
}

/*
 *
 * Returns:  Number of frames
 */
int ProcessCfgFile(char *cfgfile, CfgData *Data)
{
	FILE *f;

	if (cfgfile == (char *)NULL || cfgfile[0] == (char )NULL || 
		(f = fopen(cfgfile,"rt")) == (FILE *)NULL)
	{
		fprintf(stderr,"ERROR - can't open cfg file '%s'\n",
			cfgfile ? cfgfile : NULL);
		return 0;
	}

	// Load in cfg into local buffer
	INPUT_FILE = new lwotomd3_cfgFile(f);
	if (INPUT_FILE == 0 || INPUT_FILE->Filesize() <= 0)
	{
		fprintf(stderr,"ERROR - can't read cfg file '%s'\n",cfgfile);
		return 0;
	}

	WorkMesh = 0, WorkTag = 0, WorkFileList = 0;

	// Build mesh list and seed file list arrays
	if (!INPUT_FILE->ProcessCfgBuffer())
		return 0;

	// Set up the input 
	Data->Name(MD3_Name);
	Data->FileLists(WorkFileList->Head());
	Data->Meshes(WorkMesh->Head());
	Data->Tags(WorkTag->Head());

	return framecount;
}