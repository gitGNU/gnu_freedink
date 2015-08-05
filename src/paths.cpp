/**
 * Compute and store the search paths

 * Copyright (C) 2007, 2008, 2009, 2014  Sylvain Beucler

 * This file is part of GNU FreeDink

 * GNU FreeDink is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.

 * GNU FreeDink is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h> /* strdup */
#include <unistd.h> /* getcwd */
#include <errno.h>

#include "io_util.h"
#include "paths.h"
#include "msgbox.h"
#include "log.h"

#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0401
#include <windows.h>
#include <shlobj.h>
#endif

/* mkdir */
#include <sys/stat.h>
#include <sys/types.h>
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
#include <direct.h>
#define mkdir(name,mode) mkdir(name)
#endif

#include "str_util.h" /* asprintf_append */

#ifdef __ANDROID__
#include <unistd.h> /* chdir */
#include <errno.h>
#include "SDL.h"
#endif

/* Pleases gnulib's error module */
char* program_name = PACKAGE;

static char* pkgdatadir = NULL;
static char* fallbackdir = NULL;
static char* dmoddir = NULL;
static char* dmodname = NULL;
static char* userappdir = NULL;

/**
 * getcwd without size restriction
 * aka get_current_dir_name(3)
 */
char* paths_getcwd()
{
  char* cwd = NULL;
  int cwd_size = PATH_MAX;
  do {
    cwd = (char*)realloc(cwd, cwd_size);
    getcwd(cwd, cwd_size);
    cwd_size *= 2;
  } while (errno == ERANGE);
  return cwd;
}

static char * br_strcat (const char *str1, const char *str2)
{
  char *result;
  size_t len1, len2;
  
  if (str1 == NULL)
    str1 = "";
  if (str2 == NULL)
    str2 = "";
  
  len1 = strlen (str1);
  len2 = strlen (str2);
  
  result = (char *) malloc (len1 + len2 + 1);
  memcpy (result, str1, len1);
  memcpy (result + len1, str2, len2);
  result[len1 + len2] = '\0';
  
  return result;
}

static char* br_build_path (const char *dir, const char *file)
{
  char *dir2, *result;
  size_t len;
  int must_free = 0;
  
  len = strlen (dir);
  if (len > 0 && dir[len - 1] != '/') {
    dir2 = br_strcat (dir, "/");
    must_free = 1;
  } else
    dir2 = (char *) dir;
  
  result = br_strcat (dir2, file);
  if (must_free)
    free (dir2);
  return result;
}


void ts_paths_init() {
  pkgdatadir = strdup("");
  fallbackdir = strdup("");
  dmoddir = strdup("");
  dmodname = strdup("");
  userappdir = strdup("");
}

