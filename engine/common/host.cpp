/*
host.c - dedicated and normal host
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "build.h"
#ifdef XASH_SDL
#include <SDL.h>
#endif // XASH_SDL
#include <stdarg.h>  // va_args
#include <errno.h> // errno
#include <string.h> // strerror
#if !XASH_WIN32
#include <unistd.h> // fork
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#if XASH_EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif
#include <errno.h>
#include "common.h"
#include "base_cmd.h"
#include "client.h"
#include "server.h"
#include "netchan.h"
#include "protocol.h"
#include "mod_local.h"
#include "xash3d_mathlib.h"
#include "input.h"
#include "enginefeatures.h"
#include "render_api.h"	// decallist_t

using namespace engine;

pfnChangeGame	pChangeGame = NULL;
host_parm_t		host;	// host parms
sysinfo_t		SI;

#ifdef XASH_ENGINE_TESTS
struct tests_stats_s tests_stats;
#endif

CVAR_DEFINE( host_developer, "developer", "0", FCVAR_FILTERABLE, "engine is in development-mode" );
CVAR_DEFINE_AUTO( sys_timescale, "1.0", FCVAR_CHEAT | FCVAR_FILTERABLE, "scale frame time" );
CVAR_DEFINE_AUTO( sys_ticrate, "100", 0, "framerate in dedicated mode" );

static CVAR_DEFINE_AUTO( host_serverstate, "0", FCVAR_READ_ONLY, "displays current server state" );
static CVAR_DEFINE_AUTO( host_gameloaded, "0", FCVAR_READ_ONLY, "inidcates a loaded game.dll" );
static CVAR_DEFINE_AUTO( host_clientloaded, "0", FCVAR_READ_ONLY, "inidcates a loaded client.dll" );
CVAR_DEFINE_AUTO( host_limitlocal, "0", 0, "apply cl_cmdrate and rate to loopback connection" );
CVAR_DEFINE( host_maxfps, "fps_max", "72", FCVAR_ARCHIVE|FCVAR_FILTERABLE, "host fps upper limit" );
static CVAR_DEFINE_AUTO( host_framerate, "0", FCVAR_FILTERABLE, "locks frame timing to this value in seconds" );
static CVAR_DEFINE( host_sleeptime, "sleeptime", "1", FCVAR_ARCHIVE|FCVAR_FILTERABLE, "milliseconds to sleep for each frame. higher values reduce fps accuracy" );
static CVAR_DEFINE_AUTO( host_sleeptime_debug, "0", 0, "print sleeps between frames" );
CVAR_DEFINE( con_gamemaps, "con_mapfilter", "1", FCVAR_ARCHIVE, "when true show only maps in game folder" );

static void Sys_PrintUsage( void )
{
	/*string version_str;
	const char *usage_str;

	Q_snprintf( version_str, sizeof( version_str ),
		XASH_ENGINE_NAME " %i/" XASH_VERSION " (%s-%s build %i)", PROTOCOL_VERSION, Q_buildos(), Q_buildarch(), Q_buildnum( ));

#if XASH_WIN32
#define XASH_EXE "(xash).exe"
#else
#define XASH_EXE "(xash)"
#endif

#define O( x, y ) "  "x"  "y"\n"

	usage_str = S_USAGE XASH_EXE " [options] [+command] [+command2 arg] ...\n"

"\nCommon options:\n"
	O("-dev [level]     ", "set log verbosity 0-2")
	O("-log             ", "write log to \"engine.log\"")
	O("-nowriteconfig   ", "disable config save")
	O("-noch            ", "disable crashhandler")
#if XASH_WIN32 // !!!!
	O("-minidumps       ", "enable writing minidumps when game is crashed")
#endif
	O("-rodir <path>    ", "set read-only base directory")
	O("-bugcomp         ", "enable precise bug compatibility. Will break games that don't require it")
	O("                 ", "Refer to engine documentation for more info")
	O("-disablehelp     ", "disable this message")
#if !XASH_DEDICATED
	O("-dedicated       ", "run engine in dedicated mode")
#endif

"\nNetworking options:\n"
	O("-noip            ", "disable IPv4")
	O("-ip <ip>         ", "set IPv4 address")
	O("-port <port>     ", "set IPv4 port")
	O("-noip6           ", "disable IPv6")
	O("-ip6 <ip>        ", "set IPv6 address")
	O("-port6 <port>    ", "set IPv6 port")
	O("-clockwindow <cw>", "adjust clockwindow used to ignore client commands to prevent speed hacks")

"\nGame options:\n"
	O("-dll <path>      ", "override server DLL path")
#if !XASH_DEDICATED
	O("-clientlib <path>", "override client DLL path")
	O("-console         ", "run engine with console enabled")
	O("-toconsole       ", "run engine witn console open")
	O("-oldfont         ", "enable unused Quake font in Half-Life")
	O("-width <n>       ", "set window width")
	O("-height <n>      ", "set window height")
	O("-borderless      ", "run engine in fullscreen borderless mode")
	O("-fullscreen      ", "run engine in fullscreen mode")
	O("-windowed        ", "run engine in windowed mode")
	O("-ref <name>      ", "use selected renderer dll")
	O("-gldebug         ", "enable OpenGL debug log")
#if XASH_WIN32
	O("-noavi           ", "disable AVI support")
	O("-nointro         ", "disable intro video")
#endif
	O("-noenginejoy     ", "disable engine builtin joystick support")
	O("-noenginemouse   ", "disable engine builtin mouse support")
	O("-nosound         ", "disable sound output")
	O("-timedemo        ", "run timedemo and exit")
#endif

"\nPlatform-specific options:\n"
#if !XASH_MOBILE_PLATFORM
	O("-daemonize       ", "run engine as a daemon")
#endif
#if XASH_SDL == 2
	O("-sdl_joy_old_api ","use SDL legacy joystick API")
	O("-sdl_renderer <n>","use alternative SDL_Renderer for software")
#endif // XASH_SDL
#if XASH_ANDROID && !XASH_SDL
	O("-nativeegl       ","use native egl implementation. Use if screen does not update or black")
#endif // XASH_ANDROID
#if XASH_DOS
	O("-novesa          ","disable vesa")
#endif // XASH_DOS
#if XASH_VIDEO == VIDEO_FBDEV
	O("-fbdev <path>    ","open selected framebuffer")
	O("-ttygfx          ","set graphics mode in tty")
	O("-doublebuffer    ","enable doublebuffering")
#endif // XASH_VIDEO == VIDEO_FBDEV
#if XASH_SOUND == SOUND_ALSA
	O("-alsadev <dev>   ","open selected ALSA device")
#endif // XASH_SOUND == SOUND_ALSA
	;
#undef O

	// HACKHACK: pretty output in dedicated
#if XASH_MESSAGEBOX != MSGBOX_STDERR
	Platform_MessageBox( version_str, usage_str, false );
#else
	fprintf( stderr, "%s\n%s", version_str, usage_str );
#endif

	Sys_Quit();*/
}

