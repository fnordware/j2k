
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


#include "FrameSeq.h"

#include "j2k.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <assert.h>

#ifdef WIN_ENV
#include <Windows.h>
#endif

extern AEGP_PluginID			S_mem_id;


// edit these typedefs for each custom file importer
typedef	j2k_inData		format_inData;
typedef	j2k_outData		format_outData;

#ifndef CONVERT16TO8
#define CONVERT16TO8(A)		( (((long)(A) * PF_MAX_CHAN8) + PF_HALF_CHAN16) / PF_MAX_CHAN16)
#endif

static A_Err
EasyCopy(AEIO_BasicData	*basic_dataP, PF_EffectWorld *src, PF_EffectWorld *dest, PF_Boolean hq)
{
	A_Err err =	A_Err_NONE;
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);

	if(hq)
		err = suites.PFWorldTransformSuite()->copy_hq(NULL, src, dest, NULL, NULL);
	else
		err = suites.PFWorldTransformSuite()->copy(NULL, src, dest, NULL, NULL);
	
	return err;
}

template <typename InFormat, typename OutFormat>
static inline OutFormat Convert(InFormat in);

template <>
static inline PF_FpShort Convert<A_u_char, PF_FpShort>(A_u_char in)
{
    return (PF_FpShort)in / (PF_FpShort)PF_MAX_CHAN8;
}

template <>
static inline A_u_short Convert<A_u_char, A_u_short>(A_u_char in)
{
    return CONVERT8TO16(in);
}


template <>
static inline PF_FpShort Convert<A_u_short, PF_FpShort>(A_u_short in)
{
    return (PF_FpShort)in / (PF_FpShort)PF_MAX_CHAN16;
}

template <>
static inline A_u_char Convert<A_u_short, A_u_char>(A_u_short in)
{
    return CONVERT16TO8(in);
}


static inline PF_FpShort Clamp(PF_FpShort in)
{
    return (in > 1.f ? 1.f : in < 0.f ? 0.f : in);
}

template <>
static inline A_u_char Convert<PF_FpShort, A_u_char>(PF_FpShort in)
{
    return ( Clamp(in) * (PF_FpShort)PF_MAX_CHAN8 ) + 0.5f;
}

template <>
static inline A_u_short Convert<PF_FpShort, A_u_short>(PF_FpShort in)
{
    return ( Clamp(in) * (PF_FpShort)PF_MAX_CHAN16 ) + 0.5f;
}

typedef struct {
	PF_EffectWorld	*src;
	PF_EffectWorld	*dest;
} CopyWorldData;

template <typename InFormat, typename OutFormat>
static A_Err CopyWorldIterate(	void 	*refconPV,
								A_long 	thread_indexL,
								A_long 	i,
								A_long 	iterationsL)
{
	CopyWorldData *c_data = (CopyWorldData *)refconPV;
	
	InFormat *in_pixel = (InFormat *)((char *)c_data->src->data + (i * c_data->src->rowbytes));
	OutFormat *out_pixel = (OutFormat *)((char *)c_data->dest->data + (i * c_data->dest->rowbytes));
	
	for(int x=0; x < c_data->dest->width; x++)
	{
		*out_pixel++ = Convert<InFormat, OutFormat>( *in_pixel++ ); // alpha
		*out_pixel++ = Convert<InFormat, OutFormat>( *in_pixel++ ); // red
		*out_pixel++ = Convert<InFormat, OutFormat>( *in_pixel++ ); // green
		*out_pixel++ = Convert<InFormat, OutFormat>( *in_pixel++ ); // blue
	}
	
	return A_Err_NONE;
}

static A_Err
SmartCopyWorld(
	AEIO_BasicData		*basic_dataP,
	PF_EffectWorld 		*source_World,
	PF_EffectWorld 		*dest_World,
	PF_Boolean 			source_ext,
	PF_Boolean	 		dest_ext, // _ext means 16bpc is true 16-bit (the EXTernal meaning of 16-bit)
	PF_Boolean			source_writeable)
{
	// this is the world copy function AE should have provided
	// with additional attention to external (true) 16-bit worlds
	A_Err err =	A_Err_NONE;
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	PF_PixelFormat	source_format, dest_format;

	suites.PFWorldSuite()->PF_GetPixelFormat(source_World, &source_format);
	suites.PFWorldSuite()->PF_GetPixelFormat(dest_World, &dest_format);
	
	PF_Boolean hq = FALSE;			
	
	// can we just copy?
	if( (source_format == dest_format) && (source_ext == dest_ext) ) // make sure you are always setting ext's
	{
		EasyCopy(basic_dataP, source_World, dest_World, hq);
	}
	else
	{
		// copy to a buffer of the same size, different bit depth
		PF_EffectWorld temp_World_data;
		PF_EffectWorld *temp_World = &temp_World_data;
		
		
		// if out world is the same size, we'll copy directly, otherwise need temp
		if( (source_World->height == dest_World->width) &&
			(source_World->width  == dest_World->width) )
		{
			temp_World = dest_World;
		}
		else
		{
			suites.PFWorldSuite()->PF_NewWorld(NULL, source_World->width, source_World->height,
													FALSE, dest_format, temp_World);
		}
		
		
		CopyWorldData c_data = { source_World, temp_World };
		
		
		if(source_format == PF_PixelFormat_ARGB128)
		{
			if(dest_format == PF_PixelFormat_ARGB64)
			{
				err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(source_World->height,
														(void *)&c_data,
														CopyWorldIterate<PF_FpShort, A_u_short>);
				if(!err && dest_ext)
					err = PromoteWorld(basic_dataP, temp_World);
			}
			else if(dest_format == PF_PixelFormat_ARGB32)
			{
				err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(source_World->height,
														(void *)&c_data,
														CopyWorldIterate<PF_FpShort, A_u_char>);
			}
		}
		else if(source_format == PF_PixelFormat_ARGB64)
		{
			if(source_ext)
				DemoteWorld(basic_dataP, source_World);
		
			if(dest_format == PF_PixelFormat_ARGB128)
			{
				err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(source_World->height,
														(void *)&c_data,
														CopyWorldIterate<A_u_short, PF_FpShort>);
			}
			else if(dest_format == PF_PixelFormat_ARGB64)
			{
				EasyCopy(basic_dataP, source_World, temp_World, hq);
																	
				if(!err && dest_ext)
					err = PromoteWorld(basic_dataP, temp_World);
			}
			else if(dest_format == PF_PixelFormat_ARGB32)
			{
				err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(source_World->height,
														(void *)&c_data,
														CopyWorldIterate<A_u_short, A_u_char>);
			}
			
			if(source_ext)
				PromoteWorld(basic_dataP, source_World);
		}
		else if(source_format == PF_PixelFormat_ARGB32)
		{
			if(dest_format == PF_PixelFormat_ARGB128)
			{
				err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(source_World->height,
														(void *)&c_data,
														CopyWorldIterate<A_u_char, PF_FpShort>);
			}
			else if(dest_format == PF_PixelFormat_ARGB64)
			{
				err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(source_World->height,
														(void *)&c_data,
														CopyWorldIterate<A_u_char, A_u_short>);
														
				if(!err && dest_ext)
					PromoteWorld(basic_dataP, temp_World);
			}
		}

		// copy from temp world if necessary, dispose temp buffer
		if(temp_World != dest_World)
		{
			EasyCopy(basic_dataP, temp_World, dest_World, hq);

			suites.PFWorldSuite()->PF_DisposeWorld(NULL, temp_World);
		}
	}

	return err;
}

