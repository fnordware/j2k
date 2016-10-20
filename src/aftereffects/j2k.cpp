
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

#include "j2k.h"

#include "j2k_rgba_file.h"
#include "j2k_platform_io.h"

#include "j2k_OutUI.h"


#include <iostream>

#include <assert.h>
//#include <math.h>

#include "lcms2.h"


static bool A_BooleanToBool(A_Boolean boolean) { return boolean ? true : false; }

//#ifdef MAC_ENV
//	#include <mach/mach.h>
//#endif


extern AEGP_PluginID			S_mem_id;


#ifdef WIN_ENV
static HINSTANCE hDllInstance = NULL;
#endif

/*
#if !defined(macintosh) && !defined(__GNUC__)
int log2(int input)
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
*/

A_Err
j2k_PluginName(char *name)
{
	// just copy back the name
	A_Err err = A_Err_NONE;
	
	strcpy(name, PLUGIN_NAME);
	
	return err;
}


A_Err
j2k_Init(struct SPBasicSuite *pica_basicP)
{
/*
#ifdef MAC_ENV
	// get number of CPUs using Mach calls
	host_basic_info_data_t hostInfo;
	mach_msg_type_number_t infoCount;
	
	infoCount = HOST_BASIC_INFO_COUNT;
	host_info(mach_host_self(), HOST_BASIC_INFO, 
			  (host_info_t)&hostInfo, &infoCount);
	
	g_num_cpus = hostInfo.avail_cpus;
#else // WIN_ENV
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	g_num_cpus = systemInfo.dwNumberOfProcessors;
#endif
*/
	
	return A_Err_NONE;
}


A_Err
j2k_ConstructModuleInfo(
	AEIO_ModuleInfo	*info)
{
	// tell AE all about our plug-in

	A_Err err = A_Err_NONE;
	
	if (info)
	{
		info->sig						=	'jp2k';
		info->max_width					=	2147483647;
		info->max_height				=	2147483647;
		info->num_filetypes				=	1;
		info->num_extensions			=	5;
		info->num_clips					=	0;
		
		info->create_kind.type			=	'jp2 ';
		info->create_kind.creator		=	'    ';

		info->create_ext.pad			=	'.';
		info->create_ext.extension[0]	=	'j';
		info->create_ext.extension[1]	=	'p';
		info->create_ext.extension[2]	=	'x';
		//info->create_ext.extension[3]	=	'\0';
		

		strcpy(info->name, PLUGIN_NAME);
		
		info->num_aux_extensionsS		=	0;

		info->flags						=	AEIO_MFlag_INPUT					| 
											AEIO_MFlag_OUTPUT					| 
											AEIO_MFlag_FILE						|
											AEIO_MFlag_STILL					| 
											AEIO_MFlag_NO_TIME					| 
											AEIO_MFlag_HOST_FRAME_START_DIALOG	|
											AEIO_MFlag_CAN_DRAW_DEEP;
											
		info->flags2 =						AEIO_MFlag2_SUPPORTS_ICC_PROFILES;

		info->read_kinds[0].mac.type			=	'    ';
		info->read_kinds[0].mac.creator			=	AEIO_ANY_CREATOR;
		
		info->read_kinds[1].ext					=	info->create_ext; // .jpx
		
		info->read_kinds[2].ext.pad				=	'.';
		info->read_kinds[2].ext.extension[0]	=	'j';
		info->read_kinds[2].ext.extension[1]	=	'2';
		info->read_kinds[2].ext.extension[2]	=	'c';
		
		info->read_kinds[3].ext.pad				=	'.';
		info->read_kinds[3].ext.extension[0]	=	'j';
		info->read_kinds[3].ext.extension[1]	=	'p';
		info->read_kinds[3].ext.extension[2]	=	'2';

		info->read_kinds[4].ext.pad				=	'.';
		info->read_kinds[4].ext.extension[0]	=	'j';
		info->read_kinds[4].ext.extension[1]	=	'2';
		info->read_kinds[4].ext.extension[2]	=	'k';

		info->read_kinds[5].ext.pad				=	'.'; // preferred jpx extension
		info->read_kinds[5].ext.extension[0]	=	'j';
		info->read_kinds[5].ext.extension[1]	=	'p';
		info->read_kinds[5].ext.extension[2]	=	'f';
	}
	else
	{
		err = A_Err_STRUCT;
	}
	return err;
}


