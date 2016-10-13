/*
 *  j2k_rgba_file.h
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/12/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */


#ifndef J2K_RGBA_FILE_H
#define J2K_RGBA_FILE_H

#include "j2k_codec.h"

namespace j2k
{

typedef struct
{
	Channel r;
	Channel g;
	Channel b;
	Channel a;
	
} RGBAbuffer;


class RGBAinputFile
{
  public:
	RGBAinputFile(InputFile &file, Codec *codec = NULL);
	~RGBAinputFile();
	
	const FileInfo & GetFileInfo() const { return _fileInfo; }
	
	void ReadFile(RGBAbuffer &buffer, unsigned int subsample = 0);
	
  private:
	InputFile &_file;
	Codec *_codec;
	
	FileInfo _fileInfo;
};


class RGBAoutputFile
{
  public:
	RGBAoutputFile(OutputFile &file, const FileInfo &info, Codec *codec = NULL);
	~RGBAoutputFile() {}
	
	void WriteFile(RGBAbuffer &buffer);
	
  private:
	OutputFile &_file;
	Codec *_codec;
	
	FileInfo _fileInfo;
};


}; // namespace j2k

#endif // J2K_RGBA_FILE_H