static inline A_u_short Demote(A_u_short val)
{
	return (val > PF_MAX_CHAN16 ? ( (val - 1) >> 1 ) + 1 : val >> 1);
}

static A_Err
DemoteWorldIterate(	void 	*refconPV,
					A_long 	thread_indexL,
					A_long 	i,
					A_long 	iterationsL)
{
	PF_EffectWorld *world = (PF_EffectWorld *)refconPV;
	
	A_u_short *pixel = (A_u_short *)((char *)world->data + (i * world->rowbytes));
	
	for(int x=0; x < world->width; x++)
	{
		*pixel = Demote( *pixel ); pixel++; // alpha
		*pixel = Demote( *pixel ); pixel++; // red
		*pixel = Demote( *pixel ); pixel++; // green
		*pixel = Demote( *pixel ); pixel++; // blue
	}
	
	return A_Err_NONE;
}

A_Err
DemoteWorld(AEIO_BasicData *basic_dataP, PF_EffectWorld *world)
{
	// this function takes a true 16-bit world and converts it to 15bit+1

	A_Err err =	A_Err_NONE;
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	PF_PixelFormat	pixel_format;
	suites.PFWorldSuite()->PF_GetPixelFormat(world, &pixel_format);
	assert(pixel_format == PF_PixelFormat_ARGB64);

	err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(world->height,
															(void *)world,
															DemoteWorldIterate);
	
	return err;
}

static inline A_u_short Promote(A_u_short val)
{
	return (val > PF_HALF_CHAN16 ? ( (val - 1) << 1 ) + 1 : val << 1);
}

static A_Err
PromoteWorldIterate(void 	*refconPV,
					A_long 	thread_indexL,
					A_long 	i,
					A_long 	iterationsL)
{
	PF_EffectWorld *world = (PF_EffectWorld *)refconPV;
	
	A_u_short *pixel = (A_u_short *)((char *)world->data + (i * world->rowbytes));
	
	for(int x=0; x < world->width; x++)
	{
		*pixel = Promote( *pixel ); pixel++; // alpha
		*pixel = Promote( *pixel ); pixel++; // red
		*pixel = Promote( *pixel ); pixel++; // green
		*pixel = Promote( *pixel ); pixel++; // blue
	}
	
	return A_Err_NONE;
}

A_Err
PromoteWorld(AEIO_BasicData *basic_dataP, PF_EffectWorld *world)
{
	// convert 15bit+1 to true 16-bit

	A_Err err =	A_Err_NONE;
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	PF_PixelFormat	pixel_format;
	suites.PFWorldSuite()->PF_GetPixelFormat(world, &pixel_format);
	assert(pixel_format == PF_PixelFormat_ARGB64);

	err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(world->height,
															(void *)world,
															PromoteWorldIterate);
	
	return err;
}


template <typename PixelFormat>
static A_Err GreyToRGBWorldIterate(	void 	*refconPV,
					A_long 	thread_indexL,
					A_long 	i,
					A_long 	iterationsL)
{
	PF_EffectWorld *world = (PF_EffectWorld *)refconPV;
	
	PixelFormat *pixel = (PixelFormat *)((char *)world->data + (i * world->rowbytes));
	
	for(int x=0; x < world->width; x++)
	{
		pixel->green = pixel->blue = pixel->red;
		pixel++;
	}
	
	return A_Err_NONE;
}

static A_Err
WorldGreyToRGBA(AEIO_BasicData *basic_dataP, PF_EffectWorld *world)
{
	// here we copy the R value to G and B
	A_Err err =	A_Err_NONE;
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	PF_PixelFormat	pixel_format;
	suites.PFWorldSuite()->PF_GetPixelFormat(world, &pixel_format);
	
	if(pixel_format == PF_PixelFormat_ARGB128)
	{
		err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(world->height,
																(void *)world,
																GreyToRGBWorldIterate<PF_Pixel32>);
	}
	else if(pixel_format == PF_PixelFormat_ARGB64)
	{
		err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(world->height,
																(void *)world,
																GreyToRGBWorldIterate<PF_Pixel16>);
	}
	else if(pixel_format == PF_PixelFormat_ARGB32)
	{
		err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(world->height,
																(void *)world,
																GreyToRGBWorldIterate<PF_Pixel>);
	}
	
	return err;
}