A_Err	
j2k_GetInSpecInfo(
	const A_PathType	*file_pathZ,
	j2k_inData	*options,
	AEIO_Verbiage	*verbiageP)
{ 
	A_Err err			=	A_Err_NONE;
	
	//strcpy(verbiageP->name, "some name"); // AE wil insert file name here for us

	if(options) // hopefully this test will one day fail
		strcpy(verbiageP->type, PLUGIN_NAME " file");
	else
		strcpy(verbiageP->type, PLUGIN_NAME " sequence");
	
	
	if(options)
	{
		std::string sub_type("");
		
		if(options->format == JP2_TYPE_JP2)
			sub_type += "JP2 format";
		else if(options->format == JP2_TYPE_J2C)
			sub_type += "j2c format";
		else if(options->format == JP2_TYPE_JPX)
			sub_type += "JPX format";
			
			
		if(options->bit_depth != 8 && options->bit_depth != 16)
		{
			char str[255];
		
			sprintf(str, ", %dbpc", options->bit_depth);
			
			sub_type += str;
		}
		
		
		if(!options->reversible)
			sub_type += ", float";
			
		if(options->ycc)
			sub_type += ", ycc";
		
		strcpy(verbiageP->sub_type, sub_type.c_str() );
	}
	

	return err;
}


A_Err	
j2k_VerifyFile(
	const A_PathType		*file_pathZ, 
	A_Boolean				*importablePB)
{
	A_Err err			=	A_Err_NONE;

	try
	{
		PlatformInputFile file(file_pathZ);
		
		const j2k::Format format = j2k::Codec::GetFileFormat(file);
		
		*importablePB = (format != j2k::UNKNOWN_FORMAT);
	}
	catch(...)
	{
		*importablePB = FALSE;
	}
	
	return err;
}


A_Err
j2k_FileInfo(
	AEIO_BasicData	*basic_dataP,
	const A_PathType *file_pathZ,
	FrameSeq_Info	*info,
	j2k_inData		*options)
{
	// read a file and pass AE the basic info
	A_Err err = A_Err_NONE;
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	try
	{
		PlatformInputFile input(file_pathZ);
		
		j2k::RGBAinputFile file(input);
		
		const j2k::FileInfo &fileInfo = file.GetFileInfo();
		
		
		info->width = fileInfo.width;
		info->height = fileInfo.height;
		
		info->pixel_aspect_ratio.num = fileInfo.pixelAspect.num;
		info->pixel_aspect_ratio.den = fileInfo.pixelAspect.den;
		
		info->planes = fileInfo.channels + (fileInfo.LUTsize ? 2 : 0);
		info->depth = (fileInfo.depth > 8 ? 16 : 8);
		
		if(fileInfo.alpha == j2k::PREMULTIPLIED || fileInfo.alpha == j2k::STRAIGHT)
		{
			info->alpha_type = (fileInfo.alpha == j2k::PREMULTIPLIED ? AEIO_Alpha_PREMUL : AEIO_Alpha_STRAIGHT);
		}
		
		options->format = (fileInfo.format == j2k::J2C ? JP2_TYPE_J2C :
							fileInfo.format == j2k::JP2 ? JP2_TYPE_JP2 :
							fileInfo.format == j2k::JPX ? JP2_TYPE_JPX :
							JP2_TYPE_UNKNOWN);
		
		options->bit_depth = fileInfo.depth;
		options->reversible = fileInfo.settings.reversible;
		options->ycc = fileInfo.settings.ycc;
		options->has_LUT = !!fileInfo.LUTsize;
		
		
		if(fileInfo.colorSpace == j2k::iccRGB)
		{
			assert(fileInfo.iccProfile != NULL && fileInfo.profileLen > 0);
		
			suites.ColorSettingsSuite()->AEGP_GetNewColorProfileFromICCProfile(S_mem_id,
																				static_cast<A_long>(fileInfo.profileLen),
																				fileInfo.iccProfile,
																				&info->color_profile);
		}
		else if(fileInfo.colorSpace == j2k::sRGB)
		{
			// import sRGB profile
			size_t profileSize;
			
			void *iccProfile = j2k::Codec::CreateProfile(j2k::sRGB, profileSize);
			
			assert(iccProfile != NULL && profileSize > 0);
		
			suites.ColorSettingsSuite()->AEGP_GetNewColorProfileFromICCProfile(S_mem_id,
																				static_cast<A_long>(profileSize),
																				iccProfile,
																				&info->color_profile);
			free(iccProfile);
		}
	}
	catch(...)
	{
		err = AEIO_Err_PARSING;
	}
	
	return err;
}


