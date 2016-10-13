/*
 *  j2k_grok_codec.cpp
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/12/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#include "j2k_grok_codec.h"

#include "j2k_exception.h"

namespace j2k
{


bool
GrokCodec::Verify(InputFile &file)
{
	throw Exception("nothing here!");
}


void
GrokCodec::GetFileInfo(InputFile &file, FileInfo &info)
{
	throw Exception("nothing here!");
}


void
GrokCodec::ReadFile(InputFile &file, const Buffer &buffer, unsigned int subsample)
{
	throw Exception("nothing here!");
}


void
GrokCodec::WriteFile(OutputFile &file, const FileInfo &info, const Buffer &buffer)
{
	throw Exception("nothing here!");
}


}; // namespace j2k