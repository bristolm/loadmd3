#ifndef _LW_PLUG_BASE_H
#define _LW_PLUG_BASE_H

#include <lwpanel.h>
#include <lwhost.h>

#define SUPER_PTR(base,member,val)		\
		((base *)((int)val - (int)(&(((base *)0)->member))))

// Checkmark stuff ... (unsupported by NewTek)
#define LW_USECHECKMARK		"\03(i:MIMG_CHECKMARK)"
#define LW_NO_CHECKMARK		"   "

// Lightwave plug-in global values
extern LWPanControlDesc	desc;	 	// required by macros in lwpanel.h
extern LWValue			ival,ivecval,fval,fvecval,sval;		// required by macros in lwpanel.h

// Handlers for panel functions
void cmdGetInt(LWControl *ctl, int *dat);
void cmdGetStr(LWControl *ctl, char **dat);

// your basic error creation function
extern void SetFailureStuff(void *oi, char *msg, char *xtramsg);

// Path recorder
extern void FindLastPathfromName(const char *name);
extern void StoreLastPath(const char *name);
extern char *GetLastPath();

// Find product info
typedef struct _LightwaveProductInfo
{
	int major;
	int minor;
	int build;
} LightwaveProductInfo;

extern void FindProductInfo(GlobalFunc *global);
extern LightwaveProductInfo   ProductInfo;

// just redefine missing Grey from 5.6
#define LWP_GLINT   LWP_GRAY2

// Set draw function for a control
#define CON_SETDRAW(ctl,f)		(ival.intv.value=(int)&f,(*ctl->set)(ctl,CTL_USERDRAW,&ival) )
#define CON_SETUSERDATA(ctl,d)  (ival.intv.value=(int)d,(*ctl->set)(ctl,CTL_USERDATA,&ival)  )

// Dimension setings for controls
#define SIZE_CON(ctl,h,w)       (	(ival.intv.value=h,(*ctl->set)(ctl, CTL_H, &ival)),\
									(ival.intv.value=w,(*ctl->set)(ctl, CTL_W, &ival)) )       
// Change range for sliders
#define CON_SETMINR(ctl,n)  (ival.intv.value=n,(*ctl->set)(ctl, CTL_RANGEMIN, &ival))
#define CON_SETMAXR(ctl,n)  (ival.intv.value=n,(*ctl->set)(ctl, CTL_RANGEMAX, &ival))

//#define PAN_SETW(pfunc,pan,h)	( (ival.intv.value=h,(*pfunc->set)(pan, PAN_W, &(ival.intv.value))) )
#define PAN_SETMOUSEBTN(pfunc,pan,d)  ( ival.intv.value=(int)d,(*pfunc->set)(pan,PAN_MOUSEBUTTON,(void *)&(ival.intv.value)) )
#define PAN_GETUSERDATA(pfunc,pan) ((*pfunc->get)(pan,PAN_USERDATA,(void *)&(ival.intv.value)),ival.intv.value)

// Area/Slider pair functions

// Command func stuff
typedef int ( *LWCommandFunc )( const char *cmd );
#define LWCOMMANDINTERFACE_GLOBAL "LW Command Interface"

#endif //#ifndef _LW_PLUG_BASE_H