int Host_CompareFileTime( int ft1, int ft2 )
{
	if( ft1 < ft2 )
	{
		return -1;
	}
	else if( ft1 > ft2 )
	{
		return 1;
	}
	return 0;
}

void Host_ShutdownServer( void )
{
	SV_Shutdown( "Server was killed\n" );
}

/*
================
Host_PrintEngineFeatures
================
*/
static void Host_PrintEngineFeatures( int features )
{
	struct
	{
		uint32_t mask;
		const char *msg;
	} features_str[] =
	{
	{ ENGINE_WRITE_LARGE_COORD, "Big World Support" },
	{ ENGINE_QUAKE_COMPATIBLE, "Quake Compatibility" },
	{ ENGINE_LOAD_DELUXEDATA, "Deluxemap Support" },
	{ ENGINE_PHYSICS_PUSHER_EXT, "Improved MOVETYPE_PUSH" },
	{ ENGINE_LARGE_LIGHTMAPS, "Large Lightmaps" },
	{ ENGINE_COMPENSATE_QUAKE_BUG, "Stupid Quake Bug Compensation" },
	{ ENGINE_IMPROVED_LINETRACE, "Improved Trace Line" },
	{ ENGINE_COMPUTE_STUDIO_LERP, "Studio MOVETYPE_STEP Lerping" },
	{ ENGINE_LINEAR_GAMMA_SPACE, "Linear Gamma Space" },
	{ ENGINE_STEP_POSHISTORY_LERP, "MOVETYPE_STEP Position History Based Lerping" },
	};
	int i;

	for( i = 0; i < ARRAYSIZE( features_str ); i++ )
	{
		if( FBitSet( features, features_str[i].mask ))
			Con_Reportf( "^3EXT:^7 %s is enabled\n", features_str[i].msg );
	}
}

/*
==============
Host_ValidateEngineFeatures

validate features bits and set host.features
==============
*/
void Host_ValidateEngineFeatures( uint32_t features )
{
	uint32_t mask = ENGINE_FEATURES_MASK;

#if !HOST_DEDICATED
	if( !Host_IsDedicated( ) && cls.legacymode )
		mask = ENGINE_LEGACY_FEATURES_MASK;
#endif

	// don't allow unsupported bits
	features &= mask;

	// force bits for some games
	if( !Q_stricmp( GI->gamefolder, "cstrike" ) || !Q_stricmp( GI->gamefolder, "czero" ))
		SetBits( features, ENGINE_STEP_POSHISTORY_LERP );

	// print requested first
	Host_PrintEngineFeatures( features );

	// now warn about incompatible bits
	if( FBitSet( features, ENGINE_STEP_POSHISTORY_LERP|ENGINE_COMPUTE_STUDIO_LERP ) == ( ENGINE_STEP_POSHISTORY_LERP|ENGINE_COMPUTE_STUDIO_LERP ))
		Con_Printf( S_WARN "%s: incompatible ENGINE_STEP_POSHISTORY_LERP and ENGINE_COMPUTE_STUDIO_LERP are enabled!\n", __func__ );

	// finally set global variable
	host.features = features;
}

/*
==============
Host_IsQuakeCompatible

==============
*/
qboolean Host_IsQuakeCompatible( void )
{
	// feature set
	if( FBitSet( host.features, ENGINE_QUAKE_COMPATIBLE ))
		return true;

#if !XASH_DEDICATED
	// quake demo playing
	if( cls.demoplayback == DEMO_QUAKE1 )
		return true;
#endif // XASH_DEDICATED

	return false;
}

/*
================
Host_EndGame
================
*/
void Host_EndGame( qboolean abort, const char *message, ... )
{
	va_list		argptr;
	static char	string[MAX_SYSPATH];

	va_start( argptr, message );
	Q_vsnprintf( string, sizeof( string ), message, argptr );
	va_end( argptr );

	Con_Printf( "Host_EndGame: %s\n", string );

	SV_Shutdown( "\n" );
#if !XASH_DEDICATED
	CL_Disconnect();

	// recreate world if needs
	CL_ClearEdicts ();
#endif

	// release all models
	Mod_FreeAll();

	if( abort ) Host_AbortCurrentFrame ();
}

