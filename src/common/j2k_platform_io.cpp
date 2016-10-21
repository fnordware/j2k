
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


#include "j2k_platform_io.h"

#include "j2k_exception.h"

#include <assert.h>

using j2k::Exception;

#if defined(__APPLE__) || defined(macintosh)

PlatformInputFile::PlatformInputFile(const char *path) :
	InputFile()
{
	OSErr result = noErr;
	
	CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, path, kCFStringEncodingMacRoman);
	if(inStr == NULL)
		throw Exception("Couldn't make CFStringRef.");
		
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLPOSIXPathStyle, 0);
	CFRelease(inStr);
	if(url == NULL)
		throw Exception("Couldn't make CFURLRef.");
	
	Boolean success = CFURLGetFSRef(url, &_fsRef);
	CFRelease(url);
	if(!success)
		throw Exception("Couldn't make FSRef.");
	
	HFSUniStr255 dataForkName;
	result = FSGetDataForkName(&dataForkName);

	result = FSOpenFork(&_fsRef, dataForkName.length, dataForkName.unicode, fsRdPerm, &_refNum);

	if(result != noErr)
		throw Exception("Couldn't open file for reading.");
}


PlatformInputFile::PlatformInputFile(const uint16_t *path) :
	InputFile()
{
	OSErr result = noErr;
	
	int len = 0;
	while(path[len++] != 0);

	CFStringRef inStr = CFStringCreateWithCharacters(kCFAllocatorDefault, path, len);
	if(inStr == NULL)
		throw Exception("Couldn't make CFStringRef.");
		
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLPOSIXPathStyle, 0);
	CFRelease(inStr);
	if(url == NULL)
		throw Exception("Couldn't make CFURLRef.");
	
	Boolean success = CFURLGetFSRef(url, &_fsRef);
	CFRelease(url);
	if(!success)
		throw Exception("Couldn't make FSRef.");
	
	HFSUniStr255 dataForkName;
	result = FSGetDataForkName(&dataForkName);

	result = FSOpenFork(&_fsRef, dataForkName.length, dataForkName.unicode, fsRdPerm, &_refNum);

	if(result != noErr)
		throw Exception("Couldn't open file for reading.");
}


PlatformInputFile::~PlatformInputFile()
{
	if(_refNum)
	{
		OSErr result = FSCloseFork(_refNum);
		_refNum = 0;

		assert(result == noErr);
	}
}


size_t
PlatformInputFile::FileSize()
{
	SInt64 fork_size = 0;
	
	OSErr result = FSGetForkSize(_refNum, &fork_size);
	
	if(result != noErr)
		throw Exception("Error calling FSGetForkSize().");
	
	return fork_size;
}


size_t
PlatformInputFile::Read(void *buf, size_t num_bytes)
{
	ByteCount count = num_bytes;
	
	OSErr result = FSReadFork(_refNum, fsAtMark, 0, count, (void *)buf, &count);
	
	assert(result == noErr || count != num_bytes);
	
	return count;
}


bool
PlatformInputFile::Seek(size_t position)
{
	OSErr result = FSSetForkPosition(_refNum, fsFromStart, position);

	return (result == noErr);
}


size_t
PlatformInputFile::Tell()
{
	SInt64 lpos;

	OSErr result = FSGetForkPosition(_refNum, &lpos);
	
	assert(result == noErr);
	
	return lpos;
}


