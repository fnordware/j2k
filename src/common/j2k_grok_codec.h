/*
 *  j2k_grok_codec.h
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/12/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#ifndef J2K_GROK_CODEC_H
#define J2K_GROK_CODEC_H

#include "j2k_codec.h"


namespace j2k
{


class GrokCodec : public Codec
{
  public:
	GrokCodec() {}
	virtual ~GrokCodec() {}
	
	virtual const char * Name() const { return "Grok"; }
	virtual const char * FourCharCode() const { return "grok"; }
	
	virtual ReadFlags GetReadFlags() { return (J2K_CAN_READ | J2K_CAN_SUBSAMPLE); }
	virtual WriteFlags GetWriteFlags() { return (J2K_CAN_WRITE); }
	
	virtual bool Verify(InputFile &file);
	virtual void GetFileInfo(InputFile &file, FileInfo &info);
	virtual void ReadFile(InputFile &file, const Buffer &buffer, unsigned int subsample = 0);
	
	virtual void WriteFile(OutputFile &file, const FileInfo &info, const Buffer &buffer);
};


}; // namespace j2k

#endif // J2K_GROK_CODEC_H