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
/*
* This source code incorporates work covered by the following copyright and
* permission notice :
*
* The copyright in this software is being made available under the 2 - clauses
* BSD License, included below.This software may be subject to other third
* party and contributor rights, including patent rights, and no such rights
* are granted under this license.
*
* Copyright(c) 2002 - 2014, Universite catholique de Louvain(UCL), Belgium
* Copyright(c) 2002 - 2014, Professor Benoit Macq
* Copyright(c) 2001 - 2003, David Janssens
* Copyright(c) 2002 - 2003, Yannick Verschueren
* Copyright(c) 2003 - 2007, Francois - Olivier Devaux
* Copyright(c) 2003 - 2014, Antonin Descampe
* Copyright(c) 2005, Herve Drolon, FreeImage Team
* Copyright(c) 2006 - 2007, Parvatha Elangovan
* Copyright(c) 2008, 2011 - 2012, Centre National d'Etudes Spatiales (CNES), FR
* Copyright(c) 2012, CS Systemes d'Information, France
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met :
*1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and / or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/



#include "j2k_openjpeg_codec.h"

#include "j2k_exception.h"

#include "openjpeg.h"

#include <assert.h>
#include <algorithm>


namespace j2k
{

static OPJ_SIZE_T
InputStreamRead(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	InputFile *file = (InputFile *)p_user_data;
	
	return file->Read(p_buffer, p_nb_bytes);
}

static OPJ_OFF_T
InputStreamSkip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	InputFile *file = (InputFile *)p_user_data;
	
	const size_t currentPos = file->Tell();
	
	const size_t newPos = (currentPos + p_nb_bytes);
	
	const bool success = file->Seek(newPos);
	
	if(success)
		return p_nb_bytes;
	else
		return -1;
}

static OPJ_BOOL
InputStreamSeek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	InputFile *file = (InputFile *)p_user_data;
	
	return file->Seek(p_nb_bytes);
}

static OPJ_SIZE_T
OutputStreamRead(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	OutputFile *file = (OutputFile *)p_user_data;
	
	return file->Read(p_buffer, p_nb_bytes);
}

static OPJ_SIZE_T
OutputStreamWrite(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	OutputFile *file = (OutputFile *)p_user_data;
	
	return file->Write(p_buffer, p_nb_bytes);
}

static OPJ_OFF_T
OutputStreamSkip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	OutputFile *file = (OutputFile *)p_user_data;
	
	const size_t currentPos = file->Tell();
	
	const size_t newPos = (currentPos + p_nb_bytes);
	
	const bool success = file->Seek(newPos);
	
	if(success)
		return p_nb_bytes;
	else
		return -1;
}

static OPJ_BOOL
OutputStreamSeek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	OutputFile *file = (OutputFile *)p_user_data;
	
	return file->Seek(p_nb_bytes);
}


static void
ErrorHandler(const char *msg, void *client_data)
{
	printf("openjpeg error: %s", msg);
}

static void
WarningHandler(const char *msg, void *client_data)
{
	printf("openjpeg warning: %s", msg);
}

static void
InfoHandler(const char *msg, void *client_data)
{
	printf("openjpeg info: %s", msg);
}

#ifdef __APPLE__
#pragma mark-
#endif


static OPJ_CODEC_FORMAT
GetFormat(InputFile &file)
{
	// taken from obj_decompress.c
#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"
	
	unsigned char buf[12];
	
	memset(buf, 0, 12);
	
	file.Seek(0);
	
	const size_t read = file.Read(buf, 12);
	
	if(read == 12)
	{
		if(memcmp(buf, JP2_RFC3745_MAGIC, 12) == 0 || memcmp(buf, JP2_MAGIC, 4) == 0)
		{
			return OPJ_CODEC_JP2;
		}
		else if(memcmp(buf, J2K_CODESTREAM_MAGIC, 4) == 0)
		{
			return OPJ_CODEC_J2K;
		}
	}
	
	return OPJ_CODEC_UNKNOWN;
}


bool
OpenJPEGCodec::Verify(InputFile &file)
{
	const OPJ_CODEC_FORMAT format = GetFormat(file);
	
	return (format != OPJ_CODEC_UNKNOWN);
}


void
OpenJPEGCodec::GetFileInfo(InputFile &file, FileInfo &info)
{
	const OPJ_CODEC_FORMAT format = GetFormat(file);
	
	if(format == OPJ_CODEC_UNKNOWN)
		throw Exception("Can't read this format");
		
	file.Seek(0);
	
	
	bool success = true;
	

	opj_stream_t *stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_TRUE);
	
	if(stream)
	{
		opj_stream_set_user_data(stream, &file, NULL);
		opj_stream_set_user_data_length(stream, file.FileSize());
		opj_stream_set_read_function(stream, InputStreamRead);
		opj_stream_set_skip_function(stream, InputStreamSkip);
		opj_stream_set_seek_function(stream, InputStreamSeek);
		
		
		opj_codec_t *codec = opj_create_decompress(format);
		
		if(codec)
		{
			opj_set_error_handler(codec, ErrorHandler, NULL);
			opj_set_warning_handler(codec, WarningHandler, NULL);
			opj_set_info_handler(codec, InfoHandler, NULL);
			
			
		#define AARONs_OPENJPEG_PATCH 1
			
			opj_image_t *image = NULL;
			
		#ifdef AARONs_OPENJPEG_PATCH
			opj_header_info_t header_info;
			
			memset(&header_info, 0, sizeof(header_info));
			
			const OPJ_BOOL headerRead = opj_read_header_ex(stream, codec, &header_info, &image);
		#else
			const OPJ_BOOL headerRead = opj_read_header(stream, codec, &image);
		#endif
			
			if(headerRead && image != NULL)
			{
				info.format = (format == OPJ_CODEC_JP2 ? JP2 : J2C);
			
				info.width = (image->x1 - image->x0);
				info.height = (image->y1 - image->y0);
				
				//assert(image->x0 == 0 && image->x0 == 0); // not really handling this right now
				
				info.channels = std::min<uint8_t>(static_cast<uint8_t>(image->numcomps), J2K_CODEC_MAX_CHANNELS);
				
				info.depth = static_cast<uint8_t>(image->comps[0].prec);
				
				
				for(OPJ_UINT32 i=0U; i < info.channels; i++)
				{
					const opj_image_comp_t &comp = image->comps[i];
					
					Subsampling &sub = info.subsampling[i];
					
					//assert(comp.x0 == 0 && comp.y0 == 0); // not handling
					
					assert(comp.bpp == 0); // unused?
					
					assert(comp.factor == 0); // subsampling?
					
					sub.x = comp.dx;
					sub.y = comp.dy;
				}
				
				assert(image->color_space == OPJ_CLRSPC_UNSPECIFIED); // only read by opj_decode()
			
				assert(image->icc_profile_buf == NULL);
				
			#ifdef AARONs_OPENJPEG_PATCH
				enum
				{
					CS_CMYK = 12,
					CS_sRGB = 16,
					CS_GRAY = 17,
					CS_sYCC = 18,
					CS_esYCC = 19
				};
				
				info.colorSpace = (header_info.enumcs == CS_CMYK ? CMYK :
									header_info.enumcs == CS_sRGB ? sRGB :
									header_info.enumcs == CS_GRAY ? sLUM :
									header_info.enumcs == CS_sYCC ? sYCC :
									header_info.enumcs == CS_esYCC ? esYCC :
									UNKNOWN_COLOR_SPACE);
				
				
				if(header_info.color.icc_profile_buf != NULL)
				{
					assert(header_info.color.icc_profile_len > 0);
				
					// make my own copy
					info.iccProfile = malloc(header_info.color.icc_profile_len);
					
					if(info.iccProfile == NULL)
						throw Exception("out of memory");
					
					info.profileLen = header_info.color.icc_profile_len;
					
					memcpy(info.iccProfile, header_info.color.icc_profile_buf, info.profileLen);
					
					info.colorSpace = (image->numcomps >= 3 ? iccRGB :
										image->numcomps == 1 ? iccLUM :
										iccANY);
				}
				
				
				info.settings.reversible = (header_info.irreversible == 0);
				
				if(header_info.color.jp2_cdef != NULL)
				{
					const opj_jp2_cdef_t &cdef = *header_info.color.jp2_cdef;
				
					assert(cdef.n == image->numcomps);
					
					for(int i=0; i < cdef.n; i++)
					{
						const opj_jp2_cdef_info_t &chan = cdef.info[i];
						
						const ChannelName name = (i == 0 ? RED :
													i == 1 ? GREEN :
													i == 2 ? BLUE :
													ALPHA);
						
						info.channelMap[chan.cn] = name;
					}
				}
				
				if(header_info.color.jp2_pclr != NULL)
				{
					assert(image->numcomps == 1);
					
					const opj_jp2_pclr_t &pal = *header_info.color.jp2_pclr;
					
					assert(pal.nr_entries <= J2K_CODEC_MAX_LUT_ENTRIES);
					assert(pal.nr_channels <= J2K_CODEC_MAX_CHANNELS);
					
					info.LUTsize = pal.nr_entries;
					
					const uint32_t *entry = pal.entries;
					const uint8_t *sign = pal.channel_sign;
					const uint8_t *size = pal.channel_size;
					
					for(int i=0; i < pal.nr_entries; i++)
					{
						for(int c=0; c < pal.nr_channels; c++)
						{
							info.LUT[i].channel[c] = static_cast<uint8_t>(*entry);
							
							assert(*entry < 256);
							//assert(*sign == 0);
							//assert(*size == 8);
							
							entry++;
							sign++;
							size++;
						}
					}
					
					if(pal.cmap != NULL)
					{
						assert(pal.nr_channels == 3);
						
						for(int i=0; i < pal.nr_channels; i++)
						{
							const opj_jp2_cmap_comp_t &chan = pal.cmap[i];
							
							info.LUTmap[i] = info.channelMap[chan.pcol];
							
							assert(chan.cmp == 0);
							assert(chan.mtyp == 1);
						}
					}
				}
			#endif // AARONs_OPENJPEG_PATCH
			}
			else
				success = false;
			
			if(image)
				opj_image_destroy(image);
			
			opj_destroy_codec(codec);
		}
		else
			success = false;
		
		opj_stream_destroy(stream);
	}
	else
		success = false;
	
	
	if(!success)
		throw Exception("Error reading file");
}


#if !defined(__GNUC__)
static int log2(int input)
{
	int result = 0;

	for(int i=1; i<100; i++)
	{
		if(pow(2.f, i) > input)
		{
			result = i-1;
			break;
		}
	}

	return result;
}
#endif


void
OpenJPEGCodec::ReadFile(InputFile &file, const Buffer &buffer, unsigned int subsample, Progress *progress)
{
	const OPJ_CODEC_FORMAT format = GetFormat(file);
	
	if(format == OPJ_CODEC_UNKNOWN)
		throw Exception("Can't read this format");
		
	file.Seek(0);
	

	bool success = true;
	

	opj_stream_t *stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_TRUE);
	
	if(stream)
	{
		opj_stream_set_user_data(stream, &file, NULL);
		opj_stream_set_user_data_length(stream, file.FileSize());
		opj_stream_set_read_function(stream, InputStreamRead);
		opj_stream_set_skip_function(stream, InputStreamSkip);
		opj_stream_set_seek_function(stream, InputStreamSeek);
		
		
		opj_codec_t *codec = opj_create_decompress(format);
		
		if(codec)
		{
			opj_set_error_handler(codec, ErrorHandler, NULL);
			opj_set_warning_handler(codec, WarningHandler, NULL);
			opj_set_info_handler(codec, InfoHandler, NULL);
			
			opj_codec_set_threads(codec, NumberOfCPUs());
			
			
			opj_image_t *image = NULL;
			
			OPJ_BOOL imageRead = opj_read_header(stream, codec, &image);
			
			if(imageRead && image != NULL)
			{
				opj_dparameters_t params;
				opj_set_default_decoder_parameters(&params);
				
				// When you cp_reduce, the buffer doesn't change size, but the image is shrunk
				// into the upper left hand corner.  This means the OpenJPEG buffer doesn't match the
				// buffer we provide, but it works out because CopyBuffer is based on the destination
				// size.
				
				params.cp_reduce = log2(subsample);
				
				params.flags |= OPJ_DPARAMETERS_IGNORE_PALETTE_FLAG; // don't apply LUT if you happen to have one
				
				const OPJ_BOOL configured = opj_setup_decoder(codec, &params);
				
				assert(configured);
			
				
				// TODO: read one tile at a time and let the user interrupt
				
				imageRead = opj_decode(codec, stream, image);
			
			
			#define PROG(COUNT, TOTAL) (progress == NULL ? true : \
											!progress->keepGoing ? false : \
											progress->progressProc != NULL ? \
												(progress->keepGoing = progress->progressProc(progress->refCon, COUNT, TOTAL)) : \
												progress->abortProc != NULL ? \
													(progress->keepGoing = progress->abortProc(progress->refCon)) : \
													true)
				
			#define NOABORT() (progress == NULL ? true : \
								!progress->keepGoing ? false : \
								progress->abortProc == NULL ? true : \
									(progress->keepGoing = progress->abortProc(progress->refCon)))
				
				if(imageRead && NOABORT())
				{
					const uint8_t channels = std::min<uint8_t>(static_cast<uint8_t>(image->numcomps), J2K_CODEC_MAX_CHANNELS);
					
					Buffer openjpegBuffer;
					
					openjpegBuffer.channels = channels;
					
					for(OPJ_UINT32 i=0U; i < channels; i++)
					{
						Channel &chan = openjpegBuffer.channel[i];
						
						const opj_image_comp_t &comp = image->comps[i];
						
						chan.width = comp.w;
						chan.height = comp.h;
						
						chan.subsampling.x = comp.dx;
						chan.subsampling.y = comp.dy;
						
						chan.sampleType = INT;
						chan.depth = static_cast<uint8_t>(comp.prec);
						chan.sgnd = comp.sgnd ? true : false;
						
						assert(comp.prec > 0);
						assert(comp.bpp == 0); // unused?
						
						chan.buf = (unsigned char *)comp.data;
						chan.colbytes = sizeof(int);
						chan.rowbytes = (sizeof(int) * comp.w);
						
						assert(comp.data != NULL);
					}
					
					CopyBuffer(buffer, openjpegBuffer);
				}
				else if(!imageRead)
					success = false;
			}
			else
				success = false;
			
			if(image)
				opj_image_destroy(image);
			
			opj_destroy_codec(codec);
		}
		else
			success = false;
		
		opj_stream_destroy(stream);
	}
	else
		success = false;
	
	
	if(!success)
		throw Exception("Error reading file");
}


void
OpenJPEGCodec::WriteFile(OutputFile &file, const FileInfo &info, const Buffer &buffer, Progress *progress)
{
	assert(file.Tell() == 0);
	
	
	OPJ_BOOL success = OPJ_TRUE;
	
	
	opj_stream_t *stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_FALSE);
	
	if(stream)
	{
		opj_stream_set_user_data(stream, &file, NULL);
		opj_stream_set_read_function(stream, OutputStreamRead);
		opj_stream_set_write_function(stream, OutputStreamWrite);
		opj_stream_set_skip_function(stream, OutputStreamSkip);
		opj_stream_set_seek_function(stream, OutputStreamSeek);
		
		
		// TODO: enable JP2
		// Only writing J2K format right now because JP2 is trying to skip past the end of
		// the file.  Is that really a good idea?  What does fseek() do?
		
		//const OPJ_CODEC_FORMAT format = (info.format == J2C ? OPJ_CODEC_J2K : OPJ_CODEC_JP2);
		const OPJ_CODEC_FORMAT format = OPJ_CODEC_J2K;

		opj_codec_t *codec = opj_create_compress(format);
		
		if(codec)
		{
			opj_set_error_handler(codec, ErrorHandler, NULL);
			opj_set_warning_handler(codec, WarningHandler, NULL);
			opj_set_info_handler(codec, InfoHandler, NULL);
			
			//opj_codec_set_threads(codec, NumberOfCPUs());
			
			
			opj_image_cmptparm_t compParam[J2K_CODEC_MAX_CHANNELS];
			
			assert(info.channels == buffer.channels);
			
			for(int i=0; i < buffer.channels; i++)
			{
				const Channel &chan = buffer.channel[i];
				
				opj_image_cmptparm_t &param = compParam[i];
				
				assert(chan.width == info.width && chan.height == info.height);
				
				param.dx = 1;
				param.dy = 1;
				param.w = chan.width;
				param.h = chan.height;
				param.x0 = 0;
				param.y0 = 0;
				param.prec = info.depth;
				param.bpp = (chan.sampleType == USHORT ? 16 : 8);
				param.sgnd = OPJ_FALSE;
			}
			
			const OPJ_COLOR_SPACE colorSpace = (info.colorSpace == sRGB ? OPJ_CLRSPC_SRGB :
													info.colorSpace == sLUM ? OPJ_CLRSPC_GRAY :
													info.colorSpace == sYCC ? OPJ_CLRSPC_SYCC :
													//info.colorSpace == esRGB ? JP2_esRGB_SPACE :
													info.colorSpace == esYCC ? OPJ_CLRSPC_EYCC :
													//info.colorSpace == ROMM ? JP2_ROMMRGB_SPACE :
													info.colorSpace == CMYK ? OPJ_CLRSPC_CMYK :
													//info.colorSpace == CIELab ? JP2_CIELab_SPACE :
													//info.colorSpace == iccLUM ? JP2_iccLUM_SPACE :
													//info.colorSpace == iccRGB ? JP2_iccRGB_SPACE :
													//info.colorSpace == iccANY ? JP2_iccANY_SPACE :
													OPJ_CLRSPC_UNSPECIFIED);
			
			opj_image_t *image = opj_image_create(buffer.channels, compParam, colorSpace);
			
			if(image)
			{
				image->x0 = 0;
				image->y0 = 0;
				image->x1 = info.width;
				image->y1 = info.height;
				
				Buffer openjpegBuffer;
				
				openjpegBuffer.channels = static_cast<uint8_t>(image->numcomps);
				
				for(OPJ_UINT32 i=0U; i < image->numcomps; i++)
				{
					Channel &chan = openjpegBuffer.channel[i];
					
					const opj_image_comp_t &comp = image->comps[i];
					
					chan.width = comp.w;
					chan.height = comp.h;
					
					chan.sampleType = INT;
					chan.depth = static_cast<uint8_t>(comp.prec);
					chan.sgnd = comp.sgnd ? true : false;
					
					assert(comp.prec == info.depth);
					assert(comp.bpp == (buffer.channel[i].sampleType == USHORT ? 16 : 8));
					assert(!comp.sgnd);
					
					chan.buf = (unsigned char *)comp.data;
					chan.colbytes = sizeof(int);
					chan.rowbytes = (sizeof(int) * comp.w);
					
					assert(comp.data != NULL);
				}
				
				CopyBuffer(openjpegBuffer, buffer);
				
			
				opj_cparameters_t params;
				
				opj_set_default_encoder_parameters(&params);
				
				// TODO: copy more settings from info to here
				params.tcp_numlayers = info.settings.layers;
				params.cp_disto_alloc = OPJ_TRUE;
				
				
				if(info.settings.tileSize > 0)
				{
					params.tile_size_on = OPJ_TRUE;
					params.cp_tx0 = 0;
					params.cp_ty0 = 0;
					params.cp_tdx = info.settings.tileSize;
					params.cp_tdy = info.settings.tileSize;
				}
				
				
				success = opj_setup_encoder(codec, &params, image);
				
				if(success)
				{
					success = opj_start_compress(codec, image, stream);
					
					if(success)
					{
						success = opj_encode(codec, stream);
						
						if(success)
						{
							success = opj_end_compress(codec, stream);
						}
					}
				}
				
			
				opj_image_destroy(image);
			}
			else
				success = false;
			
			opj_destroy_codec(codec);
		}
		else
			success = false;
		
		opj_stream_destroy(stream);
	}
	else
		success = false;
	
	
	if(!success)
		throw Exception("Error writing file");
}


}; // namespace j2k