#ifdef AE_HFS_PATHS
// convert from HFS paths (fnord:Users:mrb:) to Unix paths (/Users/mrb/)
static int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen)
{
	CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, inPath ,kCFStringEncodingMacRoman);
	if (inStr == NULL)
		return -1;
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle,0);
	
	if(url)
	{
		CFStringRef outStr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		if (!CFStringGetCString(outStr, outPath, outPathMaxLen, kCFURLPOSIXPathStyle))
			return -1;
		CFRelease(outStr);
		CFRelease(url);
	}
	
	CFRelease(inStr);
	return 0;
}
#endif // AE_HFS_PATHS

// platform-specific calls to get the file size
static void GetFileSize(const A_PathType *path, A_long *size)
{
#ifdef MAC_ENV
	OSStatus status = noErr;
	FSRef fsref;
	
	// expecting a POSIX path here
#ifdef AE_UNICODE_PATHS	
	int len = 0;
	while(path[len++] != 0);
	
	CFStringRef inStr = CFStringCreateWithCharacters(kCFAllocatorDefault, path, len);
	if (inStr == NULL)
		return;
		
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLPOSIXPathStyle, 0);
	CFRelease(inStr);
	if (url == NULL)
		return;
	
	Boolean success = CFURLGetFSRef(url, &fsref);
	CFRelease(url);
	if(!success)
		return;
#else	
	status = FSPathMakeRef((const UInt8 *)path, &fsref, false);	
#endif

	if(status == noErr)
	{
		OSErr result = noErr;

	#if MAC_OS_X_VERSION_MAX_ALLOWED < 1050
		typedef SInt16 FSIORefNum;
	#endif
		
		HFSUniStr255 dataForkName;
		FSIORefNum refNum;
		SInt64 fork_size;
	
		result = FSGetDataForkName(&dataForkName);

		result = FSOpenFork(&fsref, dataForkName.length, dataForkName.unicode, fsRdPerm, &refNum);
		
		result = FSGetForkSize(refNum, &fork_size);
		
		result = FSCloseFork(refNum);
		
		*size = MIN(INT_MAX, fork_size);
	}
#else
#ifdef AE_UNICODE_PATHS
	HANDLE hFile = CreateFileW((LPCWSTR)path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif

	if(hFile)
	{
		*size = GetFileSize(hFile, NULL);

		CloseHandle(hFile);
	}
#endif
}

#pragma mark-

A_Err
FrameSeq_PluginName(char *name)
{
	// we just want to hand back the name
	// this is so that all the format specific info is in the format file
	// ...except the function names and file types - yeah, I know...
	return j2k_PluginName(name);
}


A_Err
FrameSeq_Init(struct SPBasicSuite *pica_basicP)
{
	return j2k_Init(pica_basicP);
}


A_Err
FrameSeq_ConstructModuleInfo(
	AEIO_ModuleInfo	*info)
{
	// tell AE all about our capabilities
	return j2k_ConstructModuleInfo(info);
}

#pragma mark-

A_Err	
FrameSeq_VerifyFileImportable(
	AEIO_BasicData			*basic_dataP,
	AEIO_ModuleSignature	sig, 
	const A_PathType		*file_pathZ, 
	A_Boolean				*importablePB)
{ 
	// quick check to see if file is really in our format
#ifdef AE_HFS_PATHS	
	A_char				pathZ[AEGP_MAX_PATH_SIZE];

	// convert the path format
	if(A_Err_NONE != ConvertPath(file_pathZ, pathZ, AEGP_MAX_PATH_SIZE-1) )
		return AEIO_Err_BAD_FILENAME;
#else
	const A_PathType *pathZ = file_pathZ;
#endif

	return j2k_VerifyFile(pathZ, importablePB);
}		


A_Err	
FrameSeq_GetInSpecInfo(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH, 
	AEIO_Verbiage	*verbiageP)
{ 
	// all this does it print the info (verbiage) about the file in the project window
	// you can also mess with file name and type if you must
	
	A_Err err			=	A_Err_NONE;
	
	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

	A_PathType			file_nameZ[AEGP_MAX_PATH_SIZE] = {'\0'};
	
	AEIO_Handle		optionsH		=	NULL;
	format_inData	*options			=	NULL;


	// get file path (or not - this can cause errors if the file lived on a drive that's been unmounted)
	//err = suites.IOInSuite2()->AEGP_GetInSpecFilePath(specH, file_nameZ);


	// get options handle
	err = suites.IOInSuite()->AEGP_GetInSpecOptionsHandle(specH, (void**)&optionsH);

	if(optionsH)
		err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
		
	
	// initialize the verbiage
	verbiageP->type[0] = '\0';
	verbiageP->sub_type[0] = '\0';
	
	
	if(!err)
	{
		err = j2k_GetInSpecInfo(file_nameZ, options, verbiageP);
		
		// done with options
		if(optionsH)
			suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	}

	return err;
}


A_Err	
FrameSeq_InitInSpecFromFile(
	AEIO_BasicData	*basic_dataP,
	const A_PathType *file_pathZ,
	AEIO_InSpecH	specH)
{ 
	// tell AE all the necessary information
	// we pass our format function a nice, easy struct to populate and then deal with it
	
	A_Err err						=	A_Err_NONE;

	AEIO_Handle		optionsH		=	NULL,
					old_optionsH	=	NULL;
	format_inData	*options			=	NULL;

	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);
	
	A_long			file_size = 0;
	
	// fill out the data structure for our function
	PF_Pixel 	premult_color = {255, 0, 0, 0};
	FIEL_Label	field_label;

	FrameSeq_Info info; // this is what we must tell AE about a frame
	
#ifdef AE_HFS_PATHS	
	A_char		pathZ[AEGP_MAX_PATH_SIZE];

	// convert the path format
	if(A_Err_NONE != ConvertPath(file_pathZ, pathZ, AEGP_MAX_PATH_SIZE-1) )
		return AEIO_Err_BAD_FILENAME;
#else
	const A_PathType *pathZ = file_pathZ;
