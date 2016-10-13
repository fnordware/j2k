/*
 *  j2k_platform_io.h
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/9/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

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