void paths_init(char *argv0, char *refdir_opt, char *dmoddir_opt)
{
  char *refdir = NULL;

  /** pkgdatadir **/
  {
#if defined __ANDROID__ || defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
    /* "./freedink" */
    pkgdatadir = strdup("./" PACKAGE);
#else
    /* (e.g. "/usr/share/freedink") */
    pkgdatadir = br_build_path(BUILD_DATA_DIR, PACKAGE);
#endif
  }

  /** refdir  (e.g. "/usr/share/dink") **/
  {
    /** => refdir **/
    char* match = NULL;
    int nb_dirs = 7;
    char* lookup[nb_dirs];
    int i = 0;
    if (refdir_opt == NULL)
      lookup[0] = NULL;
    else
      lookup[0] = refdir_opt;

    /* Use absolute filename, otherwise SDL_rwops fails on Android (java.io.FileNotFoundException) */
    lookup[1] = paths_getcwd();

    /* FHS mentions optional 'share/games' which some Debian packagers
       seem to be found of */
    /* PACKAGERS: don't alter these paths. FreeDink must run in a
       _consistent_ way across platforms. If you need an alternate
       path, consider using ./configure --prefix=..., or contact
       bug-freedink@gnu.org to discuss it. */
    lookup[2] = br_build_path(BUILD_DATA_DIR, "dink");
    lookup[3] = "/usr/local/share/games/dink";
    lookup[4] = "/usr/local/share/dink";
    lookup[5] = "/usr/share/games/dink";
    lookup[6] = "/usr/share/dink";

    for (; i < nb_dirs; i++)
      {
	char *dir_graphics_ci = NULL, *dir_tiles_ci = NULL;
	if (lookup[i] == NULL)
	  continue;
	dir_graphics_ci = br_build_path(lookup[i], "dink/graphics");
	dir_tiles_ci = br_build_path(lookup[i], "dink/tiles");
	ciconvert(dir_graphics_ci);
	ciconvert(dir_tiles_ci);
	if (is_directory(dir_graphics_ci) && is_directory(dir_tiles_ci))
	  {
	    match = lookup[i];
	  }

	if (match == NULL && i == 0)
	  {
	    msgbox("Invalid --refdir option: %s and/or %s are not accessible.",
		   dir_graphics_ci, dir_tiles_ci);
	    exit(1);
	  }

	free(dir_graphics_ci);
	free(dir_tiles_ci);
	if (match != NULL)
	    break;
      }
    refdir = match;
    if (refdir != NULL)
      {
	refdir = strdup(refdir);
      }
    else
      {
	char *msg = NULL;
	asprintf_append(&msg, "Error: cannot find reference directory (--refdir). I looked in:\n");
	// lookup[0] already treated above
	asprintf_append(&msg, "- %s [current dir]\n", lookup[1]);
	asprintf_append(&msg, "- %s [build prefix]\n", lookup[2]);
	asprintf_append(&msg, "- %s [hard-coded /usr/local/share/games prefix]\n", lookup[3]);
	asprintf_append(&msg, "- %s [hard-coded /usr/local/share prefix]\n", lookup[4]);
	asprintf_append(&msg, "- %s [hard-coded /usr/share/games prefix]\n", lookup[5]);
	asprintf_append(&msg, "- %s [hard-coded /usr/share prefix]\n", lookup[6]);
	asprintf_append(&msg, "The reference directory contains among others the "
		"'dink/graphics/' and 'dink/tiles/' directories (as well as "
		"D-Mods).");
	msgbox(msg);
	free(msg);
	exit(1);
      }

    free(lookup[1]); // paths_getcwd
    free(lookup[2]); // br_build_path
  }

  /** fallbackdir (e.g. "/usr/share/dink/dink") **/
  /* (directory used when a file cannot be found in a D-Mod) */
  {
    fallbackdir = br_strcat(refdir, "/dink");
  }

  /** dmoddir (e.g. "/usr/share/dink/island") **/
  {
    if (dmoddir_opt != NULL && is_directory(dmoddir_opt))
      {
	/* Use path given on the command line, either a full path or a
	   path relative to the current directory. */
	/* Note: don't search for "dink" in the default dir if no
	   '-game' option was given */
	dmoddir = strdup(dmoddir_opt);
      }
    else
      {
	/* Use path given on the command line, relative to $refdir */
	char *subdir = dmoddir_opt;
	if (subdir == NULL || strlen(subdir) == 0)  /* no opt, or -g '' */
	  subdir = "dink";
	dmoddir = (char*)malloc(strlen(refdir) + 1 + strlen(subdir) + 1);
	strcpy(dmoddir, refdir);
	strcat(dmoddir, "/");
	strcat(dmoddir, subdir);
	if (!is_directory(dmoddir))
	  {
	    char *msg = NULL;

	    asprintf_append(&msg, "Error: D-Mod directory '%s' doesn't exist. I looked in:\n", subdir);
	    if (dmoddir_opt != NULL)
	      asprintf_append(&msg, "- ./%s\n", dmoddir_opt);
	    asprintf_append(&msg, "- %s (refdir is '%s')", dmoddir, refdir);

	    msgbox(msg);
	    free(msg);
	    exit(1);
	  }
      }
    /* Strip slashes */
    while (strlen(dmoddir) > 0 && dmoddir[strlen(dmoddir) - 1] == '/')
      dmoddir[strlen(dmoddir) - 1] = '\0';
  }

  /** dmodname (e.g. "island") **/
  /* Used to save games in ~/.dink/<dmod>/... */
  {
    /* Don't accept '.' or '' or '/..' (nameless) but accept 'island' (relative path) */
    int len = strlen(dmoddir);
    char* start = dmoddir + len;
    for (; start >= dmoddir; start--)  /* basename(1) */
      if (*start == '\\' || *start == '/')
	break;
    start++;  // we're either -1 or at the last slash, start is right after
    if (strcmp(start, ".") == 0 || strcmp(start, "..") == 0
	|| strcmp(start, "") == 0)
      {
	msgbox("Error: not loading nameless D-Mod '%s'", start);
	exit(1);
      }
    int dmodname_len = start - dmoddir;
    dmodname = (char*)malloc(dmodname_len+1);
    strncpy(dmodname, start, dmodname_len);
    dmodname[dmodname_len] = '\0';
  }

  /** userappdir (e.g. "~/.dink") **/
  {
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
    userappdir = (char*)malloc(MAX_PATH);
    /* C:\Documents and Settings\name\Application Data */
    SHGetSpecialFolderPath(NULL, userappdir, CSIDL_APPDATA, 1);
#else
    char* envhome = getenv("HOME");
    if (envhome != NULL)
      userappdir = strdup(getenv("HOME"));
#endif
    if (userappdir != NULL)
      {
	userappdir = (char*)realloc(userappdir, strlen(userappdir) + 1 + 1 + strlen(PACKAGE) + 1);
	strcat(userappdir, "/");
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
#else
	strcat(userappdir, ".");
#endif
	strcat(userappdir, "dink");
      }
    else
      {
	// No detected home directory - saving in the reference
	// directory
	userappdir = strdup(refdir);
      }
  }

  log_info("datadir = %s", BUILD_DATA_DIR);
  log_info("pkgdatadir = %s", pkgdatadir);
  log_info("refdir = %s", refdir);
  log_info("fallbackdir = %s", fallbackdir);
  log_info("dmoddir = %s", dmoddir);
  log_info("dmodname = %s", dmodname);
  log_info("userappdir = %s", userappdir);

  free(refdir);
}