#endif
	
	// fill in the file size
	GetFileSize(pathZ, &file_size);


	AEFX_CLR_STRUCT(field_label);
	field_label.order		=	FIEL_Order_LOWER_FIRST;
	field_label.type		=	FIEL_Type_UNKNOWN;
	field_label.version		=	FIEL_Label_VERSION;
	field_label.signature	=	FIEL_Tag;
	
	
	info.width			= 0;
	info.height			= 0;
	info.planes			= 0;
	info.depth			= 8;
	info.fps			= 24.f;
	info.alpha_type		= AEIO_Alpha_UNKNOWN;
	info.premult_color	= &premult_color;
	info.field_label	= &field_label;
    info.pixel_aspect_ratio.num = info.pixel_aspect_ratio.den = 1;
	
	info.color_profile	= NULL;
	
	
	// set up options data
	suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "Input Options",
											sizeof(format_inData),
											AEGP_MemFlag_CLEAR, &optionsH);

	err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	
	// set input defaults
	j2k_InitInOptions(basic_dataP, options);

	
	// run the function
	err = j2k_FileInfo(basic_dataP, pathZ, &info, options);

	
	// communicate into to AE
	if(!err)
	{
		// Tell AE about our file
		A_Time			dur;
		A_Boolean		has_alpha;
		
#define DEPTH_GREY_8	40
#define DEPTH_GREY_16	-16
	
		A_short			depth = info.planes < 3 ? (info.depth == 16 ? DEPTH_GREY_16 : DEPTH_GREY_8) : // greyscale
								info.planes * info.depth; // not

		err = suites.IOInSuite()->AEGP_SetInSpecDepth(specH, depth);

		err = suites.IOInSuite()->AEGP_SetInSpecDimensions(specH, info.width, info.height);
		
		// have to set duration.scale or AE complains
		dur.value = 0;
		dur.scale = 100;
		err = suites.IOInSuite()->AEGP_SetInSpecDuration(specH, &dur);

		// might get a zero denominator error without this
		//suites.IOInSuite()->AEGP_SetInSpecNativeFPS(specH, FLOAT_2_FIX(24.0) );


		has_alpha = ( (info.planes == 4) || (info.planes == 2) ) &&
						(info.alpha_type != AEIO_Alpha_NONE);

		if (!err && info.alpha_type != AEIO_Alpha_UNKNOWN)
		{
			AEIO_AlphaLabel	alpha;
			AEFX_CLR_STRUCT(alpha);

			alpha.alpha		=	has_alpha ? info.alpha_type : AEIO_Alpha_NONE;
			alpha.flags		=	(alpha.alpha == AEIO_Alpha_PREMUL) ? AEIO_AlphaPremul : 0;
			alpha.version	=	AEIO_AlphaLabel_VERSION;
			alpha.red		=	premult_color.red;
			alpha.green		=	premult_color.green;
			alpha.blue		=	premult_color.blue;
			
			err = suites.IOInSuite()->AEGP_SetInSpecAlphaLabel(specH, &alpha);
		}
		
		if (!err && field_label.type != FIEL_Type_UNKNOWN)
		{
			err = suites.IOInSuite()->AEGP_SetInSpecInterlaceLabel(specH, &field_label);
		}
		
		if(info.pixel_aspect_ratio.num != info.pixel_aspect_ratio.den)
		{
			err = suites.IOInSuite()->AEGP_SetInSpecHSF(specH, &info.pixel_aspect_ratio);
		}

		if(file_size != 0)
		{
			err = suites.IOInSuite()->AEGP_SetInSpecSize(specH, file_size);
		}

		if(info.color_profile)
		{
			// set RGB profile
			suites.IOInSuite()->AEGP_SetInSpecEmbeddedColorProfile(specH, info.color_profile, NULL);
			
			// AE made a copy, so now we disopse?
			suites.ColorSettingsSuite()->AEGP_DisposeColorProfile(info.color_profile);
		}
		
		// set options handle
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
		
		suites.IOInSuite()->AEGP_SetInSpecOptionsHandle(specH, (void*)optionsH, (void**)&old_optionsH);
		
		// see ya, old options
		// wait, actually, DON'T delete this, or AE will crash when reading cross platform projects
		//if(old_optionsH)
		//	suites.MemorySuite()->AEGP_FreeMemHandle(old_optionsH);
	}
	else
	{
		// get rid of the options handle because we got an error
		if(optionsH)
			suites.MemorySuite()->AEGP_FreeMemHandle(optionsH);
	}


	return err;
}