static j2k::RGBAbuffer
WorldToBuffer(PF_EffectWorld *wP, PF_PixelFormat pixelFormat)
{
	j2k::RGBAbuffer rgbaBuffer;
	
	j2k::Channel *channels[4] = { &rgbaBuffer.a,
									&rgbaBuffer.r,
									&rgbaBuffer.g,
									&rgbaBuffer.b };

	const size_t pixelSize = (pixelFormat == PF_PixelFormat_ARGB32 ? sizeof(A_u_char) :
								pixelFormat == PF_PixelFormat_ARGB64 ? sizeof(A_u_short) :
								pixelFormat == PF_PixelFormat_ARGB128 ? sizeof(PF_FpShort) :
								sizeof(A_u_char));

	const j2k::SampleType sampleType = (pixelFormat == PF_PixelFormat_ARGB32 ? j2k::UCHAR :
										pixelFormat == PF_PixelFormat_ARGB64 ? j2k::USHORT :
										//pixelFormat == PF_PixelFormat_ARGB128 ? j2k::FLOAT :
										j2k::UCHAR);


	for(int i=0; i < 4; i++)
	{
		j2k::Channel &rgbaChan = *channels[i];
		
		rgbaChan.width = wP->width;
		rgbaChan.height = wP->height;
		
		rgbaChan.sampleType = sampleType;
		rgbaChan.depth = static_cast<unsigned char>(pixelSize * 8);
		rgbaChan.sgnd = false;
		
		rgbaChan.buf = (unsigned char *)wP->data + (i * pixelSize);
		rgbaChan.colbytes = (4 * pixelSize);
		rgbaChan.rowbytes = wP->rowbytes;
	}
	
	return rgbaBuffer;
}


A_Err
j2k_DrawSparseFrame(
	AEIO_BasicData					*basic_dataP,
	const AEIO_DrawSparseFramePB	*sparse_framePPB, 
	PF_EffectWorld					*wP,
	AEIO_DrawingFlags				*draw_flagsP,
	const A_PathType				*file_pathZ,
	FrameSeq_Info					*info,
	j2k_inData						*options,
	A_u_char						subsample)
{ 
	// read a file and pass AE the basic info
	A_Err err = A_Err_NONE, err2 = A_Err_NONE;

	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	try
	{
		PlatformInputFile input(file_pathZ);
		
		j2k::RGBAinputFile file(input);
		
		const j2k::FileInfo &fileInfo = file.GetFileInfo();
		
		assert(fileInfo.width == wP->width);
		assert(fileInfo.height == wP->height);
		
		
		PF_PixelFormat	pixelFormat;
		suites.PFWorldSuite()->PF_GetPixelFormat(wP, &pixelFormat);
		
		j2k::RGBAbuffer rgbaBuffer = WorldToBuffer(wP, pixelFormat);
		
		
		file.ReadFile(rgbaBuffer);
		
		
		// ReadFile returns regular 16-bit
		if(pixelFormat == PF_PixelFormat_ARGB64)
			DemoteWorld(basic_dataP, wP);
		
		/*
		// deal with LUT
		if(use_LUT)
			WorldApplyLUT(basic_dataP, wP, jp2_lut);
		
		// deal with channel mapping (bastards!)
		else if( chan_map[0] != 0 || chan_map[1] != 1 || chan_map[2] != 2 )
			WorldChannelMap(basic_dataP, wP, chan_map);

		// do our sYCC conversion
		if(colorSpace == JP2_sYCC_SPACE)
			WorldYCCtoRGB(basic_dataP, wP, FALSE);*/
	}
	catch(...)
	{
		err = AEIO_Err_PARSING;
	}

	
	if(err2) { err = err2; }

	return err;
}


A_Err	
j2k_InitInOptions(
	AEIO_BasicData	*basic_dataP,
	j2k_inData	*options)
{
	// initialize the options when they're first created
	
	A_Err err						=	A_Err_NONE;
	
	// filling in some bogus options here, especially for compression type
	options->version_major = J2K_VERSION_MAJOR;
	options->version_minor = J2K_VERSION_MINOR;
	
	options->format = JP2_TYPE_UNKNOWN;
	options->bit_depth = 0;
	options->reversible = -1;
	options->ycc = FALSE;
	options->has_LUT = FALSE;

	return err;
}