const char *paths_getpkgdatadir(void)
{
  return pkgdatadir;
}
const char *paths_getdmoddir(void)
{
  return dmoddir;
}
const char *paths_getdmodname(void)
{
  return dmodname;
}

const char *paths_getfallbackdir(void)
{
  return fallbackdir;
}

char* paths_dmodfile(char *file)
{
  char *fullpath = br_build_path(dmoddir, file);
  ciconvert(fullpath);
  return fullpath;
}

FILE* paths_dmodfile_fopen(char *file, char *mode)
{
  char *fullpath = paths_dmodfile(file);
  FILE *result = fopen(fullpath, mode);
  free(fullpath);
  return result;
}

char* paths_fallbackfile(char *file)
{
  char *fullpath = br_build_path(fallbackdir, file);
  ciconvert(fullpath);
  return fullpath;
}

FILE* paths_fallbackfile_fopen(char *file, char *mode)
{
  char *fullpath = paths_fallbackfile(file);
  FILE *result = fopen(fullpath, mode);
  free(fullpath);
  return result;
}

char* paths_pkgdatafile(char *file)
{
  char *fullpath = br_build_path(pkgdatadir, file);
  ciconvert(fullpath);
  return fullpath;
}

FILE* paths_pkgdatafile_fopen(char *file, char *mode)
{
  char *fullpath = paths_pkgdatafile(file);
  FILE *result = fopen(fullpath, mode);
  free(fullpath);
  return result;
}

FILE *paths_savegame_fopen(int num, char *mode)
{
  char *fullpath_in_dmoddir = NULL;
  char *fullpath_in_userappdir = NULL;
  FILE *fp = NULL;

  /* 20 decimal digits max for 64bit integer - should be enough :) */
  char file[4 + 20 + 4 + 1];
  sprintf(file, "save%d.dat", num);


  /** fullpath_in_userappdir **/
  char *savedir = strdup(userappdir);
  savedir = (char*)realloc(savedir, strlen(userappdir) + 1 + strlen(dmodname) + 1);
  strcat(savedir, "/");
  strcat(savedir, dmodname);
  /* Create directories if needed */
  if (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL)
      /* Note: 0777 & umask => 0755 in common case */
      if ((!is_directory(userappdir) && (mkdir(userappdir, 0777) < 0))
	  || (!is_directory(savedir) && (mkdir(savedir, 0777) < 0)))
	{
	  free(savedir);
	  return NULL;
	}
  fullpath_in_userappdir = br_build_path(savedir, file);
  ciconvert(fullpath_in_userappdir);
  free(savedir);


  /** fullpath_in_dmoddir **/
  fullpath_in_dmoddir = paths_dmodfile(file);
  ciconvert(fullpath_in_dmoddir);
  

  /* Try ~/.dink (if present) when reading - but don't try that
     first when writing */
  if (strchr(mode, 'r') != NULL)
    fp = fopen(fullpath_in_userappdir, mode);

  /* Try in the D-Mod dir */
  if (fp == NULL)
    fp = fopen(fullpath_in_dmoddir, mode);

  /* Then try in ~/.dink */
  if (fp == NULL)
    fp = fopen(fullpath_in_userappdir, mode);

  free(fullpath_in_dmoddir);
  free(fullpath_in_userappdir);

  return fp;
}

void paths_quit(void)
{
  free(pkgdatadir);
  free(fallbackdir);
  free(dmoddir);
  free(dmodname);
  free(userappdir);

  pkgdatadir        = NULL;
  fallbackdir       = NULL;
  dmoddir           = NULL;
  dmodname          = NULL;
  userappdir        = NULL;
}