A_Err	
FrameSeq_DrawSparseFrame(
	AEIO_BasicData					*basic_dataP,
	AEIO_InSpecH					specH, 
	const AEIO_DrawSparseFramePB	*sparse_framePPB, 
	PF_EffectWorld					*wP,
	AEIO_DrawingFlags				*draw_flagsP)
{ 
	// we'll give a file-size buffer for filling and then fit it
	// to what AE wants
	
	A_Err err						=	A_Err_NONE;

	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

	AEIO_Handle		optionsH		=	NULL;
	format_inData	*options			=	NULL;

	
	// file path
#ifdef AE_UNICODE_PATHS
	AEGP_MemHandle pathH = NULL;
	A_PathType *file_nameZ = NULL;
	
	suites.IOInSuite()->AEGP_GetInSpecFilePath(specH, &pathH);
	
	if(pathH)
	{
		suites.MemorySuite()->AEGP_LockMemHandle(pathH, (void **)&file_nameZ);
	}
	else
		return AEIO_Err_BAD_FILENAME; 
#else
	A_PathType				file_nameZ[AEGP_MAX_PATH_SIZE];
	
	suites.IOInSuite()->AEGP_GetInSpecFilePath(specH, file_nameZ);
#endif
	
#ifdef AE_HFS_PATHS	
	// convert the path format
	if(A_Err_NONE != ConvertPath(file_nameZ, file_nameZ, AEGP_MAX_PATH_SIZE-1) )
		return AEIO_Err_BAD_FILENAME; 
#endif

	// fill out the data structure for our function
	// should match what we previously filled
	// although this time we're asking AE for the info
	FrameSeq_Info info;
	
	A_long width, height;
	A_short depth;

	PF_Pixel 	premult_color = {0, 0, 0, 255};
	FIEL_Label	field_label;

	AEIO_AlphaLabel alphaL;
	
	// get all that info from AE
	suites.IOInSuite()->AEGP_GetInSpecDimensions(specH, &width, &height);
	suites.IOInSuite()->AEGP_GetInSpecDepth(specH, &depth);
	suites.IOInSuite()->AEGP_GetInSpecAlphaLabel(specH, &alphaL);
	suites.IOInSuite()->AEGP_GetInSpecInterlaceLabel(specH, &field_label);
	
	// set the info struct
	info.width = width;
	info.height = height;
	info.planes = (depth == 32 || depth == 64 || depth == 128) ? 4 : (depth == DEPTH_GREY_8 || depth == DEPTH_GREY_16) ? 1 : 3;
	info.depth  = (depth == 48 || depth == 64 || depth == DEPTH_GREY_16) ? 16 : (depth == 96 || depth == 128) ? 32 : 8;
	
	info.alpha_type = alphaL.alpha;
	premult_color.red = alphaL.red;
	premult_color.green = alphaL.green;
	premult_color.blue = alphaL.blue;
	info.premult_color = &premult_color;
	
	info.field_label = &field_label;

	
	// get options handle
	err = suites.IOInSuite()->AEGP_GetInSpecOptionsHandle(specH, (void**)&optionsH);


	if(optionsH)
		err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	
	
	if (!err) // so far so good?
	{
		PF_EffectWorld	temp_World_data;
		
		PF_EffectWorld	*temp_World = &temp_World_data,
					*active_World = NULL;
		
		PF_PixelFormat	pixel_format;

		suites.PFWorldSuite()->PF_GetPixelFormat(wP, &pixel_format);

		A_long		wP_depth =	(pixel_format == PF_PixelFormat_ARGB128)? 32 :
								(pixel_format == PF_PixelFormat_ARGB64) ? 16 : 8;
		

		// JPEG 2000 can subsample if that's what AE wants
		PF_Point scale;
		scale.h = sparse_framePPB->rs.x.den / sparse_framePPB->rs.x.num; // scale.h = 2 means 1/2 x scale
		scale.v = sparse_framePPB->rs.y.den / sparse_framePPB->rs.y.num;
		
		const A_u_char max_subsample = MIN(scale.h, scale.v);
		
		// only subsample to power of 2 dimensions
		// not sure what Kadaku does otherwise
		A_u_char subsample = 1;
		
		/* -- turning off subsampling for now
		while(((subsample * 2) <= max_subsample) &&
				(info.width % (subsample * 2) == 0) &&
				(info.height % (subsample * 2) == 0))
		{
			subsample *= 2;
		}
		-- */

		// here's the only time we won't need to make our own buffer
		if(	(info.width/subsample == wP->width) && (info.height/subsample == wP->height) &&
			!(wP_depth != 8 && options->has_LUT) ) // need to make an 8-bit world if we have a LUT
		{
			active_World = wP; // just use the PF_EffectWorld AE gave us
			
			temp_World = NULL; // won't be needing this
		}
		else
		{
			// make our own PF_EffectWorld
			suites.PFWorldSuite()->PF_NewWorld(NULL, info.width/subsample, info.height/subsample, FALSE,
										(options->has_LUT ? PF_PixelFormat_ARGB32 : pixel_format), temp_World);
			
			active_World = temp_World;
		}


		// should always pass a full-sized float world to write into (using options we pass)
		err = j2k_DrawSparseFrame(basic_dataP, sparse_framePPB, active_World,
										draw_flagsP, file_nameZ, &info, options, subsample);

		
		// for single-channel images, copy R to GB
		if (info.planes == 1)
			WorldGreyToRGBA(basic_dataP, active_World);


		if(temp_World)
		{
			// demotion, etc handled in j2k_DrawSparseFrame()
			SmartCopyWorld(basic_dataP, temp_World, wP, FALSE, FALSE, TRUE);

			suites.PFWorldSuite()->PF_DisposeWorld(NULL, temp_World);
		}


		// done with options
		if(optionsH)
			suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	}


	return err;
};


A_Err	
FrameSeq_SeqOptionsDlg(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,  
	A_Boolean		*user_interactedPB0)
{
	// yay! input options!
	// we get the options handle, you do the dialog
	// if only there weren't a bug in AE preventing this from working with file sequences
	
	A_Err err						=	A_Err_NONE;

	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

	AEIO_Handle		optionsH		=	NULL;
	format_inData	*options			=	NULL;
	
	// get options handle
	err = suites.IOInSuite()->AEGP_GetInSpecOptionsHandle(specH, (void**)&optionsH);

	if(optionsH)
		err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	

	if(!err && options)
	{
		// pop up a dialog and change those read options
		err = j2k_ReadOptionsDialog(basic_dataP, options, user_interactedPB0);
	}


	// done with options
	if(optionsH)
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	

	return err;
}


A_Err
FrameSeq_DisposeInSpec(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH)
{ 
	// dispose input options
	
	A_Err				err			=	A_Err_NONE; 
	AEIO_Handle			optionsH	=	NULL;
	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);	

	// I guess we'll dump the handle if we've got it
	err = suites.IOInSuite()->AEGP_GetInSpecOptionsHandle(specH, reinterpret_cast<void**>(&optionsH));

	if (!err && optionsH)
	{
		err = suites.MemorySuite()->AEGP_FreeMemHandle(optionsH);
	}
	return err;
};


