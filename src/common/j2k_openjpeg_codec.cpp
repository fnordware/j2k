/*
 *  j2k_openjpeg_codec.cpp
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/13/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#include "j2k_openjpeg_codec.h"

#include "j2k_exception.h"

#include "openjpeg.h"

#include <assert.h>

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


#pragma mark-


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
			
			
			opj_image_t *image = NULL;
			
			const OPJ_BOOL headerRead = opj_read_header(stream, codec, &image);
			
			if(headerRead && image != NULL)
			{
				info.format = (format == OPJ_CODEC_JP2 ? JP2 : J2C);
			
				info.width = image->x1;
				info.height = image->y1;
				
				info.channels = image->numcomps;
				
				info.colorSpace = (image->color_space == OPJ_CLRSPC_SRGB ? sRGB :
									image->color_space == OPJ_CLRSPC_GRAY ? sLUM :
									image->color_space == OPJ_CLRSPC_SYCC ? sYCC :
									image->color_space == OPJ_CLRSPC_EYCC ? esYCC :
									image->color_space == OPJ_CLRSPC_CMYK ? CMYK :
									UNKNOWN_COLOR_SPACE);
									
				if(image->icc_profile_buf != NULL)
				{
					assert(image->icc_profile_len > 0);
				
					// openjpeg wants me to free the profile myself, and I will!
					info.iccProfile = image->icc_profile_buf;
					
					info.profileLen = image->icc_profile_len;
					
					info.colorSpace = iccRGB;
				}
				
				info.depth = image->comps[0].prec;
				
				assert(image->comps[0].bpp == 0); // unused?
				
				assert(!image->comps[0].sgnd); // not sure I can deal with signed
				
				// TODO: fill in more fields in info, like the compression params
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
OpenJPEGCodec::ReadFile(InputFile &file, const Buffer &buffer, unsigned int subsample)
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
				assert(subsample == 0); // not dealing with subsample yet
				
				// TODO: read one tile at a time and let the user interrupt
				
				imageRead = opj_decode(codec, stream, image);
				
				if(imageRead)
				{
					assert(image->numcomps <= J2K_CODEC_MAX_CHANNELS);
					
					Buffer openjpegBuffer;
					
					openjpegBuffer.channels = image->numcomps;
					
					for(int i=0; i < image->numcomps; i++)
					{
						Channel &chan = openjpegBuffer.channel[i];
						
						const opj_image_comp_t &comp = image->comps[i];
						
						chan.width = comp.w;
						chan.height = comp.h;
						
						chan.sampleType = INT;
						chan.depth = comp.prec;
						chan.sgnd = comp.sgnd;
						
						assert(comp.prec > 0);
						assert(comp.bpp == 0); // unused?
						assert(!comp.sgnd);
						
						chan.buf = (unsigned char *)comp.data;
						chan.colbytes = sizeof(int);
						chan.rowbytes = (sizeof(int) * comp.w);
						
						assert(comp.data != NULL);
					}
					
					CopyBuffer(buffer, openjpegBuffer);
				}
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
OpenJPEGCodec::WriteFile(OutputFile &file, const FileInfo &info, const Buffer &buffer)
{
	assert(file.Tell() == 0);
	
	
	bool success = true;
	
	
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
				param.prec = (chan.sampleType == USHORT ? 16 : 8);
				param.bpp = info.depth;
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
				
				openjpegBuffer.channels = image->numcomps;
				
				for(int i=0; i < image->numcomps; i++)
				{
					Channel &chan = openjpegBuffer.channel[i];
					
					const opj_image_comp_t &comp = image->comps[i];
					
					chan.width = comp.w;
					chan.height = comp.h;
					
					chan.sampleType = INT;
					chan.depth = comp.prec;
					chan.sgnd = comp.sgnd;
					
					assert(comp.prec > 0);
					assert(comp.bpp == comp.prec); // now it's used
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