/*
================
Host_AbortCurrentFrame

aborts the current host frame and goes on with the next one
================
*/
void Host_AbortCurrentFrame( void )
{
	longjmp( host.abortframe, 1 );
}

/*
==================
Host_CalcSleep
==================
*/
static int Host_CalcSleep( void )
{
#ifndef XASH_DEDICATED
	// never sleep in timedemo for benchmarking purposes
	// also don't sleep with vsync for less lag
	if( CL_IsTimeDemo( ) || gl_vsync.value )
		return 0;
#endif

	if( Host_IsDedicated() )
	{
		// let the dedicated server some sleep
		return host_sleeptime.value;
	}

	switch( host.status )
	{
	case HOST_NOFOCUS:
		if( SV_Active() && CL_IsInGame())
			return host_sleeptime.value;
		// fallthrough
	case HOST_SLEEP:
		return 20;
	}

	return host_sleeptime.value;
}

static void Host_NewInstance( const char *name, const char *finalmsg )
{
	if( !pChangeGame ) return;

	host.change_game = true;
	Q_strncpy( host.finalmsg, finalmsg, sizeof( host.finalmsg ));

	if( !Sys_NewInstance( name ))
		pChangeGame( name ); // call from hl.exe
}

/*
=================
Host_ChangeGame_f

Change game modification
=================
*/
static void Host_ChangeGame_f( void )
{
	int	i;

	if( Cmd_Argc() != 2 )
	{
		Con_Printf( S_USAGE "game <directory>\n" );
		return;
	}

	// validate gamedir
	for( i = 0; i < FI->numgames; i++ )
	{
		if( !Q_stricmp( FI->games[i]->gamefolder, Cmd_Argv( 1 )))
			break;
	}

	if( i == FI->numgames )
	{
		Con_Printf( "%s not exist\n", Cmd_Argv( 1 ));
	}
	else if( !Q_stricmp( GI->gamefolder, Cmd_Argv( 1 )))
	{
		Con_Printf( "%s already active\n", Cmd_Argv( 1 ));
	}
	else
	{
		char finalmsg[MAX_VA_STRING];

		Q_snprintf( finalmsg, sizeof( finalmsg ), "change game to '%s'", FI->games[i]->title );
		Host_NewInstance( Cmd_Argv( 1 ), finalmsg );
	}
}

/*
===============
Host_Exec_f
===============
*/
static void Host_Exec_f( void )
{
	string cfgpath;
	byte *f;
	char *txt;
	fs_offset_t	len;
	const char *arg;

	if( Cmd_Argc() != 2 )
	{
		Con_Printf( S_USAGE "exec <filename>\n" );
		return;
	}

	arg = Cmd_Argv( 1 );

#ifndef XASH_DEDICATED
	if( !Cmd_CurrentCommandIsPrivileged() )
	{
		const char *unprivilegedWhitelist[] =
		{
			NULL, "mapdefault.cfg", "scout.cfg", "sniper.cfg",
			"soldier.cfg", "demoman.cfg", "medic.cfg", "hwguy.cfg",
			"pyro.cfg", "spy.cfg", "engineer.cfg", "civilian.cfg"
		};
		int i;
		char temp[MAX_VA_STRING];
		qboolean allow = false;

		Q_snprintf( temp, sizeof( temp ), "%s.cfg", clgame.mapname );
		unprivilegedWhitelist[0] = temp;

		for( i = 0; i < ARRAYSIZE( unprivilegedWhitelist ); i++ )
		{
			if( !Q_strcmp( arg, unprivilegedWhitelist[i] ))
			{
				allow = true;
				break;
			}
		}

		if( !allow )
		{
			Con_Printf( "exec %s: not privileged or in whitelist\n", arg );
			return;
		}
	}
#endif // XASH_DEDICATED

	if( !Q_stricmp( "game.cfg", arg ))
	{
		// don't execute game.cfg in singleplayer
		if( SV_GetMaxClients() == 1 )
			return;
	}

	Q_strncpy( cfgpath, arg, sizeof( cfgpath ));
	COM_DefaultExtension( cfgpath, ".cfg", sizeof( cfgpath )); // append as default

	f = FS_LoadFile( cfgpath, &len, false );
	if( !f )
	{
		Con_Reportf( "couldn't exec %s\n", Cmd_Argv( 1 ));
		return;
	}

	if( !Q_stricmp( "config.cfg", arg ))
		host.config_executed = true;

	// adds \n\0 at end of the file
	txt = Z_Calloc( len + 2 );
	memcpy( txt, f, len );
	Q_strncat( txt, "\n", len + 2 );
	Mem_Free( f );

	if( !host.apply_game_config )
		Con_Printf( "execing %s\n", arg );
	Cbuf_InsertText( txt );
	Mem_Free( txt );
}

/*
===============
Host_MemStats_f
===============
*/
static void Host_MemStats_f( void )
{
	switch( Cmd_Argc( ))
	{
	case 1:
		Mem_PrintList( 1<<30 );
		Mem_PrintStats();
		break;
	case 2:
		Mem_PrintList( Q_atoi( Cmd_Argv( 1 )) * 1024 );
		Mem_PrintStats();
		break;
	default:
		Con_Printf( S_USAGE "memlist <all>\n" );
		break;
	}
}

static void Host_Minimize_f( void )
{
#ifdef XASH_SDL
	if( host.hWnd ) SDL_MinimizeWindow( (SDL_Window*)host.hWnd );
#endif
}