A_Err
FrameSeq_FlattenOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	AEIO_Handle		*flat_optionsPH)
{ 
	// pass a new handle of flattened options for saving
	// but no, don't delete the handle we're using
	
	A_Err				err			=	A_Err_NONE; 
	AEIO_Handle			optionsH	=	NULL;
	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);	

	
	// get the options for flattening
	err = suites.IOInSuite()->AEGP_GetInSpecOptionsHandle(specH, reinterpret_cast<void**>(&optionsH));

	if (!err && optionsH)
	{
		AEGP_MemSize mem_size;
		
		void *round_data, *flat_data;
		
		// make a new handle that's the same size
		// we're assuming that the options are already flat
		// although they may need byte flippage
		suites.MemorySuite()->AEGP_GetMemHandleSize(optionsH, &mem_size);
		
		suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "Flat Options",
												mem_size,
												AEGP_MemFlag_CLEAR, flat_optionsPH);
		
		suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&round_data);
		suites.MemorySuite()->AEGP_LockMemHandle(*flat_optionsPH, (void**)&flat_data);
		
		// copy data
		memcpy((char*)flat_data, (char*)round_data, mem_size);
		
		// flatten copied data
		j2k_FlattenInputOptions((format_inData *)flat_data);
		
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
		suites.MemorySuite()->AEGP_UnlockMemHandle(*flat_optionsPH);
		
		// just because we're flattening the options doesn't mean we're done with them
		//suites.MemorySuite()->AEGP_FreeMemHandle(optionsH);
	}
	
	return err;
};		

A_Err
FrameSeq_InflateOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	AEIO_Handle		flat_optionsH)
{ 
	// take flat options handle and then set a new inflated options handle
	// AE wants to take care of the flat one for you this time, so no delete
	A_Err				err			=	A_Err_NONE; 
	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);	

	
	if(flat_optionsH)
	{
		AEGP_MemSize	mem_size;
		AEIO_Handle		optionsH	=	NULL,
						old_optionsH =	NULL;
		
		void *round_data, *flat_data;
		
		// make a new handle that's the same size
		// this assumes that options are always flat but may need byte flippage
		suites.MemorySuite()->AEGP_GetMemHandleSize(flat_optionsH, &mem_size);

		if( mem_size == sizeof(format_inData) )
		{
			suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "Round Options",
													mem_size,
													AEGP_MemFlag_CLEAR, &optionsH);
			
			suites.MemorySuite()->AEGP_LockMemHandle(flat_optionsH, (void**)&flat_data);
			suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&round_data);
			
			// copy it
			memcpy((char*)round_data, (char*)flat_data, mem_size);
			
			// inflate copied data
			j2k_InflateInputOptions((format_inData *)round_data);

			suites.MemorySuite()->AEGP_UnlockMemHandle(flat_optionsH);
			suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
		}
		else // options are out-dated, reset
		{
			suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "Round Options",
													sizeof(format_inData),
													AEGP_MemFlag_CLEAR, &optionsH);
			
			suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&round_data);
			
			// initialize
			j2k_InitInOptions(basic_dataP, (format_inData *)round_data);

			suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
		}

		suites.IOInSuite()->AEGP_SetInSpecOptionsHandle(specH, (void*)optionsH, (void**)&old_optionsH);

		// we'll let AE get rid of the flat options handle for us
		//suites.MemorySuite()->AEGP_FreeMemHandle(flat_optionsH);
	}
	
	return err;
};		

//     input
// =====================================================================
//     output

#pragma mark-


A_Err	
FrameSeq_GetDepths(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	AEIO_SupportedDepthFlags	*which)
{
	// just tell us what depths to enable in the menu
	return j2k_GetDepths(which);
}


A_Err	
FrameSeq_InitOutputSpec(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	A_Boolean				*user_interacted)
{
	// pass a new handle with output options
	// you may have to initialize the data, but you probably have
	// an old options handle to read from (and then dispose actually)
	
	A_Err err						=	A_Err_NONE;

	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

	AEIO_Handle		optionsH		=	NULL,
					old_optionsH	=	NULL,
					old_old_optionsH=	NULL;
	format_outData	*options			=	NULL,
					*old_options		=	NULL;


	AEGP_MemSize mem_size;

	// make new options handle
	suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "Output Options",
											sizeof(format_outData),
											AEGP_MemFlag_CLEAR, &optionsH);
	
	err = suites.MemorySuite()->AEGP_GetMemHandleSize(optionsH, &mem_size);
	
	err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);

	
	// get old options
	suites.IOOutSuite()->AEGP_GetOutSpecOptionsHandle(outH, (void**)&old_optionsH);
	
	
	if(old_optionsH)
	{
		AEGP_MemSize old_size;
		
		err = suites.MemorySuite()->AEGP_GetMemHandleSize(old_optionsH, &old_size);
				
		suites.MemorySuite()->AEGP_LockMemHandle(old_optionsH, (void**)&old_options);
		
		// first we copy the data (only as much as the previous size)
		memcpy((char*)options, (char*)old_options, old_size);
		
		// then we inflate it
		j2k_InflateOutputOptions((format_outData *)options);

		suites.MemorySuite()->AEGP_UnlockMemHandle(old_optionsH);
	}
	else
	{
		err = j2k_InitOutOptions(options);
	}

	suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	
	
	// set the options handle
	suites.IOOutSuite()->AEGP_SetOutSpecOptionsHandle(outH, (void*)optionsH, (void**)&old_old_optionsH);
	
	
	// so now AE wants me to delete this. whatever.
	if(old_old_optionsH)
		suites.MemorySuite()->AEGP_FreeMemHandle(old_old_optionsH);

	
	return err;
}


A_Err	
FrameSeq_OutputFrame(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH			outH, 
	const PF_EffectWorld	*wP)
{
	// write that frame
	// again, we'll get the info and provide a suitable buffer
	// you just write the file, big guy
	
	A_Err err						=	A_Err_NONE;

	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);


	AEIO_Handle			optionsH		=	NULL;
	format_outData		*options		=	NULL;

	PF_EffectWorld		temp_World_data;
	
	PF_EffectWorld		*temp_World		=	&temp_World_data,
						*active_World	=	NULL;

	PF_Pixel 			premult_color = {0, 0, 0, 255};
	AEIO_AlphaLabel		alpha;
	FIEL_Label			field;
	
	A_short				depth;

	FrameSeq_Info		info;
	
	
	// get options data
	suites.IOOutSuite()->AEGP_GetOutSpecOptionsHandle(outH, (void**)&optionsH);
	
	if(optionsH)
		suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	
	
	// get file path