PlatformOutputFile::PlatformOutputFile(const char *path) :
	OutputFile()
{
	OSErr result = noErr;
	
	CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, path, kCFStringEncodingMacRoman);
	if(inStr == NULL)
		throw Exception("Couldn't make CFStringRef.");
		
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLPOSIXPathStyle, 0);
	CFRelease(inStr);
	if(url == NULL)
		throw Exception("Couldn't make CFURLRef.");
	
	Boolean file_exists = CFURLGetFSRef(url, &_fsRef);
	CFRelease(url);
	
	if(!file_exists)
	{
		// means the file doesn't exist and we have to create it
		
		// find the last slash, splitting the directory from the file name
		int dir_name_len = 0;
		const char *file_name = NULL;
		int file_name_len = 0;
		
		int len = strlen(path);
		
		for(int i = (len - 2); i >= 0 && file_name == NULL; i--)
		{
			if(path[i] == '/')
			{
				dir_name_len = i;
				file_name = &path[i + 1];
				file_name_len = (len - i) - 1;
			}
		}
		
		if(file_name == NULL)
			throw Exception("Error parsing path.");
		
		CFURLRef dir_url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)path, dir_name_len, TRUE);
		
		FSRef parent_fsRef;
		Boolean dir_success = CFURLGetFSRef(dir_url, &parent_fsRef);
		CFRelease(dir_url);
		
		if(dir_success)
		{
			UniChar u_path[256];
			
			// poor-man's unicode copy
			UniChar *u = u_path;
			for(int i=0; i < file_name_len; i++)
				*u++ = file_name[i];
			
			FSSpec my_fsSpec;
			result =  FSCreateFileUnicode(&parent_fsRef, file_name_len, u_path, kFSCatInfoNone, NULL, &_fsRef, &my_fsSpec);
			
			if(result != noErr)
				throw Exception("Couldn't create new file.");
		}
		else
			throw Exception("Couldn't make FSRef.");
	}
	
	HFSUniStr255 dataForkName;
	result = FSGetDataForkName(&dataForkName);

	result = FSOpenFork(&_fsRef, dataForkName.length, dataForkName.unicode, fsWrPerm, &_refNum);

	if(result != noErr)
		throw Exception("Couldn't open file for writing.");
}


PlatformOutputFile::PlatformOutputFile(const uint16_t *path) :
	OutputFile()
{
	OSErr result = noErr;
	
	int len = 0;
	while(path[len++] != 0);
	
	CFStringRef inStr = CFStringCreateWithCharacters(kCFAllocatorDefault, path, len);
	if(inStr == NULL)
		throw Exception("Couldn't make CFStringRef.");
		
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLPOSIXPathStyle, 0);
	CFRelease(inStr);
	if(url == NULL)
		throw Exception("Couldn't make CFURLRef.");
	
	Boolean file_exists = CFURLGetFSRef(url, &_fsRef);
	CFRelease(url);
	
	if(!file_exists)
	{
		// find the last slash, splitting the directory from the file name
		int dir_name_len = 0;
		const uint16_t *file_name = NULL;
		int file_name_len = 0;
		
		for(int i = (len - 2); i >= 0 && file_name == NULL; i--)
		{
			if(path[i] == '/')
			{
				dir_name_len = i;
				file_name = &path[i + 1];
				file_name_len = (len - i) - 1;
			}
		}
		
		if(file_name == NULL)
			throw Exception("Error parsing path.");

		CFStringRef dirStr = CFStringCreateWithCharacters(kCFAllocatorDefault, path, dir_name_len);
		if(dirStr == NULL)
			throw Exception("Couldn't make CFStringRef.");

		CFURLRef dir_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, dirStr, kCFURLPOSIXPathStyle, 0);
		CFRelease(dirStr);
		if(dir_url == NULL)
			throw Exception("Couldn't make CFURLRef.");

		FSRef parent_fsRef;
		Boolean dir_success = CFURLGetFSRef(dir_url, &parent_fsRef);
		CFRelease(dir_url);
		
		if(dir_success)
		{
			FSSpec my_fsSpec;
			result =  FSCreateFileUnicode(&parent_fsRef, file_name_len, file_name, kFSCatInfoNone, NULL, &_fsRef, &my_fsSpec);
			
			if(result != noErr)
				throw Exception("Couldn't create new file.");
		}
		else
			throw Exception("Couldn't make FSRef.");
	}
	
	HFSUniStr255 dataForkName;
	result = FSGetDataForkName(&dataForkName);

	result = FSOpenFork(&_fsRef, dataForkName.length, dataForkName.unicode, fsWrPerm, &_refNum);

	if(result != noErr)
		throw Exception("Couldn't open file for writing.");
}


PlatformOutputFile::~PlatformOutputFile()
{
	if(_refNum)
	{
		OSErr result = FSCloseFork(_refNum);
		_refNum = 0;

		assert(result == noErr);
	}
}


size_t
PlatformOutputFile::Read(void *buf, size_t num_bytes)
{
	ByteCount count = num_bytes;
	
	OSErr result = FSReadFork(_refNum, fsAtMark, 0, count, (void *)buf, &count);
	
	assert(result == noErr);
	
	return count;
}