A_Err	
j2k_ReadOptionsDialog(
	AEIO_BasicData	*basic_dataP,
	j2k_inData	*options,
	A_Boolean		*user_interactedPB0)
{
	// if we could have an input dialog, we'd do it here
	// but it's broken for sequences
	basic_dataP->msg_func(0, "IO: Here's my sequence options dialog!");
	
	
	return A_Err_NONE;
}

A_Err
j2k_FlattenInputOptions(
	j2k_inData	*options)
{
	// no swabbing necessary 

	return A_Err_NONE;
}

A_Err
j2k_InflateInputOptions(
	j2k_inData	*options)
{
	// just flip again
	return j2k_FlattenInputOptions(options);
}

//     input
// =====================================================================
//     output

#pragma mark-


A_Err	
j2k_GetDepths(
	AEIO_SupportedDepthFlags		*which)
{
	// these options will be avialable in the depth menu
	A_Err err						=	A_Err_NONE;
	
	*which =	AEIO_SupportedDepthFlags_DEPTH_GRAY_8	|
				AEIO_SupportedDepthFlags_DEPTH_GRAY_16	|
				AEIO_SupportedDepthFlags_DEPTH_24		|
				AEIO_SupportedDepthFlags_DEPTH_32		|
				AEIO_SupportedDepthFlags_DEPTH_48		|
				AEIO_SupportedDepthFlags_DEPTH_64;

	return err;
}


A_Err	
j2k_InitOutOptions(
	j2k_outData	*options)
{
	// initialize the options when they're first created
	// will probably do this only once per AE user per version
	
	A_Err err						=	A_Err_NONE;
	
	options->version_major = J2K_VERSION_MAJOR;
	options->version_minor = J2K_VERSION_MINOR;

	options->format 		= OUT_DEFAULT_FORMAT;
	options->custom_depth	= OUT_DEFAULT_CUSTOM_DEPTH;
	options->bit_depth 		= OUT_DEFAULT_BIT_DEPTH;
	options->reversible 	= OUT_DEFAULT_REVERSIBLE;
	options->method 		= OUT_DEFAULT_METHOD;
	options->quality 		= OUT_DEFAULT_QUALITY;
	options->size 			= OUT_DEFAULT_SIZE;
	options->advanced 		= OUT_DEFAULT_ADVANCED;
	options->ycc 			= OUT_DEFAULT_YCC;
	options->sub			= OUT_DEFAULT_SUB;
	options->layers 		= OUT_DEFAULT_LAYERS;
	options->order 			= OUT_DEFAULT_ORDER;
	options->tile_size		= OUT_DEFAULT_TILE_SIZE;
	options->color_space	= OUT_DEFAULT_COLOR_SPACE;
	options->dci_profile	= OUT_DEFAULT_DCI_PROFILE;
	options->dci_data_rate	= OUT_DEFAULT_DATA_RATE;
	options->dci_per_frame	= OUT_DEFAULT_PER_FRAME;
	options->dci_frame_rate = OUT_DEFAULT_FRAME_RATE;
	options->dci_stereo		= OUT_DEFAULT_STEREO;

	return err;
}