#ifdef AE_UNICODE_PATHS
	AEGP_MemHandle pathH = NULL;
	A_PathType *file_pathZ = NULL;
	
	A_Boolean file_reservedPB = FALSE; // WTF?
	suites.IOOutSuite()->AEGP_GetOutSpecFilePath(outH, &pathH, &file_reservedPB);
	
	if(pathH)
	{
		suites.MemorySuite()->AEGP_LockMemHandle(pathH, (void **)&file_pathZ);
	}
	else
		return AEIO_Err_BAD_FILENAME; 
#else
	A_PathType file_pathZ[AEGP_MAX_PATH_SIZE+1];
	
	A_Boolean file_reservedPB = FALSE; // WTF?
	suites.IOOutSuite()->AEGP_GetOutSpecFilePath(outH, file_pathZ, &file_reservedPB);
#endif
	
#ifdef AE_HFS_PATHS	
	// convert the path format
	if(A_Err_NONE != ConvertPath(file_pathZ, file_pathZ, AEGP_MAX_PATH_SIZE-1) )
		return AEIO_Err_BAD_FILENAME; 
#endif
	
	// get dimensions
	suites.IOOutSuite()->AEGP_GetOutSpecDimensions(outH, &info.width, &info.height);
	
	
	// get depth
	suites.IOOutSuite()->AEGP_GetOutSpecDepth(outH, &depth);

	// translate to planes & depth - negative depths are greyscale
	info.planes = (depth == 32 || depth == 64 || depth == 128) ?  4 : (depth == DEPTH_GREY_8 || depth == DEPTH_GREY_16) ? 1 : 3;
	info.depth  = (depth == DEPTH_GREY_16 || depth == 48 || depth == 64) ? 16 : (depth == 96 || depth == 128) ? 32 : 8;
	

	// get pixel aspect ratio
	suites.IOOutSuite()->AEGP_GetOutSpecHSF(outH, &info.pixel_aspect_ratio);

	
	// get alpha info
	suites.IOOutSuite()->AEGP_GetOutSpecAlphaLabel(outH, &alpha);
	
	info.alpha_type = alpha.alpha;
	
	premult_color.red   = alpha.red;
	premult_color.green = alpha.green;
	premult_color.blue  = alpha.blue;
	info.premult_color = &premult_color;


	// get field info
	suites.IOOutSuite()->AEGP_GetOutSpecInterlaceLabel(outH, &field);
	
	info.field_label = &field;
	
	
	// get frame rate
	A_Fixed native_fps = INT2FIX(24);
	suites.IOOutSuite()->AEGP_GetOutSpecFPS(outH, &native_fps);
	
	info.fps = FIX_2_FLOAT(native_fps);
	
	
	// get color profile
	info.color_profile = NULL;
	A_Boolean embed_profile = FALSE;
	suites.IOOutSuite()->AEGP_GetOutSpecShouldEmbedICCProfile(outH, &embed_profile);
	
	if(embed_profile)
	{
		suites.IOOutSuite()->AEGP_GetNewOutSpecColorProfile(S_mem_id, outH, &info.color_profile);
	}
	
	
	// in case we have to create an identical world
	PF_PixelFormat	pixel_format;
	suites.PFWorldSuite()->PF_GetPixelFormat(wP, &pixel_format);


	// with JPEG 2000 we only have to copy the buffer for sYCC (format specific)
	if(options->advanced && options->color_space == JP2_COLOR_sYCC)
	{
		// make our own float PF_EffectWorld
		suites.PFWorldSuite()->PF_NewWorld(NULL, info.width, info.height, FALSE,
												pixel_format, temp_World);
		
		// copy the world, promoting will be dealt with later
		SmartCopyWorld(basic_dataP, (PF_EffectWorld *)wP, temp_World, FALSE, FALSE, FALSE);
			
		active_World = temp_World;
	}
	else
	{
		active_World = (PF_EffectWorld *)wP; // otherwise no
		
		temp_World = NULL;
	}



	// write out image (finally)
	err = j2k_OutputFile(basic_dataP, file_pathZ, &info, options, active_World);

	
	// dispose temp world if we made one	
	if(temp_World)
		suites.PFWorldSuite()->PF_DisposeWorld(NULL, temp_World);
	
	
	// dispose profile if we got one
	if(info.color_profile)
		suites.ColorSettingsSuite()->AEGP_DisposeColorProfile(info.color_profile);

	
	if(optionsH)
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	
	return err;
}


A_Err	
FrameSeq_UserOptionsDialog(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH, 
	const PF_EffectWorld	*sample0,
	A_Boolean			*user_interacted0)
{
	// output options dialog here
	// we'll give you the options data, you change it
	
	A_Err err						=	A_Err_NONE;

	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

	AEIO_Handle			optionsH		=	NULL;
	format_outData	*options			=	NULL;
	
	// get options handle
	err = suites.IOOutSuite()->AEGP_GetOutSpecOptionsHandle(outH, (void**)&optionsH);

	if(optionsH)
		err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);

	// do we have a color profile?
	A_Boolean embed_profile = FALSE;
	AEGP_ColorProfileP color_profile = NULL;
	suites.IOOutSuite()->AEGP_GetOutSpecShouldEmbedICCProfile(outH, &embed_profile);
	
	if(embed_profile)
		suites.IOOutSuite()->AEGP_GetNewOutSpecColorProfile(S_mem_id, outH, &color_profile);
	
	if(!err && options)
	{
		// do a dialog and change those output options
		err = j2k_WriteOptionsDialog(basic_dataP, options, color_profile, user_interacted0);
	}


	// dispose profile if we got one
	if(color_profile)
		suites.ColorSettingsSuite()->AEGP_DisposeColorProfile(color_profile);
		

	// cheating!  this is plug-in specific
	if(options->method == JP2_METHOD_CINEMA)
	{
		A_short	depth;
		suites.IOOutSuite()->AEGP_GetOutSpecDepth(outH, &depth);
		
		if(depth != 48)
			err = suites.IOOutSuite()->AEGP_SetOutSpecDepth(outH, 48);
	}
	else if(options->advanced && options->custom_depth && (options->bit_depth > 8) )
	{
		A_short	depth;
		suites.IOOutSuite()->AEGP_GetOutSpecDepth(outH, &depth);
		
		// force some 16-bit action
		if(depth == 24)
			err = suites.IOOutSuite()->AEGP_SetOutSpecDepth(outH, 48);
		else if(depth == DEPTH_GREY_8)
			err = suites.IOOutSuite()->AEGP_SetOutSpecDepth(outH, DEPTH_GREY_16);
		else if(depth == 32)
			err = suites.IOOutSuite()->AEGP_SetOutSpecDepth(outH, 64);
	}


	// done with options
	if(optionsH)
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	
	return err;
}