/*
=================
Host_IsLocalGame

singleplayer game detect
=================
*/
qboolean Host_IsLocalGame( void )
{
	if( SV_Active( ))
	{
		return ( SV_GetMaxClients() == 1 ) ? true : false;
	}
	else
	{
		return ( CL_GetMaxClients() == 1 ) ? true : false;
	}
}

qboolean Host_IsLocalClient( void )
{
	// only the local client have the active server
	if( CL_Initialized( ) && SV_Initialized( ))
		return true;
	return false;
}

/*
=================
Host_RegisterDecal
=================
*/
static qboolean Host_RegisterDecal( const char *name, int *count )
{
	char	shortname[MAX_QPATH];
	int	i;

	if( !COM_CheckString( name ))
		return 0;

	COM_FileBase( name, shortname, sizeof( shortname ));

	for( i = 1; i < MAX_DECALS && host.draw_decals[i][0]; i++ )
	{
		if( !Q_stricmp( host.draw_decals[i], shortname ))
			return true;
	}

	if( i == MAX_DECALS )
	{
		Con_DPrintf( S_ERROR "MAX_DECALS limit exceeded (%d)\n", MAX_DECALS );
		return false;
	}

	// register new decal
	Q_strncpy( host.draw_decals[i], shortname, sizeof( host.draw_decals[i] ));
	*count += 1;

	return true;
}

/*
=================
Host_InitDecals
=================
*/
static void Host_InitDecals( void )
{
	int	i, num_decals = 0;
	search_t	*t;

	// NOTE: only once resource without which engine can't continue work
	if( !FS_FileExists( "gfx/conchars", false ))
		Sys_Error( "W_LoadWadFile: couldn't load gfx.wad\n" );

	memset( host.draw_decals, 0, sizeof( host.draw_decals ));

	// lookup all the decals in decals.wad (basedir, gamedir, falldir)
	t = FS_Search( "decals.wad/*.*", true, false );

	for( i = 0; t && i < t->numfilenames; i++ )
	{
		if( !Host_RegisterDecal( t->filenames[i], &num_decals ))
			break;
	}

	if( t ) Mem_Free( t );
	Con_Reportf( "InitDecals: %i decals\n", num_decals );
}

/*
===================
Host_GetCommands

Add them exactly as if they had been typed at the console
===================
*/
static void Host_GetCommands( void )
{
	char	*cmd;

	while( ( cmd = Sys_Input() ) )
	{
		Cbuf_AddText( cmd );
		Cbuf_Execute();
	}
}

/*
===================
Host_CalcFPS

compute actual FPS for various modes
===================
*/
static double Host_CalcFPS( void )
{
	double	fps = 0.0;

	if( Host_IsDedicated() )
	{
		fps = sys_ticrate.value;
	}
#if !XASH_DEDICATED
	else if( CL_IsPlaybackDemo() || CL_IsRecordDemo( )) // NOTE: we should play demos with same fps as it was recorded
	{
		fps = CL_GetDemoFramerate();
	}
	else if( Host_IsLocalGame( ))
	{
		if( !gl_vsync.value )
			fps = host_maxfps.value;
	}
	else
	{
		if( !gl_vsync.value )
		{
			fps = host_maxfps.value;
			if( fps == 0.0 ) fps = MAX_FPS;
			fps = bound( MIN_FPS, fps, MAX_FPS );
		}
	}
#endif

	return fps;
}

static qboolean Host_Autosleep( double dt, double scale )
{
	double targetframetime, fps;
	int sleep;

	fps = Host_CalcFPS();

	if( fps <= 0 )
		return true;

	// limit fps to withing tolerable range
	fps = bound( MIN_FPS, fps, MAX_FPS );

	if( Host_IsDedicated( ))
		targetframetime = ( 1.0 / ( fps + 1.0 ));
	else targetframetime = ( 1.0 / fps );

	sleep = Host_CalcSleep();
	if( sleep == 0 ) // no sleeps between frames, much simpler code
	{
		if( dt < targetframetime * scale )
			return false;
	}
	else
	{
		static double timewindow; // allocate a time window for sleeps
		static int counter; // for debug
		static double realsleeptime;
		const double sleeptime = sleep * 0.001;

		if( dt < targetframetime * scale )
		{
			// if we have allocated time window, try to sleep
			if( timewindow > realsleeptime )
			{
				// Sys_Sleep isn't guaranteed to sleep an exact amount of milliseconds
				// so we measure the real sleep time and use it to decrease the window
				double t1 = Sys_DoubleTime(), t2;
				Sys_Sleep( sleep ); // in msec!
				t2 = Sys_DoubleTime();
				realsleeptime = t2 - t1;

				timewindow -= realsleeptime;

				if( host_sleeptime_debug.value )
				{
					counter++;

					Con_NPrintf( counter, "%d: %.4f %.4f", counter, timewindow, realsleeptime );
				}
			}

			return false;
		}

		// if we exhausted this time window, allocate a new one after new frame
		if( timewindow <= realsleeptime )
		{
			double targetsleeptime = targetframetime - host.pureframetime * 2;

			if( targetsleeptime > 0 )
				timewindow = targetsleeptime;
			else timewindow = 0;

			realsleeptime = sleeptime; // reset in case CPU was too busy

			if( host_sleeptime_debug.value )
			{
				counter = 0;

				Con_NPrintf( 0, "tgt = %.4f, pft = %.4f, wnd = %.4f", targetframetime, host.pureframetime, timewindow );
			}
		}
	}

	return true;
}