A_Err
j2k_OutputFile(
	AEIO_BasicData		*basic_dataP,
	const A_PathType	*file_pathZ,
	FrameSeq_Info		*info,
	j2k_outData			*options,
	PF_EffectWorld		*wP)
{
	// write da file, mon
	A_Err err = A_Err_NONE;

	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);


	bool advanced = options->advanced ? true : false;
	JPEG_Method method	= options->method;
	
	// make sure we're ok to do DCI, otherwise revert to lossless
	if(method == JP2_METHOD_CINEMA)
	{
		if(info->width > 4096 || info->height > 2160)
		{
			method = JP2_METHOD_LOSSLESS;
			advanced = false;
		}
	}

	
	// resolve advanced parameters
	JPEG_Format	format					= advanced ? options->format : OUT_DEFAULT_FORMAT;
	A_short bit_depth					= static_cast<A_short>((advanced && options->custom_depth) ? options->bit_depth : info->depth);
	A_Boolean reversible				= advanced ? options->reversible : OUT_DEFAULT_REVERSIBLE;
	A_Boolean ycc						= advanced ? options->ycc : OUT_DEFAULT_YCC;
	A_u_char layers						= advanced ? options->layers : OUT_DEFAULT_LAYERS;
	JPEG_Order order					= advanced ? options->order : OUT_DEFAULT_ORDER;
	A_u_short tile_size					= advanced ? options->tile_size : OUT_DEFAULT_TILE_SIZE;
	JPEG_DCI_Profile dci_profile		= advanced ? options->dci_profile: OUT_DEFAULT_DCI_PROFILE;
	A_long dci_data_rate				= advanced ? options->dci_data_rate : OUT_DEFAULT_DATA_RATE;
	JPEG_DCI_Per_Frame dci_per_frame	= advanced ? options->dci_per_frame : OUT_DEFAULT_PER_FRAME;
	A_char dci_frame_rate				= advanced ? options->dci_frame_rate : OUT_DEFAULT_FRAME_RATE;
	A_Boolean dci_stereo				= advanced ? options->dci_stereo : OUT_DEFAULT_STEREO;
	

	const j2k::ColorSpace optionsColorSpace = (options->color_space == JP2_COLOR_sRGB ? j2k::sRGB :
												options->color_space == JP2_COLOR_sYCC ? j2k::sYCC :
												options->color_space == JP2_COLOR_ICC ? j2k::iccRGB :
												j2k::sRGB);
	
	j2k::ColorSpace color_space = advanced ? optionsColorSpace : j2k::iccRGB;
	
	// if we didn't actually get a profile
	if(color_space == j2k::iccRGB && info->color_profile == NULL)
	{
		color_space = j2k::sRGB;
	}
	
	if(info->planes < 3) // greyscale images need profiles too
	{
		ycc = FALSE;
		
		color_space = j2k::sLUM; // we'll just always go with sLUM
		
		/*
		if(color_space == JP2_sRGB_SPACE)
			color_space = JP2_sLUM_SPACE;
		else if(color_space == JP2_iccRGB_SPACE)
			color_space = JP2_iccLUM_SPACE;
		else if(color_space == JP2_sYCC_SPACE) // back out
			color_space = JP2_sLUM_SPACE;
		*/
	}
	
	if(format == JP2_TYPE_J2C)
	{
		color_space = j2k::sRGB; // meaning that no real color space info will get saved
	}
	
	
	if(color_space == j2k::sYCC)
	{
		// for sYCC the codestream thinks it's not YCC (muhaha)
		ycc = FALSE;
	}
	
	
	// subsampling - only in sYCC space
	int x_subsampling = 1, y_subsampling = 1;
	
	if(advanced && (color_space == j2k::sYCC) )
	{
		// if you don't want to subsample, why are you using sYCC?
		x_subsampling = 2, y_subsampling = 2;
		/*
		switch(options->sub)
		{
			case JP2_SUBSAMPLE_422:		x_subsampling = 2, y_subsampling = 1;	break;
			case JP2_SUBSAMPLE_411:		x_subsampling = 4, y_subsampling = 1;	break;
			case JP2_SUBSAMPLE_420:		x_subsampling = 2, y_subsampling = 2;	break;
			case JP2_SUBSAMPLE_311:		x_subsampling = 3, y_subsampling = 1;	break;
			case JP2_SUBSAMPLE_2x2:		x_subsampling = 2, y_subsampling = 2;	break;
			case JP2_SUBSAMPLE_3x3:		x_subsampling = 3, y_subsampling = 3;	break;
			case JP2_SUBSAMPLE_4x4:		x_subsampling = 4, y_subsampling = 4;	break;
		}
		*/
	}


	try
	{
		j2k::FileInfo fileInfo;
		
		fileInfo.width = info->width;
		fileInfo.height = info->height;
		
		fileInfo.channels = static_cast<unsigned char>(info->planes);
		fileInfo.depth = static_cast<unsigned char>(bit_depth);
		
		fileInfo.format = (format == JP2_TYPE_J2C ? j2k::J2C :
							format == JP2_TYPE_JP2 ? j2k::JP2 :
							format == JP2_TYPE_JPX ? j2k::JPX :
							j2k::JPX);
							
		fileInfo.pixelAspect = j2k::Rational(info->pixel_aspect_ratio.num, info->pixel_aspect_ratio.den);
		
		fileInfo.alpha = (info->alpha_type == AEIO_Alpha_STRAIGHT ? j2k::STRAIGHT :
							info->alpha_type == AEIO_Alpha_PREMUL ? j2k::PREMULTIPLIED :
							info->alpha_type == AEIO_Alpha_NONE ? j2k::NO_ALPHA :
							j2k::UNKNOWN_ALPHA);

		if(info->planes < 4)
			fileInfo.alpha = j2k::NO_ALPHA;
		
		
		fileInfo.colorSpace = color_space;
		
		
		AEGP_MemHandle icc_profileH = NULL;
		
		if(fileInfo.colorSpace == j2k::iccRGB)
		{
			assert(info->color_profile != NULL);
		
			suites.ColorSettingsSuite()->AEGP_GetNewICCProfileFromColorProfile(S_mem_id, info->color_profile, &icc_profileH);
			
			if(icc_profileH)
			{
				AEGP_MemSize prof_len;
				void *icc;
				
				suites.MemorySuite()->AEGP_GetMemHandleSize(icc_profileH, &prof_len);
				
				suites.MemorySuite()->AEGP_LockMemHandle(icc_profileH, (void**)&icc);
			
				// is this just an sRGB profile?
				if( j2k::Codec::IssRGBProfile(icc, prof_len) && !advanced)
				{
					fileInfo.colorSpace = j2k::sRGB;
				}
				else
				{
					fileInfo.iccProfile = icc;
					fileInfo.profileLen = prof_len;
				}
				
				// ditch the handle
				suites.MemorySuite()->AEGP_FreeMemHandle(icc_profileH);
			}
			else
				fileInfo.colorSpace = j2k::sRGB;
		}
		
		
		fileInfo.settings.method = (method == JP2_METHOD_LOSSLESS ? j2k::LOSSLESS :
									method == JP2_METHOD_SIZE ? j2k::SIZE :
									method == JP2_METHOD_QUALITY ? j2k::QUALITY :
									method == JP2_METHOD_CINEMA ? j2k::CINEMA :
									j2k::LOSSLESS);
		
		fileInfo.settings.fileSize = options->size;
		fileInfo.settings.quality = options->quality;
		fileInfo.settings.layers = layers;
		
		fileInfo.settings.order = (order == JP2_ORDER_LRCP ? j2k::LRCP :
									order == JP2_ORDER_RLCP ? j2k::RLCP :
									order == JP2_ORDER_RPCL ? j2k::RPCL :
									order == JP2_ORDER_PCRL ? j2k::PCRL :
									order == JP2_ORDER_CPRL ? j2k::CPRL :
									j2k::RPCL);
									
		fileInfo.settings.dciProfile = (dci_profile == JP2_DCI_4K ? j2k::DCI_4K : j2k::DCI_2K);
		
		fileInfo.settings.tileSize = tile_size;
		fileInfo.settings.ycc = A_BooleanToBool(ycc);
		fileInfo.settings.reversible = A_BooleanToBool(reversible);
		
		
		if(fileInfo.settings.method == j2k::CINEMA)
		{
			A_u_long max_per_frame = dci_data_rate;

			if(dci_per_frame == DCI_PER_SECOND)
			{
				max_per_frame = static_cast<A_long>((double)max_per_frame / dci_frame_rate);

				if(dci_stereo)
					max_per_frame /= 2;
			}
			
			fileInfo.settings.fileSize = max_per_frame;
		}
		
		
		PlatformOutputFile output(file_pathZ);
		
		j2k::RGBAoutputFile file(output, fileInfo);
		
		
		PF_PixelFormat	pixel_format;
		suites.PFWorldSuite()->PF_GetPixelFormat(wP, &pixel_format);
		
		
		// do the YCC color conversion (if necessary)
		//if(color_space == j2k::sYCC)
		//	WorldRGBtoYCC(basic_dataP, wP, FALSE); // sYCC always gets irreversible

			
		// we can't be bothered with 15bit+1
		if(pixel_format ==  PF_PixelFormat_ARGB64)
			PromoteWorld(basic_dataP, wP);
			
			
		j2k::RGBAbuffer buffer = WorldToBuffer(wP, pixel_format);
		
		
		file.WriteFile(buffer);
		
		
		// gotta switch it back!
		if(pixel_format ==  PF_PixelFormat_ARGB64)
			DemoteWorld(basic_dataP, wP);
		
		
		// free profile
		if(icc_profileH)
			suites.MemorySuite()->AEGP_FreeMemHandle(icc_profileH);
	}
	catch(...)
	{
		err = AEIO_Err_INAPPROPRIATE_ACTION;
	}
	
	return err;
}


