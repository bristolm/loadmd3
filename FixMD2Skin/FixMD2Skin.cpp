/**************************************
 *
 *  FixMD2Skin.cpp
 *
 *  Loads in a MD2 and rights all the slashes
 *  in the texture's pathname
 *
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  MD2 file format property of id software
 *
 **************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "ParseMD2.h"

main (int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stdout,"Usage: %s <.md2 Filename>\n",argv[0]);;

		return -1;
	}

	size_t len = strlen(argv[1]);

	MD2		Mdl(argv[1]);

	if (!Mdl.isLoaded())
	{
		fprintf(stdout,"MD2 file '%s' seems invalid",argv[1]);
		return -1;
	}

	UINT32 i = 0, dirty = 0;

	// swap skin names from "models\items ... " to "models/items ..."
	for (i = 0; i < Mdl.SkinCount(); i++)
	{
		int j = 0, skindirty = 0;;
		fprintf(stdout,"\tFound Skin %2d: %s ...",i,Mdl.Skin(i).Name);
		char *p = Mdl.Skin(i).Name;


		for (;j < MAX_MD2SKINNAME && *p; j++, p++)
		{
			if (*p == '\\')
			{
				*p = '/';
				skindirty = 1;
			}
		}

		if (skindirty)
			fprintf(stdout,"\n\t\t Changed to - %s ... \n",Mdl.Skin(i).Name);
		else
			fprintf(stdout,"OK\n");

		dirty += skindirty;
	}

	if (dirty)
		Mdl.WritetoDisk();

	return 1;
}
	