/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
static qboolean Host_FilterTime( float time )
{
	static double	oldtime;
	double dt;
	double scale = sys_timescale.value;

	host.realtime += time * scale;
	dt = host.realtime - oldtime;

	// clamp the fps in multiplayer games
	if( !Host_Autosleep( dt, scale ))
		return false;

	host.frametime = host.realtime - oldtime;
	host.realframetime = bound( MIN_FRAMETIME, host.frametime, MAX_FRAMETIME );
	oldtime = host.realtime;

	// NOTE: allow only in singleplayer while demos are not active
	if( host_framerate.value > 0.0f && Host_IsLocalGame() && !CL_IsPlaybackDemo() && !CL_IsRecordDemo( ))
		host.frametime = bound( MIN_FRAMETIME, host_framerate.value * scale, MAX_FRAMETIME );
	else host.frametime = bound( MIN_FRAMETIME, host.frametime, MAX_FRAMETIME );

	return true;
}

/*
=================
Host_Frame
=================
*/
void Host_Frame( float time )
{
	double t1, t2;

	// decide the simulation time
	if( !Host_FilterTime( time ))
		return;

	t1 = Sys_DoubleTime();

	if( host.framecount == 0 )
		Con_DPrintf( "Time to first frame: %.3f seconds\n", t1 - host.starttime );

	Host_InputFrame ();  // input frame
	Host_ClientBegin (); // begin client
	Host_GetCommands (); // dedicated in
	Host_ServerFrame (); // server frame
	Host_ClientFrame (); // client frame
	HTTP_Run();			 // both server and client

	t2 = Sys_DoubleTime();

	host.pureframetime = t2 - t1;

	host.framecount++;
}

/*
=================
Host_Error
=================
*/
void GAME_EXPORT Host_Error( const char *error, ... )
{
	static char	hosterror1[MAX_SYSPATH];
	static char	hosterror2[MAX_SYSPATH];
	static qboolean	recursive = false;
	va_list		argptr;

	va_start( argptr, error );
	Q_vsnprintf( hosterror1, sizeof( hosterror1 ), error, argptr );
	va_end( argptr );

	CL_WriteMessageHistory (); // before Q_error call

	if( host.framecount < 3 )
	{
		Sys_Error( "Host_InitError: %s", hosterror1 );
	}
	else if( host.framecount == host.errorframe )
	{
		Sys_Error( "Host_MultiError: %s", hosterror2 );
	}
	else
	{
		if( host.allow_console )
		{
			UI_SetActiveMenu( false );
			Key_SetKeyDest( key_console );
			Con_Printf( "Host_Error: %s", hosterror1 );
		}
		else Platform_MessageBox( "Host Error", hosterror1, true );
	}

	// host is shutting down. don't invoke infinite loop
	if( host.status == HOST_SHUTDOWN ) return;

	if( recursive )
	{
		Con_Printf( "Host_RecursiveError: %s", hosterror2 );
		Sys_Error( "%s", hosterror1 );
	}

	recursive = true;
	Q_strncpy( hosterror2, hosterror1, MAX_SYSPATH );
	host.errorframe = host.framecount; // to avoid multply calls per frame
	Q_snprintf( host.finalmsg, sizeof( host.finalmsg ), "Server crashed: %s", hosterror1 );

	// clearing cmd buffer to prevent execute any commands
	COM_InitHostState();
	Cbuf_Clear();

	Host_ShutdownServer();
	CL_Drop(); // drop clients

	// recreate world if needs
	CL_ClearEdicts ();

	// release all models
	Mod_FreeAll();

	recursive = false;
	Host_AbortCurrentFrame();
}

static void Host_Error_f( void )
{
	const char *error = Cmd_Argv( 1 );

	if( !*error ) error = "Invoked host error";
	Host_Error( "%s\n", error );
}

static void Sys_Error_f( void )
{
	const char *error = Cmd_Argv( 1 );

	if( !*error ) error = "Invoked sys error";
	Sys_Error( "%s\n", error );
}

/*
=================
Host_Crash_f
=================
*/
static void Host_Crash_f( void )
{
	*(volatile int *)0 = 0xffffffff;
}

/*
=================
Host_Userconfigd_f
=================
*/
static void Host_Userconfigd_f( void )
{
	search_t *t;
	int i;

	t = FS_Search( "userconfig.d/*.cfg", true, false );
	if( !t ) return;

	for( i = 0; i < t->numfilenames; i++ )
	{
		Cbuf_AddTextf( "exec %s\n", t->filenames[i] );
	}

	Mem_Free( t );
}

#if XASH_ENGINE_TESTS
static void Host_RunTests( int stage )
{
	switch( stage )
	{
	case 0: // early engine load
		memset( &tests_stats, 0, sizeof( tests_stats ));
		TEST_LIST_0;
#if !XASH_DEDICATED
		TEST_LIST_0_CLIENT;
#endif /* XASH_DEDICATED */
		break;
	case 1: // after FS load
		TEST_LIST_1;
#if !XASH_DEDICATED
		TEST_LIST_1_CLIENT;
#endif
		Msg( "Done! %d passed, %d failed\n", tests_stats.passed, tests_stats.failed );
		Sys_Quit();
	}
}
#endif

