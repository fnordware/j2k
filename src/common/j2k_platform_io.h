
/* ---------------------------------------------------------------------
// 
// j2k - JPEG 2000 plug-ins for Adobe programs
// Copyright (c) 2002-2016,  Brendan Bolles, http://www.fnordware.com
// Copyright (c) 2016,       Aaron Boxer,    http://grokimagecompression.github.io/grok
// 
// This file is part of j2k.
//
// j2k is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
// -------------------------------------------------------------------*/

#ifndef J2K_PLATFORM_IO_H
#define J2K_PLATFORM_IO_H

#include "j2k_io.h"


#ifdef WIN32
#include <Windows.h>
typedef unsigned short uint16_t;
#endif

#if defined(__APPLE__) || defined(macintosh)

#include <MacTypes.h>

#ifndef MAC_OS_X_VERSION_10_5
#define MAC_OS_X_VERSION_10_5 1050
#endif

#ifndef MAC_OS_X_VERSION_MAX_ALLOWED
#define MAC_OS_X_VERSION_MAX_ALLOWED 0
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5
typedef SInt16 FSIORefNum;
#endif
#endif // __APPLE__


class PlatformInputFile : public j2k::InputFile
{
  public:
	PlatformInputFile(const char *path);
	PlatformInputFile(const uint16_t *path);
	virtual ~PlatformInputFile();
	
	virtual ReadFlags Flags() const { return J2K_READ_SEEKABLE; }
	
	virtual size_t FileSize();
	virtual size_t Read(void *buf, size_t num_bytes);
	virtual bool Seek(size_t position);
	virtual size_t Tell();

  private:
#if defined(__APPLE__) || defined(macintosh)
	FSRef _fsRef;
	FSIORefNum _refNum;
#endif

#ifdef WIN32
	HANDLE _hFile;
#endif
};


class PlatformOutputFile : public j2k::OutputFile
{
  public:
	PlatformOutputFile(const char *path);
	PlatformOutputFile(const uint16_t *path);
	virtual ~PlatformOutputFile();
	
	virtual WriteFlags Flags() const { return (J2K_WRITE_SEEKABLE | J2K_WRITE_READABLE); }
	
	virtual size_t Read(void *buf, size_t num_bytes);
	virtual size_t Write(const void *buf, size_t num_bytes);
	virtual bool Seek(size_t position);
	virtual size_t Tell();

  private:
#if defined(__APPLE__) || defined(macintosh)
	FSRef _fsRef;
	FSIORefNum _refNum;
#endif

#ifdef WIN32
	HANDLE _hFile;
#endif
};


#endif // J2K_PLATFORM_IO_H
