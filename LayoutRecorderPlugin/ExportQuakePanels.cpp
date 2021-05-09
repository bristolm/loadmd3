/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lwrender.h"

/* Mystuff */
#include "sys_base.h"
extern "C"
{
#include "LWPlugBase.h"
#include "mdv_mdformat.h"
#include "ExportQuakeMD.h"
#include "LWPanExt.h"
}

#include "ExportQuakePanels.h"

static int				PanelBaseHeight;
static LWPanelID		panID;

static const char		*BlankBuf[4];
static char				tmp[1024];

static int				AlreadySetupPanels = 0;

// .cfg file handling objects
#define MAX_CFGFILESLOADED		5
static const char	*LoadedCfgFiles[MAX_CFGFILESLOADED];	
LWControl			*cCfgFileLoad, *cCfgList, *cCfgFileSave, *cCfgFileClear;

// mesh output definitions
LWControl			*cMeshName;

#define CANVAS_LWLEFT			pan_x + CANVAS_WIDTH - 150
#define CANVAS_MD3LEFT			pan_x + CANVAS_WIDTH - 70

static const int CTLTS_FRAMELENGTH	= 0;
static const int CTLTS_LW_FRAMESTART	= 1;
static const int CTLTS_MD3_FRAMESTART	= 2;

static	LWControl		*cTabs;
static const char	*tabchoice[5];
static int		EntryCount = 0;

static	LWControl		*cFrames,*cFrameSlider;
static	LWControl		*cFrameBtnAdd,*cFrameBtnDel;
static	LWControl		*cFrameStrName, *cFrameTxt[3], *cFrameStr[3];

static	CanvasEntry		*ActiveEntryList = (CanvasEntry *)NULL;
static	CanvasEntry		*ActiveCanvasItem = (CanvasEntry *)NULL;


/*
 * CfgFile class functions
 */
#define TOKEN_STRING	" \t\n\r"
#define TERMINATE_TOKEN(t)								\
		p = t;										\
		while (*p != ' ' && *p != '/' && *p !=13)		\
				++p;									\
		*p = 0;											\


CfgFile  ::	CfgFile(char *filename, FILE *openfile) :
		list(0), next(0)
{
	char	*p = (char *)0, *token = (char *)0;
	char	*tokens[5];
	int		NextInputToken = -1;

	strcpy(name,filename);

	fp = openfile;

	while(!feof(fp ))
	{
		p = fgets(tmp,1024,fp);
		if (ferror(fp))
			break;

		token = tmp;

		while(token = strtok(token, TOKEN_STRING))
		{	// Find the end of the token
			if (token[0] == 13)							// get next line
				break;
			else if (token[0] == '/')					// comment token
			{
				if (NextInputToken < 0)					// fully commented line
					break;
			}
			else if (NextInputToken >= 0)
			{
				tokens[++NextInputToken] = token;
				if (NextInputToken == 4)	// done with this one - add in entry
				{										
/*					(new CanvasEntry(tokens[4],
									 atoi(tokens[1]),
									 atoi(tokens[0]), atoi(tokens[0]),
									 atoi(tokens[2]), atoi(tokens[3])))->Add(&list);
*/					NextInputToken = -1;
					++EntryCount;
					break;					// and get a new line
				}
			}
			else if (strcmp("sex",token) == 0)				// sex m
			{
				strcpy(sex,token + strlen(token) +1);
				break;
			}
			else if(strcmp("headoffset",token) == 0)	// headoffset 0 0 0
			{
				strcpy(headoffset,token + strlen(token) +1);
				break;
			}
			else if(strcmp("footsteps",token) == 0)		// footsteps normal
			{
				strcpy(footsteps,token + strlen(token) +1);
				break;
			}
			else
			{											// line with real info in it
				tokens[NextInputToken = 0] = token;
			}

			token += (strlen(token) +1);
		}
	}

	ActiveEntryList = list;
	CON_SETMAXR(cFrameSlider,EntryCount);
}

/*
 * CanvasEntry class functions
 */

CanvasEntry :: CanvasEntry(char *n, int length, int lw_start, int q_start, 
						   int loop, int fps):
	FPS(fps),
	Loop(loop),
	flags(0),
	LW(lw_start,length),
	MD3(q_start,length)
{
	strcpy(Name(),n);
}