A_Err	
j2k_WriteOptionsDialog(
	AEIO_BasicData		*basic_dataP,
	j2k_outData			*options,
	AEGP_ColorProfileP	color_profile,
	A_Boolean			*user_interactedPB0)
{
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	// get color profile so we can pass description
	const A_char *profile_name = NULL;
	char name_storage[256];
	AEGP_MemHandle icc_profileH = NULL;
	void *icc;
	cmsHPROFILE iccH = NULL;
	char *alt_profile_name = "Embedded Profile";
			
	if(color_profile)
	{
		suites.ColorSettingsSuite()->AEGP_GetNewICCProfileFromColorProfile(S_mem_id, color_profile, &icc_profileH);
		
		if(icc_profileH)
		{
			AEGP_MemSize prof_len;
			
			suites.MemorySuite()->AEGP_GetMemHandleSize(icc_profileH, &prof_len);
			
			suites.MemorySuite()->AEGP_LockMemHandle(icc_profileH, (void**)&icc);
			
			iccH = cmsOpenProfileFromMem(icc, prof_len);
			
			if(iccH)
			{
				cmsUInt32Number namelen = cmsGetProfileInfoASCII(iccH, cmsInfoDescription,
																"en", cmsNoCountry, name_storage, 255);
				
				if(namelen)
					profile_name = name_storage;
				else
					profile_name = alt_profile_name;
			}
		}
	}
	
	// our dialog
	j2k_OutUI_Data params;
	
	params.method			= (DialogMethod)options->method;
	params.size				= options->size;
	params.quality			= options->quality;
	params.advanced			= A_BooleanToBool(options->advanced);
	params.format			= (DialogFormat)options->format;
	params.customDepth		= A_BooleanToBool(options->custom_depth);
	params.bitDepth			= options->bit_depth;
	params.reversible		= A_BooleanToBool(options->reversible);
	params.ycc				= A_BooleanToBool(options->ycc);
	params.order			= (DialogOrder)options->order;
	params.tileSize			= options->tile_size;
	params.icc_profile		= (options->color_space == JP2_COLOR_ICC ? DIALOG_PROFILE_ICC : DIALOG_PROFILE_GENERIC);
	params.dci_profile		= (DialogDCIProfile)options->dci_profile;
	params.dci_data_rate	= options->dci_data_rate;
	params.dci_per_frame	= (DialogDCIPerFrame)options->dci_per_frame;
	params.dci_frame_rate	= options->dci_frame_rate;
	params.dci_stereo		= A_BooleanToBool(options->dci_stereo);
	
#ifdef MAC_ENV
	const char *plugHndl = "com.fnordware.AfterEffects.j2k";
	const void *hwnd = NULL;
#else
	// get platform handles
	const void *plugHndl = hDllInstance;
	HWND hwnd = NULL;
	suites.UtilitySuite()->AEGP_GetMainHWND((void *)&hwnd);
#endif
	
	*user_interactedPB0 = j2k_OutUI(&params, "sRGB", profile_name, false, plugHndl, hwnd);
	
	if(*user_interactedPB0)
	{
		options->method			= params.method;
		options->size			= params.size;
		options->quality		= params.quality;
		options->advanced		= params.advanced;
		options->format			= params.format;
		options->custom_depth	= params.customDepth;
		options->bit_depth		= params.bitDepth;
		options->reversible		= params.reversible;
		options->ycc			= params.ycc;
		options->order			= params.order;
		options->tile_size		= params.tileSize;
		options->color_space	= (params.icc_profile == DIALOG_PROFILE_GENERIC ? JP2_COLOR_sRGB : JP2_COLOR_ICC);
		options->dci_profile	= params.dci_profile;
		options->dci_data_rate	= params.dci_data_rate;
		options->dci_per_frame	= params.dci_per_frame;
		options->dci_frame_rate	= params.dci_frame_rate;
		options->dci_stereo		= params.dci_stereo;
	}
	
	if(iccH)
		cmsCloseProfile(iccH);
	
	if(icc_profileH)
		suites.MemorySuite()->AEGP_FreeMemHandle(icc_profileH);
	
	return A_Err_NONE;
}

