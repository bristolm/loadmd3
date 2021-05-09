# Microsoft Developer Studio Project File - Name="LayoutRecorder" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=LayoutRecorder - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LayoutRecorder.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LayoutRecorder.mak" CFG="LayoutRecorder - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LayoutRecorder - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "LayoutRecorder - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LayoutRecorder - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\include" /I "$(LW6_DIR)\SDK\include" /D "NDEBUG" /D D_X86_=1 /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fr /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 server.lib kernel32.lib /nologo /subsystem:windows /dll /pdb:none /machine:I386 /nodefaultlib:"libcd" /nodefaultlib:"libc" /out:"MRB_Xhub.p" /implib:"Debug/LayoutRecorder.lib" /libpath:"$(LW6_DIR)\SDK\lib" /libpath:"..\lib" -export:_mod_descrip
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy
PostBuild_Cmds=copy        MRB_Xhub.p          $(LW5_DIR)\Plugins\Common\       	copy        MRB_Xhub.p         $(LW6_DIR)\Programs\Plugins\Input-Output\       	copy        MRB_Xhub.p         X:\LW_Content\Demo\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "LayoutRecorder - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "$(LW6_DIR)\SDK\include" /D "_DEBUG" /D D_X86_=1 /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 server.lib libcmtd.lib kernel32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcd" /nodefaultlib:"libc" /out:"MRB_Xhub.p" /libpath:"..\lib" /libpath:"$(LW6_DIR)\SDK\lib" -export:_mod_descrip
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy
PostBuild_Cmds=copy        MRB_Xhub.p        $(LW5_DIR)\Plugins\Layout\       	copy        MRB_Xhub.p         $(LW6_DIR)\Programs\Plugins\Input-Output\       	copy        MRB_Xhub.p         X:\LW_Content\Demo\ 
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "LayoutRecorder - Win32 Release"
# Name "LayoutRecorder - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Displacement_Funcs.cpp
# End Source File
# Begin Source File

SOURCE=.\Displacement_XPanels.cpp
# End Source File
# Begin Source File

SOURCE=.\ExportObj.cpp
# End Source File
# Begin Source File

SOURCE=.\ExportPanels.cpp
# End Source File
# Begin Source File

SOURCE=.\Generic_Funcs.cpp
# End Source File
# Begin Source File

SOURCE=.\Global_Funcs.cpp
# End Source File
# Begin Source File

SOURCE=.\LayoutRecorderPlugin.c
# End Source File
# Begin Source File

SOURCE=.\ListenChannel.cpp
# End Source File
# Begin Source File

SOURCE=.\Master_Funcs.cpp
# End Source File
# Begin Source File

SOURCE=.\MotionHandler_Funcs.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectWrapper.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Displacement_XPanels.h
# End Source File
# Begin Source File

SOURCE=.\ExportObj.h
# End Source File
# Begin Source File

SOURCE=.\ExportPanels.h
# End Source File
# Begin Source File

SOURCE=.\LayoutRecorderPlugin.h
# End Source File
# Begin Source File

SOURCE=.\ListenChannel.h
# End Source File
# Begin Source File

SOURCE=..\include\LwChannel.h
# End Source File
# Begin Source File

SOURCE=..\include\lwmrbxprt.h
# End Source File
# Begin Source File

SOURCE=.\ObjectWrapper.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\README.txt
# End Source File
# Begin Source File

SOURCE=X:\LW_Content\Demo\README_hub.txt
# End Source File
# End Group
# Begin Group "Common Source"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\common\lw_base.c
# End Source File
# Begin Source File

SOURCE=..\common\lw_ctleditlistbox.cpp
# End Source File
# Begin Source File

SOURCE=..\common\lw_vbase.cpp
# End Source File
# End Group
# End Target
# End Project
