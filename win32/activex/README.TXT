== ACTIVEX Control for VLC ==

The VLC ActiveX Control has been primary designed to work with Internet
Explorer. However it may also work with Visual Basic and/or .NET. Please
note, that this code does not rely upon Microsoft MFC/ATL code, hence
good compatibility is not guaranteed.

I. Compiling

The ActiveX Control should compile without any glitches as long as you
have the latest version of mingw gcc and headers.

In order to script the ActiveX Control on Internet Explorer, a type
library is required. This type library is usually generated from an IDL
file using Microsoft MIDL compiler. Therefore, for convenience I have
checked in the output of the MIDL compiler in the repository so that you
will only need the MIDL compiler if you change axvlc.idl. the generated
files are as follow:

axvlc_idl.c
axvlc_idl.h
axvlc.tlb

To use the MIDL compiler on cygwin, you will need to set some
environment variables before configuring vlc. If you have a copy of
'Microsoft Visual C++ 6.0' installed, the following settings are
correct:

export PATH=$PATH:"/cygdrive/c/Program Files/Microsoft Visual Studio/COMMON/MSDev98/Bin":"/cygdrive/c/Program Files/Microsoft Visual Studio/VC98/Bin"
export INCLUDE='C:\Program Files\Microsoft Visual Studio\VC98\Include'
export MIDL="midl"

If you are cross-compiling on Linux, you can use 'widl' which is part of
the WINE project (http://www.winehq.com). At leat wine-dev-0.9.57 works,
the comand line to compile IDL should looks like the following :

widl -I/usr/include/wine/windows/ \
	-h -H axvlc_idl.h -t -T axvlc.tlb -u -U axvlc_idl.c axvlc.idl

NOTE: widl breaks compatibility with Visual Basic. If that is important
to you then you must use midl.

II. Debugging

The ActiveX control is compiled with verbose output by default, but you
will need to launch Internet Explorer from a Cygwin shell to see the
output. Alternatively, the plugin will also use the VLC preferences, so
if you enable the file logging interface through the player and save the
preferences, the control will automatically log its verbose output into
the designated file.

Debugging the ActiveX control DLL with GNU GDB can be difficult.
Fortunately the ActiveX control can also be compiled as an executable
rather than a DLL. In ActiveX terms, this is called a local server. The
advantage of a local server is that it will never crash its client,
i.e. Internet Explorer, even if the local server crashes. The build
system does not currently allow to create an executable version of the
ActiveX control, you will need to manually define the BUILD_LOCALSERVER
pre-processor variable and modify the Makefile to exclude the '-shared'
option at the linking stage. Once this is done, launch axvlc.exe to have
a working Activex control. Please note, that executable version of the
ActiveX control will override any settings required for the DLL version,
which will no longer work until you (re)register it as shown in the
following section

III. Local Install

The VLC NSIS installer will install the ActiveX Control without
requiring any further manual intervention, but for people who like to
live on the edge, here are the steps you need to perform once you have
built the ActiveX Control.

The ActiveX control DLL file may be copied anywhere on the target
machine, but before you can use the control, you will need to register
it with Windows by using the REGSVR32 command, as per following example:

REGSVR32 C:\WINDOWS\AXVLC.DLL

If the control needs to use external VLC plugins (i.e other than the
built-in ones), make sure that the plugin path is set in the registry as
per following example:

[HKEY_LOCAL_MACHINE\Software\VideoLAN\VLC]
InstallDir="C:\Program Files\VideoLAN\VLC"

The InstallDir must be the parent directory of the 'plugins' directory.

WARNING: Both control and plugins must come from the same source build
tree. Otherwise, at best, the control will not play any content,
at worse it may crash Internet Explorer while attempting to load
incompatible plugins.

IV. Internet Install

The activex control may be installed from a remote through Internet
Installer if it is packaged up in a CAB file. The following link
explains how to achieve this

http://msdn.microsoft.com/workshop/components/activex/packaging.asp

For convenience, I have provided a sample axvlc.INF file, which assumes
that the VLC NSIS Installer has been packaged up a CAB file called
AXVLC.CAB.

The ActiveX Control DLL file can also be distributed by itself if it has
been compiled with built-in VLC plugins; check developer information for
more information on built-in plugins.