void CanvasEntry :: Remove(CanvasEntry **list)
{
	CanvasEntry		*e = *list;

	if (*list == this)
		*list = Next();
	else
	{
		while (e && e->Next() != this)
			e = e->Next();

		if (e)
			e->next = Next();
	}
}

void CanvasEntry :: Add(CanvasEntry **list)
{
	CanvasEntry		*e = *list;

	if (e && e->LW.StartFrame() < LW.StartFrame())
	{
		while(e->next && e->next->LW.StartFrame() < LW.StartFrame())
			e = e->Next();
	}

	if (!e || LW.StartFrame() < (*list)->LW.StartFrame())
	{
		next = *list;
		*list = this;
	}
	else
	{
		if (LW.StartFrame() < e->LW.StartFrame())
		{
			next = e;
			e->next = (CanvasEntry *)NULL;
		}
		else
		{
			next = e->Next();
			e->next = this;
		}
	}

	ActiveCanvasItem = this;
}

void CanvasEntry :: Reseed(CanvasEntry **list)
{
	Remove(list);
	Add(list);
}

void CanvasEntry :: ShowText()
{
	SET_STR(cFrameStrName,Name(),strlen(Name()));
	SET_STR(cFrameStr[CTLTS_FRAMELENGTH],LW.countText,5);
	SET_STR(cFrameStr[CTLTS_LW_FRAMESTART],LW.startText,5);
	SET_STR(cFrameStr[CTLTS_MD3_FRAMESTART],MD3.startText,5);
}

void CanvasEntry :: ShowEmptyText()
{
	SET_STR(cFrameStrName,"",30);
	SET_STR(cFrameStr[CTLTS_FRAMELENGTH],"",5);
	SET_STR(cFrameStr[CTLTS_LW_FRAMESTART],"",5);
	SET_STR(cFrameStr[CTLTS_MD3_FRAMESTART],"",5);
}

/*
 * EVENT handler functions
 */

// CFG based events
static void eventCfgFileLoad(LWControl *ctl, void *dat)
{
	FILE	*fp;
	GET_STR(ctl, tmp, 1024);

	if (tmp == 0 || tmp[0] == 0)
		return;

	if ((fp = fopen(tmp,"rb")) == (FILE *)NULL)
	{
		fprintf(stderr,"Can't open '%s'\n",tmp);
		return;
	}

	new CfgFile(tmp,fp);
}

// List Events
static void eventFrameBtnAdd(LWControl *ctl, void *dat)
{
	char	Name[30];
	GET_STR(cFrameStrName,Name,30);

	CanvasEntry *c = ActiveEntryList;
	while (c)
	{
		if (strcmp(Name,c->Name()) == 0)
			return;
		c = c->Next();
	}

	char	Len[5];
	GET_STR(cFrameStr[CTLTS_FRAMELENGTH],Len,5);

	char	LW_Start[5];
	GET_STR(cFrameStr[CTLTS_LW_FRAMESTART],LW_Start,5);

	char	MD3_Start[5];
	GET_STR(cFrameStr[CTLTS_MD3_FRAMESTART],MD3_Start,5);

	if (Name[0] && Len[0] && LW_Start[0] && MD3_Start[0])
	{
		(new CanvasEntry(Name,
						 atoi(Len),
						 atoi(LW_Start),	atoi(MD3_Start),
						 atoi(Len),			20))->Add(&ActiveEntryList);
	}

	CON_SETMAXR(cFrameSlider,++EntryCount);
	REDRAW_CON(cFrames);
}

static void eventFrameStrName(LWControl *ctl, void *dat)
{
	if (ActiveCanvasItem == (CanvasEntry *)NULL)
		return;

	GET_STR(ctl,tmp,64);

	CanvasEntry *c = ActiveEntryList;
	while (c)
	{
		if (strcmp(tmp,c->Name()) == 0)
			return;
		c = c->Next();
	}

	strcpy(ActiveCanvasItem->Name(),tmp);
	
	REDRAW_CON(cFrames);
}

