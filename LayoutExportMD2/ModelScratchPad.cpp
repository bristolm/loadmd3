#include "ModelScratchPad.h"

ModelScratchPad& ModelScratchPad::operator=( const ModelScratchPad& rhs)
{	// Just copy some stuff over
	strcpy(QuakeDir,rhs.QuakeDir);
	strcpy(ModelDir,rhs.ModelDir);

	strcpy(ModelName,rhs.ModelName);
	SaveModel = rhs.SaveModel;
	SkinWidth = rhs.SkinWidth;
	SkinHeight = rhs.SkinHeight;

	strcpy(HeaderFile,rhs.HeaderFile);
	SaveHeader = rhs.SaveHeader;

	// Fill in model name based on this stuff
	sprintf(tmp,"%s.md2",ModelName);
//	Model.UpdateName(tmp);

	return *this;
}

void ModelScratchPad::dumpHeaderFile()
{
	FILE *fp = 0;

	if (HeaderFile[0] == 0)
	{
		return;
	}

	// Now the output file
	if ((fp = fopen(HeaderFile,"w")) == (FILE *)NULL)
		return;

	fprintf(fp,"/* Header file created automatically by the \n");
	fprintf(fp," * LightWave MD2 Export plugin - Mike Bristol */\n\n");
	int iFrame = 0;
	for (int i = 0; Loops && Loops[i]; i++)
	{
		// For each loop, do a _START, _END, and one for each frame
		fprintf(fp,"#define FRAME_%s_START \t%d\n",
						Loops[i]->Name,iFrame);

		for (int iLoop = 0; iLoop < Loops[i]->length; iLoop++, iFrame++)
		{
		fprintf(fp,"#define FRAME_%s%-6d \t%d\n",
						Loops[i]->Name,iLoop,iFrame);
		}
		
		fprintf(fp,"#define FRAME_%s_END   \t%d\n\n",
						Loops[i]->Name,iFrame -1);
	}

	fclose(fp);
	fp = 0;
}

// Lump values for Load/Save
#define ID_MD2D  LWID_( 'M','D','2','D' )
static LWBlockIdent idmain[] = {
	ID_MD2D, "MD2Data",
	0
};

#define ID_MD2G  LWID_( 'M','D','2','G' )
#define ID_MD2M  LWID_( 'M','D','2','M' )
#define ID_MD2H  LWID_( 'M','D','2','H' )
static LWBlockIdent idelement[] = {
	ID_MD2G, "General",
	ID_MD2M, "Model",
	ID_MD2H, "Header",
	0
};

void ModelScratchPad::SAVE(const LWSaveState *save)
{
	float f[1] = {0};
	f[0] = PLUGINVERSION ;
	short s[3] = {0};

	// Spit out version
	LWSAVE_FP(save,f,1);
	
	LWSAVE_BEGIN( save, &idmain[ 0 ], 0 );

	if (this)
	{
		// Save General data
		LWSAVE_BEGIN( save, &idelement[ 0 ], 1 );
		 LWSAVE_STR(save, QuakeDir);
		 LWSAVE_STR(save, ModelDir);
		LWSAVE_END( save );

		// Save Model data
		LWSAVE_BEGIN( save, &idelement[ 1 ], 1 );
		 LWSAVE_STR(save, ModelName);
		 LWSAVE_STR(save, "");
		 s[0] = SaveModel;
		 s[1] = SkinWidth;
		 s[2] = SkinHeight;
		 LWSAVE_I2(save,s,3);
		LWSAVE_END( save );

		// Save Header data
		LWSAVE_BEGIN( save, &idelement[ 2 ], 1 );
		 LWSAVE_STR(save, HeaderFile);
		 LWSAVE_STR(save, "");

		 s[0] = SaveHeader;
		 s[1] = s[2] = 0;
		 LWSAVE_I2(save,s,3);
		LWSAVE_END( save );
	}
	LWSAVE_END( save );
}

int ModelScratchPad::LOAD(const LWLoadState *load)
{
	int retval = 0;		// Return 0 if we really didn't do anything here ...

	float f[1];
	LWLOAD_FP(load,f,1);		// Snag version

	short s[3] = {0};

	LWID id = LWLOAD_FIND( load, idmain );
	while ( id = LWLOAD_FIND( load, idelement ))
	{
		switch ( id )
		{
		case ID_MD2G:		// General
			retval = 1;
			LWLOAD_STR(load,QuakeDir,256);
			LWLOAD_STR(load,ModelDir,256);
			break;

		case ID_MD2M:		// Model
			LWLOAD_STR(load,ModelName,256);
			LWLOAD_STR(load,tmp,256);
			LWLOAD_I2(load,s,3);
			SaveModel = s[0];
			SkinWidth = s[1];
			SkinHeight = s[2];
			break;

		case ID_MD2H:		// Header
			LWLOAD_STR(load,HeaderFile,1024);
			LWLOAD_STR(load,tmp,256);
			LWLOAD_I2(load,s,3);
			SaveHeader = s[0];
			//s[1];
			//s[2];
			break;
		}
		LWLOAD_END( load );
	}	

	LWLOAD_END( load );

	return retval;
}