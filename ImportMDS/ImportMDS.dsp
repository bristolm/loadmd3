# Microsoft Developer Studio Project File - Name="ImportMDS" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ImportMDS - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ImportMDS.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ImportMDS.mak" CFG="ImportMDS - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ImportMDS - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ImportMDS - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ImportMDS - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IMPORTMDS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "$(LW6_DIR)\SDK\include" /D "NDEBUG" /D D_X86_=1 /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 server.lib libcmt.lib kernel32.lib /nologo /subsystem:windows /dll /pdb:none /machine:I386 /nodefaultlib:"libcd" /nodefaultlib:"libc" /nodefaultlib:"libcmtd" /out:"ImportMDS.p" /implib:"Debug/UTSkel.lib" /libpath:"$(LW6_DIR)\SDK\lib" /libpath:"..\lib" -export:_mod_descrip
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy ImportMDS.p  $(LW6_DIR)\Programs\Plugins\Input-Output\UTSkel.p	copy ImportMDS.p  X:\LW_Content\Demo\UTSkel.p
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ImportMDS - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IMPORTMDS_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "$(LW6_DIR)\SDK\include" /D "_DEBUG" /D D_X86_=1 /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 server.lib kernel32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcd" /nodefaultlib:"libcmt" /out:"ImportMDS.p" /libpath:"..\lib" /libpath:"$(LW6_DIR)\SDK\lib" -export:_mod_descrip
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy ImportMDS.p  $(LW6_DIR)\Programs\Plugins\Input-Output\MDS.p	copy ImportMDS.p  X:\LW_Content\Demo\MDS.p
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ImportMDS - Win32 Release"
# Name "ImportMDS - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\GUI.cpp
# End Source File
# Begin Source File

SOURCE=.\ImportMDS.c
# End Source File
# Begin Source File

SOURCE=.\ObjImp.cpp
# End Source File
# Begin Source File

SOURCE=..\objQuake\ParseMDS.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ImportMDS.h
# End Source File
# Begin Source File

SOURCE=..\include\ParseMDS.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Common Source"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\common\lw_base.c
# End Source File
# End Group
# End Target
# End Project