size_t
PlatformOutputFile::Write(const void *buf, size_t num_bytes)
{
	ByteCount count = num_bytes;

	OSErr result = FSWriteFork(_refNum, fsAtMark, 0, count, (const void *)buf, &count);
	
	return (result == noErr ? count : 0);
}


bool
PlatformOutputFile::Seek(size_t position)
{
	OSErr result = FSSetForkPosition(_refNum, fsFromStart, position);

	return (result == noErr);
}


size_t
PlatformOutputFile::Tell()
{
	SInt64 lpos;

	OSErr result = FSGetForkPosition(_refNum, &lpos);
	
	assert(result == noErr);
	
	return lpos;
}


#else


PlatformInputFile::PlatformInputFile(const char *path) :
	InputFile()
{
	_hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(_hFile == INVALID_HANDLE_VALUE)
		throw Exception("Couldn't open file.");
}


PlatformInputFile::PlatformInputFile(const uint16_t *path) :
	InputFile()
{
	_hFile = CreateFileW((LPCWSTR)path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(_hFile == INVALID_HANDLE_VALUE)
		throw Exception("Couldn't open file.");
}


PlatformInputFile::~PlatformInputFile()
{
	BOOL result = CloseHandle(_hFile);

	assert(result);
}


size_t
PlatformInputFile::FileSize()
{
	return GetFileSize(_hFile, NULL);
}


size_t
PlatformInputFile::Read(void *buf, size_t num_bytes)
{
	DWORD count = static_cast<DWORD>(num_bytes), bytes_read = 0;
	
	BOOL result = ReadFile(_hFile, (LPVOID)buf, count, &bytes_read, NULL);

	return bytes_read;
}


bool
PlatformInputFile::Seek(size_t position)
{
	LARGE_INTEGER lpos;

	lpos.QuadPart = position;

	BOOL result = SetFilePointerEx(_hFile, lpos, NULL, FILE_BEGIN);
	
	return result ? true : false;
}


size_t
PlatformInputFile::Tell()
{
	size_t pos;
	LARGE_INTEGER lpos, zero;

	zero.QuadPart = 0;

	BOOL result = SetFilePointerEx(_hFile, zero, &lpos, FILE_CURRENT);

	pos = lpos.QuadPart;
	
	return pos;
}


PlatformOutputFile::PlatformOutputFile(const char *path) :
	OutputFile()
{
	_hFile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if(_hFile == INVALID_HANDLE_VALUE)
		throw Exception("Couldn't open file.");
}


PlatformOutputFile::PlatformOutputFile(const uint16_t *path) :
	OutputFile()
{
	_hFile = CreateFileW((LPCWSTR)path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if(_hFile == INVALID_HANDLE_VALUE)
		throw Exception("Couldn't open file.");
}


PlatformOutputFile::~PlatformOutputFile()
{
	BOOL result = CloseHandle(_hFile);

	assert(result == TRUE);
}


size_t
PlatformOutputFile::Read(void *buf, size_t num_bytes)
{
	DWORD count = static_cast<DWORD>(num_bytes), bytes_read = 0;
	
	BOOL result = ReadFile(_hFile, (LPVOID)buf, count, &bytes_read, NULL);

	return bytes_read;
}


size_t
PlatformOutputFile::Write(const void *buf, size_t num_bytes)
{
	DWORD count = static_cast<DWORD>(num_bytes), out = 0;
	
	BOOL result = WriteFile(_hFile, (LPVOID)buf, count, &out, NULL);
	
	return count;
}


bool
PlatformOutputFile::Seek(size_t position)
{
	LARGE_INTEGER lpos;

	lpos.QuadPart = position;

	BOOL result = SetFilePointerEx(_hFile, lpos, NULL, FILE_BEGIN);
	
	return result ? true : false;
}


size_t
PlatformOutputFile::Tell()
{
	size_t pos;
	LARGE_INTEGER lpos, zero;

	zero.QuadPart = 0;

	BOOL result = SetFilePointerEx(_hFile, zero, &lpos, FILE_CURRENT);

	pos = lpos.QuadPart;
	
	return pos;
}

#endif // defined(__APPLE__) || defined(macintosh)

