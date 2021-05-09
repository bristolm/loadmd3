# Microsoft Developer Studio Generated NMAKE File, Based on CheckMD3.dsp
!IF "$(CFG)" == ""
CFG=CheckMD3 - Win32 Debug
!MESSAGE No configuration specified. Defaulting to CheckMD3 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "CheckMD3 - Win32 Release" && "$(CFG)" != "CheckMD3 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CheckMD3.mak" CFG="CheckMD3 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CheckMD3 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "CheckMD3 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "CheckMD3 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : ".\CheckMD3.exe" "$(OUTDIR)\CheckMD3.bsc"


CLEAN :
	-@erase "$(INTDIR)\CheckMD3.obj"
	-@erase "$(INTDIR)\CheckMD3.sbr"
	-@erase "$(INTDIR)\lw_vbase.obj"
	-@erase "$(INTDIR)\lw_vbase.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\CheckMD3.bsc"
	-@erase ".\CheckMD3.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\include" /I "$(LW6_DIR)\SDK\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\CheckMD3.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\CheckMD3.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CheckMD3.sbr" \
	"$(INTDIR)\lw_vbase.sbr"

"$(OUTDIR)\CheckMD3.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=objLW.lib objUnreal.lib objQuake.lib server.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\CheckMD3.pdb" /machine:I386 /nodefaultlib:"libc" /nodefaultlib:"libcmtd.lib" /out:"CheckMD3.exe" /libpath:"$(LW6_DIR)\SDK\lib" /libpath:"..\lib" 
LINK32_OBJS= \
	"$(INTDIR)\CheckMD3.obj" \
	"$(INTDIR)\lw_vbase.obj"

".\CheckMD3.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : ".\CheckMD3.exe" "$(OUTDIR)\CheckMD3.bsc"
   copy CheckMD3.exe  X:\LW_Content\Demo\FileInfo.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "CheckMD3 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : ".\CheckMD3.exe" "$(OUTDIR)\CheckMD3.bsc"


CLEAN :
	-@erase "$(INTDIR)\CheckMD3.obj"
	-@erase "$(INTDIR)\CheckMD3.sbr"
	-@erase "$(INTDIR)\lw_vbase.obj"
	-@erase "$(INTDIR)\lw_vbase.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\CheckMD3.bsc"
	-@erase "$(OUTDIR)\CheckMD3.pdb"
	-@erase ".\CheckMD3.exe"
	-@erase ".\CheckMD3.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "$(LW6_DIR)\SDK\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\CheckMD3.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\CheckMD3.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CheckMD3.sbr" \
	"$(INTDIR)\lw_vbase.sbr"

"$(OUTDIR)\CheckMD3.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=objLW.lib objUnreal.lib objQuake.lib server.lib kernel32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\CheckMD3.pdb" /debug /machine:I386 /nodefaultlib:"libcmt" /out:"CheckMD3.exe" /pdbtype:sept /libpath:"..\lib" /libpath:"$(LW6_DIR)\SDK\lib" 
LINK32_OBJS= \
	"$(INTDIR)\CheckMD3.obj" \
	"$(INTDIR)\lw_vbase.obj"

".\CheckMD3.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

$(DS_POSTBUILD_DEP) : ".\CheckMD3.exe" "$(OUTDIR)\CheckMD3.bsc"
   copy CheckMD3.exe  X:\LW_Content\Demo\FileInfo.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("CheckMD3.dep")
!INCLUDE "CheckMD3.dep"
!ELSE 
!MESSAGE Warning: cannot find "CheckMD3.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "CheckMD3 - Win32 Release" || "$(CFG)" == "CheckMD3 - Win32 Debug"
SOURCE=.\CheckMD3.cpp

"$(INTDIR)\CheckMD3.obj"	"$(INTDIR)\CheckMD3.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=..\common\lw_vbase.cpp

"$(INTDIR)\lw_vbase.obj"	"$(INTDIR)\lw_vbase.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