/*
=================
Host_InitCommon
=================
*/
static void Host_InitCommon( int argc, char **argv, const char *progname, qboolean bChangeGame )
{
	char		dev_level[4];
	int		developer = DEFAULT_DEV;
	const char *baseDir;
	char ticrate[16];
	int len, i;

	// some commands may turn engine into infinite loop,
	// e.g. xash.exe +game xash -game xash
	// so we clear all cmd_args, but leave dbg states as well
	Sys_ParseCommandLine( argc, argv );

	if( !Sys_CheckParm( "-disablehelp" ) )
	{
		 if( Sys_CheckParm( "-help" ) || Sys_CheckParm( "-h" ) || Sys_CheckParm( "--help" ) )
		 {
			Sys_PrintUsage();
		 }
	}

	if( !Sys_CheckParm( "-noch" ) )
		Sys_SetupCrashHandler();

	host.enabledll = !Sys_CheckParm( "-nodll" );

	host.change_game = bChangeGame || Sys_CheckParm( "-changegame" );
	host.config_executed = false;
	host.status = HOST_INIT; // initialzation started

	Memory_Init(); // init memory subsystem

	host.mempool = Mem_AllocPool( "Zone Engine" );

	host.allow_console = DEFAULT_ALLOWCONSOLE;

	// HACKHACK: Quake console is always allowed
	// TODO: determine if we are running QWrap more reliable
	if( !host.allow_console && ( Sys_CheckParm( "-console" ) || !Q_stricmp( SI.exeName, "quake" )))
		host.allow_console = true;

	if( Sys_CheckParm( "-dev" ))
	{
		host.allow_console = true;
		developer = DEV_NORMAL;

		if( Sys_GetParmFromCmdLine( "-dev", dev_level ))
		{
			if( Q_isdigit( dev_level ))
				developer = bound( DEV_NONE, abs( Q_atoi( dev_level )), DEV_EXTENDED );
		}
	}

#if XASH_ENGINE_TESTS
	if( Sys_CheckParm( "-runtests" ))
	{
		host.allow_console = true;
		developer = DEV_EXTENDED;
	}
#endif

	host.con_showalways = true;

#if XASH_DEDICATED
	host.type = HOST_DEDICATED; // predict state
#else
	if( Sys_CheckParm("-dedicated") || progname[0] == '#' )
	{
		host.type = HOST_DEDICATED;
	}
	else
	{
		host.type = HOST_NORMAL;
	}
#endif

	// set default gamedir
	if( progname[0] == '#' )
		progname++;

	Q_strncpy( SI.exeName, progname, sizeof( SI.exeName ));
	Q_strncpy( SI.basedirName, progname, sizeof( SI.basedirName ));

	if( Host_IsDedicated() )
	{
		Sys_MergeCommandLine( );

		host.allow_console = true;
	}
	else
	{
		// don't show console as default
		if( developer <= DEV_NORMAL )
			host.con_showalways = false;
	}

	// member console allowing
	host.allow_console_init = host.allow_console;

	if( Sys_CheckParm( "-bugcomp" ))
	{
		// add argument check here when we add other levels
		// of bugcompatibility
		host.bugcomp = BUGCOMP_GOLDSRC;
	}

	// timeBeginPeriod( 1 ); // a1ba: Do we need this?

	// NOTE: this message couldn't be passed into game console but it doesn't matter
//	Con_Reportf( "Sys_LoadLibrary: Loading xash.dll - ok\n" );

	// get default screen res
	VID_InitDefaultResolution();

	// init host state machine
	COM_InitHostState();

	// init hashed commands
	BaseCmd_Init();

	// startup cmds and cvars subsystem
	Cmd_Init();
	Cvar_Init();

	// share developer level across all dlls
	Q_snprintf( dev_level, sizeof( dev_level ), "%i", developer );
	Cvar_DirectSet( &host_developer, dev_level );
	Cvar_RegisterVariable( &sys_ticrate );

	if( Sys_GetParmFromCmdLine( "-sys_ticrate", ticrate ))
	{
		double fps = bound( MIN_FPS, atof( ticrate ), MAX_FPS );
		Cvar_SetValue( "sys_ticrate", fps );
	}

	Con_Init(); // early console running to catch all the messages

#if XASH_ENGINE_TESTS
	if( Sys_CheckParm( "-runtests" ))
		Host_RunTests( 0 );
#endif

	Platform_Init();

	baseDir = getenv( "XASH3D_BASEDIR" );

	if( COM_CheckString( baseDir ) )
	{
		host.rootdir = baseDir;
	}
	else
	{
#if TARGET_OS_IOS
		Q_strncpy( host.rootdir, IOS_GetDocsDir(), sizeof( host.rootdir ));
#elif XASH_ANDROID && XASH_SDL
		Q_strncpy( host.rootdir, SDL_AndroidGetExternalStoragePath(), sizeof( host.rootdir ));
#elif XASH_PSVITA
		if ( !PSVita_GetBasePath( host.rootdir, sizeof( host.rootdir )))
		{
			Sys_Error( "couldn't find xash3d data directory" );
			host.rootdir[0] = 0;
		}
#elif (XASH_SDL == 2) && !XASH_NSWITCH && !EMSCRIPTEN// GetBasePath not impl'd in switch-sdl2
		char *szBasePath = SDL_GetBasePath();
		if( szBasePath )
		{
			//Q_strncpy( (char*)host.rootdir.c_str(), szBasePath, sizeof(host.rootdir));
			host.rootdir = szBasePath;
			SDL_free( szBasePath );
		}
		else
		{
#if 0
#if XASH_POSIX || XASH_WIN32
			if( !getcwd( host.rootdir, sizeof( host.rootdir )))
				Sys_Error( "couldn't determine current directory: %s, getcwd: %s", SDL_GetError(), strerror( errno ));
#else
			Sys_Error( "couldn't determine current directory: %s", SDL_GetError() );
#endif
#endif
		}
#else
		host.rootdir = "./";
#endif
	}

#if XASH_WIN32
	COM_FixSlashes( (char*)host.rootdir.c_str() );
#endif

	len = Q_strlen( host.rootdir.c_str() );

	if( len && host.rootdir[len - 1] == '/' )
		host.rootdir[len - 1] = 0;

	// get readonly root. The order is: check for arg, then env.
	// if still not got it, rodir is disabled.
	host.rodir[0] = '\0';
	if( !Sys_GetParmFromCmdLine( "-rodir", host.rodir ))
	{
		char *roDir = getenv( "XASH3D_RODIR" );

		if( COM_CheckString( roDir ))
			Q_strncpy( host.rodir, roDir, sizeof( host.rodir ));
	}

#if XASH_WIN32
	COM_FixSlashes( (char*)host.rootdir.c_str() );
#endif

	len = Q_strlen( host.rodir );

	if( len && host.rodir[len - 1] == '/' )
		host.rodir[len - 1] = 0;

	if( !COM_CheckStringEmpty( host.rootdir.c_str() ))
	{
		Sys_Error( "Changing working directory failed (empty working directory)\n" );
		return;
	}

	FS_LoadProgs();

	// TODO: this function will cause engine to stop in case of fail
	// when it will have an option to return string error, restore Sys_Error
	FS_SetCurrentDirectory( host.rootdir.c_str() );

	FS_Init();

	Sys_InitLog();

	// print bugcompatibility level here, after log was initialized
	if( host.bugcomp == BUGCOMP_GOLDSRC )
	{
		Con_Printf( "^3BUGCOMP^7: GoldSrc bug-compatibility enabled\n" );
	}

	Cmd_AddCommand( "exec", Host_Exec_f, "execute a script file" );
	Cmd_AddCommand( "memlist", Host_MemStats_f, "prints memory pool information" );
	Cmd_AddRestrictedCommand( "userconfigd", Host_Userconfigd_f, "execute all scripts from userconfig.d" );

	Image_Init();
	Sound_Init();

#if XASH_ENGINE_TESTS
	if( Sys_CheckParm( "-runtests" ))
		Host_RunTests( 1 );
#endif

	FS_LoadGameInfo( NULL );
	Cvar_PostFSInit();

	Q_strncpy( host.gamefolder, GI->gamefolder, sizeof( host.gamefolder ));

	for( i = 0; i < 3; i++ )
	{
		const char *rcName;
		switch( i )
		{
		case 0: rcName = SI.basedirName; break; // e.g. valve.rc
		case 1: rcName = SI.exeName; break;     // e.g. quake.rc
		case 2: rcName = host.gamefolder; break; // e.g. game.rc (ran from default launcher)
		}

		if( FS_FileExists( va( "%s.rc", rcName ), false ))
		{
			Q_strncpy( SI.rcName, rcName, sizeof( SI.rcName ));
			break;
		}
	}

	Image_CheckPaletteQ1 ();
	Host_InitDecals ();	// reload decals

	// DEPRECATED: by FWGS fork
#if 0
	if( GI->secure )
	{
		// clear all developer levels when game is protected
		Cvar_DirectSet( &host_developer, "0" );
		host.allow_console_init = false;
		host.con_showalways = false;
		host.allow_console = false;
	}
#endif
	HPAK_Init();

	IN_Init();
	Key_Init();
}