static void eventFrameStr(LWControl *ctl, void *dat)
{
	if (ActiveCanvasItem == (CanvasEntry *)NULL)
		return;

	GET_STR(ctl,tmp,10);

	switch(*(int *)dat)
	{
	case CTLTS_FRAMELENGTH:
		ActiveCanvasItem->MD3.NewLength(atoi(tmp));
		ActiveCanvasItem->LW.NewLength(atoi(tmp));
		break;

	case CTLTS_LW_FRAMESTART:
		ActiveCanvasItem->LW.NewStart(atoi(tmp));
		break;

	case CTLTS_MD3_FRAMESTART:
		ActiveCanvasItem->MD3.NewStart(atoi(tmp));
		break;
	}

	ActiveCanvasItem->Reseed(&ActiveEntryList);

	REDRAW_CON(cFrames);
}

static void eventFrameBtnDel(LWControl *ctl, void *dat)
{
	if (ActiveCanvasItem == (CanvasEntry *)NULL)
		return;

	delete ActiveCanvasItem;
	ActiveCanvasItem = 0;

	CON_SETMAXR(cFrameSlider,--EntryCount);
	REDRAW_CON(cFrames);
}

static void eventFrameSlider(LWControl *ctl, void *dat)
{
	REDRAW_CON(cFrames);
}

static void eventCanvas(LWControl *ctl, void *dat)
{
	int my = CON_MOUSEY(ctl), mhot = CON_HOTY(ctl);
	int entryslot;
	
	GET_INT(cFrameSlider,entryslot);
	entryslot += ((my - mhot) / CANVASENTRY_HEIGHT);

	CanvasEntry		*e = ActiveEntryList;

	for (;e && entryslot > 0; e = e->Next(), entryslot--)
		;

	if (ActiveCanvasItem = e)
	{
		e->ToggleFlag(CANVAS_FLAG_SELECTED);
		e->ShowText();
	}
	else
		CanvasEntry :: ShowEmptyText();

	REDRAW_CON(ctl);
}

static void redrawCanvas(LWControl *c, void *dat, DrMode mode) 
{
	ERASE_CON(c);

	CanvasEntry		*e;
	static	char	tmp[16];
	int i = 0, j = 0;
	int pan_x = CON_HOTX(c), pan_y = CON_HOTY(c);

	{// Fix the 'look' of the tabbed areas
		int x1 = 0;
		int x2 = PAN_GETW(panelFuncs, panID) + x1;
		int y = CON_HOTY(cTabs);
		y += CON_HOTH(cTabs);

		drawFuncs->drawLine(panID,COLOR_BLACK,x1,y,x2,y);
		++y;
		drawFuncs->drawLine(panID,COLOR_WHITE,x1,y,x2,y);

		// Draw Titles
		drawFuncs->drawText(panID,"Sequence", COLOR_BLACK, pan_x + 5, pan_y - 15);
		drawFuncs->drawText(panID,"LW frames  >", COLOR_BLACK, CANVAS_LWLEFT, pan_y -15);
		drawFuncs->drawText(panID,"MD3 frames", COLOR_BLACK, CANVAS_MD3LEFT, pan_y -15);
	}


	// Draw list entries
	GET_INT(cFrameSlider,j);
	for (e = ActiveEntryList; e; e = e->Next(), j--)
	{
		if (j > 0)
			continue;

		if (j == -CANVAS_ITEMSHOWN)
			break;

		int color = LWP_GRAY1;
		int conflictMD3 = 0, conflitLW = 0;
		int y = pan_y + (CANVASENTRY_HEIGHT * i);

		if (ActiveCanvasItem == e)
			color = COLOR_LT_YELLOW;		// Selected one

		else if (ActiveCanvasItem)
		{	// See if the Active one (ideally the just added one) has a conflicting range
			conflictMD3 = ActiveCanvasItem->MD3.isRangeInConflict(&(e->MD3));
			conflitLW = ActiveCanvasItem->LW.isRangeInConflict(&(e->LW)) ;
		}


		if (e->Flags() & CANVAS_FLAG_SELECTED)
			drawFuncs->drawRGBBox(panID,	125,	125,	150,
									pan_x,			y,
									CANVAS_WIDTH,	CANVASENTRY_HEIGHT);

		drawFuncs->drawText(panID,e->Name(),color,
						pan_x + 5,				y);

		// LW values
		drawFuncs->drawText(panID,e->LW.startText,conflitLW ? COLOR_WHITE : color,
						CANVAS_LWLEFT,			y);

		drawFuncs->drawText(panID,e->LW.endText,conflitLW ? COLOR_WHITE : color,
						CANVAS_LWLEFT + 30,		y);

		// MD3 values
		drawFuncs->drawText(panID,e->MD3.startText,conflictMD3 ? COLOR_WHITE : color,
						CANVAS_MD3LEFT,			y);

		drawFuncs->drawText(panID,e->MD3.endText,conflictMD3 ? COLOR_WHITE : color,
						CANVAS_MD3LEFT + 30,	y);

		i++;
	}
}

