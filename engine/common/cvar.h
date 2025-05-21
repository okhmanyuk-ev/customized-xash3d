/*
cvar.h - dynamic variable tracking
Copyright (C) 2017 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef CVAR_H
#define CVAR_H

#include "cvardef.h"

cvar_t *Cvar_GetList( void );
#define Cvar_FindVar( name )	Cvar_FindVarExt( name, 0 )
convar_t *Cvar_FindVarExt( const char *var_name, int ignore_group );
void Cvar_RegisterVariable( convar_t *var );
convar_t *Cvar_Get( const char *var_name, const char *value, int flags, const char *description );
convar_t *Cvar_Getf( const char *var_name, int flags, const char *description, const char *format, ... );
void Cvar_LookupVars( int checkbit, void *buffer, void *ptr, setpair_t callback );
void Cvar_FullSet( const char *var_name, const char *value, int flags );
void Cvar_DirectSet( convar_t *var, const char *value );
void Cvar_DirectSetValue( convar_t *var, float value );
void Cvar_Set( const char *var_name, const char *value );
void Cvar_SetValue( const char *var_name, float value );
const char *Cvar_BuildAutoDescription( const char *szName, int flags );
float Cvar_VariableValue( const char *var_name );
int Cvar_VariableInteger( const char *var_name );
const char *Cvar_VariableString( const char *var_name );
void Cvar_WriteVariables( file_t *f, int group );
qboolean Cvar_Exists( const char *var_name );
void Cvar_Reset( const char *var_name );
void Cvar_SetCheatState( void );
qboolean Cvar_CommandWithPrivilegeCheck( convar_t *v, qboolean isPrivileged );
void Cvar_Init( void );
void Cvar_PostFSInit( void );
void Cvar_Unlink( int group );

#endif//CVAR_H