static void Host_FreeCommon( void )
{
	Image_Shutdown();
	Sound_Shutdown();
	Netchan_Shutdown();
	HPAK_FlushHostQueue();
	FS_Shutdown();
}

/*
=================
Host_Main
=================
*/
static double Host_FrameOldTime;
static double Host_FrameNewTime;

static void COM_Frame()
{
#ifdef EMSCRIPTEN
	static bool sync_inited = false;

	if (!sync_inited)
	{
		sync_inited = true;

		EM_ASM(
			FS.mkdir('/valve/downloaded');
			FS.mount(IDBFS, {}, '/valve/downloaded');
			Module.syncdone = 0;
			FS.syncfs(true, function(err) {
				if (err) {
					console.log('FS.syncfs error:', err);
				}
				Module.syncdone = 1;
			});
		);
	}

	if (emscripten_run_script_int("Module.syncdone") == 0)
		return;
#endif

	Host_FrameNewTime = Sys_DoubleTime();
	SCHEDULER->frame();
	COM_Frame(Host_FrameNewTime - Host_FrameOldTime);
	Host_FrameOldTime = Host_FrameNewTime;
}

int EXPORT Host_Main( int argc, char **argv, const char *progname, int bChangeGame, pfnChangeGame func )
{
	static double	oldtime, newtime;
	string demoname;

	host.starttime = Sys_DoubleTime();

	pChangeGame = func;	// may be NULL

	Host_InitCommon( argc, argv, progname, bChangeGame );

	// init commands and vars
	if( host_developer.value >= DEV_EXTENDED )
	{
		Cmd_AddRestrictedCommand ( "sys_error", Sys_Error_f, "just throw a fatal error to test shutdown procedures");
		Cmd_AddRestrictedCommand ( "host_error", Host_Error_f, "just throw a host error to test shutdown procedures");
		Cmd_AddRestrictedCommand ( "crash", Host_Crash_f, "a way to force a bus error for development reasons");
	}

	Cvar_RegisterVariable( &host_serverstate );
	Cvar_RegisterVariable( &host_maxfps );
	Cvar_RegisterVariable( &host_framerate );
	Cvar_RegisterVariable( &host_sleeptime );
	Cvar_RegisterVariable( &host_sleeptime_debug );
	Cvar_RegisterVariable( &host_gameloaded );
	Cvar_RegisterVariable( &host_clientloaded );
	Cvar_RegisterVariable( &host_limitlocal );
	Cvar_RegisterVariable( &con_gamemaps );
	Cvar_RegisterVariable( &sys_timescale );

	Cvar_Getf( "buildnum", FCVAR_READ_ONLY, "returns a current build number", "%i", Q_buildnum_compat());
	Cvar_Getf( "ver", FCVAR_READ_ONLY, "shows an engine version", "%i/%s (hw build %i)", PROTOCOL_VERSION, XASH_COMPAT_VERSION, Q_buildnum_compat());
	Cvar_Getf( "host_ver", FCVAR_READ_ONLY, "detailed info about this build", "%i " XASH_VERSION " %s %s %s", Q_buildnum(), Q_buildos(), Q_buildarch(), Q_buildcommit());
	Cvar_Getf( "host_lowmemorymode", FCVAR_READ_ONLY, "indicates if engine compiled for low RAM consumption (0 - normal, 1 - low engine limits, 2 - low protocol limits)", "%i", XASH_LOW_MEMORY );

	Mod_Init();
	NET_Init();
	NET_InitMasters();
	Netchan_Init();

	// allow to change game from the console
	if( pChangeGame != NULL )
	{
		Cmd_AddRestrictedCommand( "game", Host_ChangeGame_f, "change game" );
		Cvar_Get( "host_allow_changegame", "1", FCVAR_READ_ONLY, "allows to change games" );
	}
	else
	{
		Cvar_Get( "host_allow_changegame", "0", FCVAR_READ_ONLY, "allows to change games" );
	}

	SV_Init();
	CL_Init();

	HTTP_Init();
	ID_Init();

	if( Host_IsDedicated() )
	{
#ifdef _WIN32
		Wcon_InitConsoleCommands ();
#endif

		Cmd_AddRestrictedCommand( "quit", Sys_Quit, "quit the game" );
		Cmd_AddRestrictedCommand( "exit", Sys_Quit, "quit the game" );
	}
	else Cmd_AddRestrictedCommand( "minimize", Host_Minimize_f, "minimize main window to tray" );

	HPAK_CheckIntegrity( CUSTOM_RES_PATH );

	host.errorframe = 0;

	// post initializations
	switch( host.type )
	{
	case HOST_NORMAL:
#ifdef _WIN32
		Wcon_ShowConsole( false ); // hide console
#endif
		// execute startup config and cmdline
		Cbuf_AddTextf( "exec %s.rc\n", SI.rcName );
		Cbuf_Execute();
		if( !host.config_executed )
		{
			Cbuf_AddText( "exec config.cfg\n" );
			Cbuf_Execute();
		}
		// exec all files from userconfig.d
		Host_Userconfigd_f();
		break;
	case HOST_DEDICATED:
		// allways parse commandline in dedicated-mode
		host.stuffcmds_pending = true;
		break;
	}

	host.change_game = false;	// done
	Cmd_RemoveCommand( "setgl" );
	Cbuf_ExecStuffCmds();	// execute stuffcmds (commandline)
	SCR_CheckStartupVids();	// must be last

	if( Sys_GetParmFromCmdLine( "-timedemo", demoname ))
		Cbuf_AddTextf( "timedemo %s\n", demoname );

	oldtime = Sys_DoubleTime() - 0.1;

	if( Host_IsDedicated( ))
	{
		// in dedicated server input system can't set HOST_FRAME status
		// so set it here as we're finished initializing
		host.status = HOST_FRAME;

		if( GameState->nextstate == STATE_RUNFRAME )
			Con_Printf( "Type 'map <mapname>' to start game... (TAB-autocomplete is working too)\n" );

		// execute server.cfg after commandline
		// so we have a chance to set servercfgfile
		Cbuf_AddTextf( "exec %s\n", Cvar_VariableString( "servercfgfile" ));
		Cbuf_Execute();
	}

#ifdef EMSCRIPTEN
	emscripten_set_main_loop(COM_Frame, 0, 1);
#else
	// main window message loop
	while( !host.crashed )
	{
		COM_Frame();
	}
#endif

	// never reached
	return 0;
}

/*
=================
Host_Shutdown
=================
*/
void EXPORT Host_Shutdown( void )
{
	qboolean error = host.status == HOST_ERR_FATAL;

	if( host.shutdown_issued ) return;
	host.shutdown_issued = true;

	if( host.status != HOST_ERR_FATAL ) host.status = HOST_SHUTDOWN; // prepare host to normal shutdown
	if( !host.change_game ) Q_strncpy( host.finalmsg, "Server shutdown", sizeof( host.finalmsg ));

#if !XASH_DEDICATED
	if( host.type == HOST_NORMAL && !error )
		Host_WriteConfig();
#endif

	SV_Shutdown( "Server shutdown\n" );
	SV_UnloadProgs();
	SV_ShutdownFilter();
	CL_Shutdown();

	Mod_Shutdown();
	NET_Shutdown();
	HTTP_Shutdown();
	Host_FreeCommon();
	Platform_Shutdown();

	// must be last, console uses this
	Mem_FreePool( &host.mempool );

	// restore filter
	Sys_RestoreCrashHandler();
	Sys_CloseLog();
}