void *SetupPanels()
{
	if (AlreadySetupPanels == 0)
	{
		AlreadySetupPanels = 1;

		new CanvasEntry("Zero",				10, 101, 0,	10,	20);
		new CanvasEntry("One",				20, 1, 10,	10,	20);
		new CanvasEntry("Two",				5, 51, 20,	10,	20);
		new CanvasEntry("TORSO_GESTURE",	15,56, 25,	10,	20);
		new CanvasEntry("Four",				10,21, 40,	10,	20);

		if( panID=PAN_CREATE(panelFuncs,"Quake MD3 file Export - v" PROG_VERSION " - mbristol@tp.net") )
		{
//			PAN_SETDRAW(panelFuncs, panID, redrawPanel);
			BlankBuf[0] = "";
			BlankBuf[1] = 0;

			PanelBaseHeight = PAN_GETH(panelFuncs,panID);

			// Setup the .cfg loading dialogs
			LoadedCfgFiles[0] = "(none)";
			LoadedCfgFiles[1] = 0;

			cCfgFileLoad =	FILEBUTTON_CTL(panelFuncs, panID, ".cfg file: ",PREFERRED_BUTTON_WIDTH);
			CON_SETEVENT(cCfgFileLoad,eventCfgFileLoad,ActiveCanvasItem);

			cCfgList =		WPOPUP_CTL(panelFuncs, panID, "", LoadedCfgFiles, CANVAS_WIDTH);
			cCfgFileSave =	WBUTTON_CTL(panelFuncs, panID, "Save",PREFERRED_BUTTON_WIDTH);
			cCfgFileClear =	WBUTTON_CTL(panelFuncs, panID, "Clear",PREFERRED_BUTTON_WIDTH);

			// Setup the meshname field (just this for now)
			cMeshName =	STR_CTL(panelFuncs, panID, "Mesh Name: ", 25);

			// Setup the tabs
			tabchoice[0] = "Frames";
			tabchoice[1] = "Meshes";
			tabchoice[2] = 0;

			cTabs = TABCHOICE_CTL(panelFuncs, panID,"",tabchoice);

			// Display the filename we're working with
			cFrames = DRAGAREA_CTL(panelFuncs,panID,"",CANVAS_WIDTH,CANVAS_HEIGHT);
			CON_SETDRAW(cFrames,redrawCanvas);
			CON_SETEVENT(cFrames,eventCanvas,ActiveCanvasItem);

			// Setup the vertical slider
			cFrameSlider = VSLIDER_CTL(panelFuncs,panID,"",CANVAS_HEIGHT,0,0);
			CON_SETEVENT(cFrameSlider,eventFrameSlider,ActiveCanvasItem);

//			PAN_SETDRAW(panelFuncs,panID,redrawCanvas);
//			c1 = LISTBOX_CTL(panelFuncs,panID,"Title",300,3,namefunc,numfunc);

			// Setup buttons
			cFrameBtnAdd = WBUTTON_CTL(panelFuncs,panID,"Add", PREFERRED_BUTTON_WIDTH +10);
			CON_SETEVENT(cFrameBtnAdd,eventFrameBtnAdd,&ActiveCanvasItem);

			cFrameBtnDel = WBUTTON_CTL(panelFuncs,panID,"Delete", PREFERRED_BUTTON_WIDTH +10);
			CON_SETEVENT(cFrameBtnDel,eventFrameBtnDel,&ActiveCanvasItem);

			// Setup test and string fields
			cFrameStrName = STR_CTL(panelFuncs,panID,"",20);
			CON_SETEVENT(cFrameStrName,eventFrameStrName,&ActiveCanvasItem);

			cFrameTxt[CTLTS_FRAMELENGTH] = TEXT_CTL(panelFuncs,panID,"Length:",BlankBuf);
			cFrameStr[CTLTS_FRAMELENGTH] = STR_CTL(panelFuncs,panID,"",6);
			CON_SETEVENT(cFrameStr[CTLTS_FRAMELENGTH],eventFrameStr,&CTLTS_FRAMELENGTH);

			cFrameTxt[CTLTS_LW_FRAMESTART] = TEXT_CTL(panelFuncs,panID,"LW Start:",BlankBuf);
			cFrameStr[CTLTS_LW_FRAMESTART] = STR_CTL(panelFuncs,panID,"",6);
			CON_SETEVENT(cFrameStr[CTLTS_LW_FRAMESTART],eventFrameStr,&CTLTS_LW_FRAMESTART);

			cFrameTxt[CTLTS_MD3_FRAMESTART] = TEXT_CTL(panelFuncs,panID,"MD3 Start:",BlankBuf);
			cFrameStr[CTLTS_MD3_FRAMESTART] = STR_CTL(panelFuncs,panID,"",6);
			CON_SETEVENT(cFrameStr[CTLTS_MD3_FRAMESTART],eventFrameStr,&CTLTS_MD3_FRAMESTART);
		}
		else
		{
			PAN_KILL(panelFuncs,panID);
//			*err = AFUNC_BADGLOBAL;
			return (void *)NULL;
		}

		// Rearrange things
		{
			int i = 0, j =0, k = 0, x = 0, y = 0;

			// Line up the .cfg stuff horizontally
			x = CON_HOTW(cCfgFileLoad);
			y = CON_Y(cCfgFileLoad);

			MOVE_CON(cCfgList,		x,					y);
			x += CON_HOTW(cCfgList);
			x += 25;

			// squeeze the buttons in
			MOVE_CON(cCfgFileSave,	x,					y);
			k = (CON_W(cFrameStrName));
			k -= (CON_W(cCfgFileSave));
			k -= (CON_W(cCfgFileClear));

			x += (CON_W(cCfgFileSave));
			x += k;
			MOVE_CON(cCfgFileClear,	x,					y);

			y += CON_HOTH(cCfgFileLoad);

			// Move the mesh field
			MOVE_CON(cMeshName,		0,					y += CANVASENTRY_HEIGHT);
			y += CON_HOTH(cMeshName);

			// Reposition the tabs
			MOVE_CON(cTabs,			0,					y += CANVASENTRY_HEIGHT);

			// Reposition the frames down a little
			y += (CANVASENTRY_HEIGHT * 2);
			MOVE_CON(cFrames,		0,					y += 5);

			// Move the slider beside the frames
			x = CON_HOTW(cFrames);
			y = CON_Y(cFrames);
			MOVE_CON(cFrameSlider,	x,					y);

			// Move the text fields around
			x += 35;

			MOVE_CON(cFrameStrName,	x - 14,				y);
			x += 20;
			y += 20;

			x = (CON_W(cFrameStrName));
			x += (CON_X(cFrameStrName));
			x -= (60 + CON_W(cFrameStr[0]));

			for (k = 0; k < 3; k++, y += 20)
			{
				MOVE_CON(cFrameTxt[k],	x,				y);
				MOVE_CON(cFrameStr[k],	x + 60,			y);
			}

			// Move the Buttons
			y += 10;
			x = CON_HOTX(cFrameStrName) - 10;
			MOVE_CON(cFrameBtnAdd,	x,					y);

			k = (CON_W(cFrameStrName));
			k -= (CON_W(cFrameBtnAdd));
			k -= (CON_W(cFrameBtnDel));

			x += (CON_W(cFrameBtnAdd));
			x += k;
			MOVE_CON(cFrameBtnDel,	x,					y);
	
			// Fix panel size
			y -= CON_HOTH(cFrameBtnDel);
			PAN_SETH(panelFuncs,panID,	y += PanelBaseHeight);
			PAN_SETW(panelFuncs,panID,	x + 60);
		}
	}
	if (ActiveCanvasItem)
		ActiveCanvasItem->ShowText();
	
	panelFuncs->open(panID,PANF_CANCEL|PANF_BLOCKING);

	return (void *)ActiveEntryList;
}