A_Err	
FrameSeq_GetOutputInfo(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH,
	AEIO_Verbiage		*verbiageP)
{ 
	// all this function does is print details about our output options
	// in the output module window
	// or rather, gets the options so your function can do that

	A_Err err			=	A_Err_NONE;
	
	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);
	
	AEIO_Handle		optionsH		=	NULL;
	format_outData	*options			=	NULL;


	// get file path (but don't freak out if it's an invalid path (do you really need a path here?))
#ifdef AE_UNICODE_PATHS
	AEGP_MemHandle pathH = NULL;
	A_PathType *file_pathZ = NULL;
	
	A_Boolean file_reservedPB = FALSE; // WTF?
	suites.IOOutSuite()->AEGP_GetOutSpecFilePath(outH, &pathH, &file_reservedPB);
	
	if(pathH)
	{
		suites.MemorySuite()->AEGP_LockMemHandle(pathH, (void **)&file_pathZ);
	}
	else
		return AEIO_Err_BAD_FILENAME; 
#else
	A_PathType file_pathZ[AEGP_MAX_PATH_SIZE+1];
	
	A_Boolean file_reservedPB = FALSE; // WTF?
	suites.IOOutSuite()->AEGP_GetOutSpecFilePath(outH, file_pathZ, &file_reservedPB);
#endif
	
#ifdef AE_HFS_PATHS	
	// convert the path format
	ConvertPath(file_pathZ, file_pathZ, AEGP_MAX_PATH_SIZE-1);
#endif


	// get options handle
	err = suites.IOOutSuite()->AEGP_GetOutSpecOptionsHandle(outH, (void**)&optionsH);

	if(optionsH)
		err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
		
	
	// initialize the verbiage
	verbiageP->type[0] = '\0';
	verbiageP->sub_type[0] = '\0';

	if(!err)
	{
		err = j2k_GetOutSpecInfo(file_pathZ, options, verbiageP);
		
		// done with options
		if(optionsH)
			suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	}

	return err;
};


A_Err	
FrameSeq_GetOutputSuffix(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	A_char			*suffix)
{
	// get our output options and set up a suffix based on them
	
	A_Err err			=	A_Err_NONE;
	
	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

	AEIO_Handle		optionsH		=	NULL;
	format_outData	*options			=	NULL;


	// get options handle
	err = suites.IOOutSuite()->AEGP_GetOutSpecOptionsHandle(outH, (void**)&optionsH);

	if(optionsH)
		err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	else
		err = AEIO_Err_USE_DFLT_CALLBACK; // just stick with the format default


	if(!err)
	{
		err = j2k_GetOutputSuffix(options, suffix);
	}


	// done with options
	if(options)
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);

	return err;
}


A_Err	
FrameSeq_DisposeOutputOptions(
	AEIO_BasicData	*basic_dataP,
	void			*optionsPV) // what the...?
{ 
	// the options gotta go sometime
	// couldn't you just say optionsPV was a handle?
	
	A_Err				err			=	A_Err_NONE; 
	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);	

	// here's our options handle apparently
	//AEIO_Handle		optionsH	=	reinterpret_cast<AEIO_Handle>(optionsPV);
	AEIO_Handle		optionsH	=	static_cast<AEIO_Handle>(optionsPV);
	
	if (!err && optionsH)
	{
		err = suites.MemorySuite()->AEGP_FreeMemHandle(optionsH);
	}
	return err;
};


A_Err	
FrameSeq_GetFlatOutputOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	AEIO_Handle		*flat_optionsPH)
{
	// give AE and handle with flat options for saving
	// but don't delete the old handle, AE still wants it
	
	A_Err				err			=	A_Err_NONE; 
	AEIO_Handle			optionsH	=	NULL;
	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);	

	
	// get the options for flattening
	err = suites.IOOutSuite()->AEGP_GetOutSpecOptionsHandle(outH, reinterpret_cast<void**>(&optionsH));

	if (!err && optionsH)
	{
		AEGP_MemSize mem_size;
		
		format_outData *round_data, *flat_data;
		
		// make a new handle that's the same size
		// we're assuming that the options are already flat
		// although they may need byte flippage
		suites.MemorySuite()->AEGP_GetMemHandleSize(optionsH, &mem_size);
		
		suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "Flat Options",
												mem_size,
												AEGP_MemFlag_CLEAR, flat_optionsPH);
		
		suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&round_data);
		suites.MemorySuite()->AEGP_LockMemHandle(*flat_optionsPH, (void**)&flat_data);
		
		// first we copy
		memcpy((char*)flat_data, (char*)round_data, mem_size);
		
		// then we flatten
		j2k_FlattenOutputOptions((format_outData *)flat_data);

		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
		suites.MemorySuite()->AEGP_UnlockMemHandle(*flat_optionsPH);
		
		// just because we're flattening the options doesn't mean we're done with them
		//suites.MemorySuite()->AEGP_FreeMemHandle(optionsH);
	}
	
	return err;
}