A_Err	
j2k_GetOutSpecInfo(
	const A_PathType	*file_pathZ,
	j2k_outData			*options,
	AEIO_Verbiage		*verbiageP)
{ 
	// describe out output options state in English (or another language if you prefer)
	// only sub-type appears to work (but with carriage returs (\r) )
	A_Err err			=	A_Err_NONE;

	// actually, this shows up in the template
	strcpy(verbiageP->type, PLUGIN_NAME);

	if(options)
	{
		std::string sub_type("");
		

		if(options->method != JP2_METHOD_CINEMA)
		{
			// method
			if(options->method == JP2_METHOD_LOSSLESS)
			{
				if(options->advanced && !options->reversible)
					sub_type += "Maximum";
				else
					sub_type += "Lossless";
			}
			else if(options->method == JP2_METHOD_SIZE)
			{
				char buf[64];
				
				sprintf(buf, "Size: %dk", options->size);
				
				sub_type += buf;
			}
			else // quality
			{
				char buf[64];
				
				sprintf(buf, "Quality: %d", options->quality);
				
				sub_type += buf;
			}


			// advanced
			if(options->advanced)
			{
				// next line
				sub_type += "\r";
				

				// format
				if(options->format == JP2_TYPE_J2C)
					sub_type += "j2c";
				else if(options->format == JP2_TYPE_JPX)
					sub_type += "JPX";
				else // JP2
					sub_type += "JP2";
				

				// next line
				sub_type += "\r";
				
				
				// bit depth
				if(options->custom_depth)
				{
					char buf[8];
					
					sprintf(buf, "%dbpc", options->bit_depth);
					
					sub_type += buf;
				}
				else
					sub_type += "AE depth";
				
				// ycc
				if(options->ycc)
					sub_type += ", ycc";
					
				// reversible
				if(!options->reversible)
					sub_type += ", float encoding";
			}
		}
		else
		{
			sub_type = "Digital Cinema";
			
			if(options->advanced)
			{
				sub_type += "\n";
				
				if(options->dci_profile == JP2_DCI_4K)
					sub_type += "Cinema 4k";
				else
					sub_type += "Cinema 2K";
				
				
				char data_buf[128];

				if(options->dci_per_frame == DCI_PER_FRAME)
				{
					sprintf(data_buf, "\n%d KB / frame", options->dci_data_rate);
				}
				else
				{
					sprintf(data_buf, "\n%d Mb / sec, %d fps", options->dci_data_rate * 8 / 1024, options->dci_frame_rate);

					if(options->dci_stereo)
						strcat(data_buf, "\nStereo");
				}

				sub_type += data_buf;
			}
		}
		

		strcpy(verbiageP->sub_type, sub_type.c_str() );
	}
	
	return err;
}


A_Err	
j2k_GetOutputSuffix(
	j2k_outData			*options,
	A_char				*suffix)
{
	A_Err err			=	A_Err_NONE;

	if(options)
	{
		// default suffix
		suffix[0] = '.';
			suffix[1] = 'j';
			suffix[2] = 'p';
			suffix[3] = 'x';
		suffix[4] = '\0';


		// the alternatives
		if(options->advanced && options->method != JP2_METHOD_CINEMA)
		{
			if(options->format == JP2_TYPE_J2C)
			{
				suffix[1] = 'j';
				suffix[2] = '2';
				suffix[3] = 'c';
			}
			
			if(options->format == JP2_TYPE_JP2)
			{
				suffix[1] = 'j';
				suffix[2] = 'p';
				suffix[3] = '2';
			}
		}
		else if(options->method == JP2_METHOD_CINEMA)
		{
			suffix[1] = 'j';
			suffix[2] = '2';
			suffix[3] = 'c';
		}
	}
	else
		err = AEIO_Err_USE_DFLT_CALLBACK;
	
		
	return err;
}

#ifndef SWAP_SHORT
#define SWAP_SHORT(a)		((a >> 8) | ((a & 0xff) << 8))
#endif

#ifndef SWAP_LONG
#define SWAP_LONG(a)		((a >> 24) | ((a >> 8) & 0xff00) | ((a << 8) & 0xff0000) | (a << 24))
#endif

A_Err
j2k_FlattenOutputOptions(
	j2k_outData	*options)
{
	// Swab these values on little endian (Intel) machines
#ifdef AE_LITTLE_ENDIAN
	options->bit_depth	= SWAP_SHORT(options->bit_depth);
	options->size		= SWAP_LONG(options->size);
	options->tile_size	= SWAP_SHORT(options->tile_size);
	options->color_space = SWAP_LONG(options->color_space);
	options->dci_data_rate = SWAP_LONG(options->dci_data_rate);
#endif

	return A_Err_NONE;
}

A_Err
j2k_InflateOutputOptions(
	j2k_outData	*options)
{
	// just flip again
	return j2k_FlattenOutputOptions(options);
}

#ifdef WIN_ENV
BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hDllInstance = (HINSTANCE)hInstance;

	return TRUE;   // Indicate that the DLL was initialized successfully.
}
#endif
