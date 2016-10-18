
/* ---------------------------------------------------------------------
// 
// j2k - JPEG 2000 plug-ins for Adobe programs
// Copyright (c) 2002-2016 Brendan Bolles and Aaron Boxer
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

#ifndef j2k_H
#define j2k_H

#include "FrameSeq.h"


#define PLUGIN_NAME		"JPEG 2000"


enum {
	JP2_TYPE_UNKNOWN = 0,
	JP2_TYPE_J2C,
	JP2_TYPE_JP2,
	JP2_TYPE_JPX
};
typedef A_u_char	JPEG_Format;

#define JP2_BIT_DEPTH_AE	-1  // get depth from AE, not dialog

enum{
	JP2_METHOD_LOSSLESS = 1,
	JP2_METHOD_SIZE,
	JP2_METHOD_QUALITY,
	JP2_METHOD_CINEMA
};
typedef A_u_char JPEG_Method;


enum{
	JP2_ORDER_LRCP = 0,
	JP2_ORDER_RLCP,
	JP2_ORDER_RPCL,
	JP2_ORDER_PCRL,
	JP2_ORDER_CPRL
};
typedef A_u_char JPEG_Order;
	

enum{
	JP2_SUBSAMPLE_NONE = 0,
	JP2_SUBSAMPLE_422,
	JP2_SUBSAMPLE_411,
	JP2_SUBSAMPLE_420,
	JP2_SUBSAMPLE_311,
	JP2_SUBSAMPLE_2x2,
	JP2_SUBSAMPLE_3x3,
	JP2_SUBSAMPLE_4x4
};
typedef A_u_char JPEG_Subsampling;


enum{
	JP2_COLOR_sRGB = 16, // same as jp2_colour_space
	JP2_COLOR_sYCC = 18,
	JP2_COLOR_ICC  = 101 // use profile passed by AE
};
typedef A_u_long JPEG_Color;

enum {
	JP2_DCI_2K = 0,
	JP2_DCI_4K
};
typedef A_u_char JPEG_DCI_Profile;

typedef enum {
	DCI_PER_FRAME = 0,
	DCI_PER_SECOND
};
typedef A_u_char JPEG_DCI_Per_Frame;


#define J2K_VERSION_MAJOR	2
#define J2K_VERSION_MINOR	6

typedef struct j2k_inData
{
	A_u_char			version_major;
	A_u_char			version_minor;
	JPEG_Format			format;
	A_u_char			bit_depth;
	A_Boolean			reversible;
	A_Boolean			ycc;
	A_Boolean			has_LUT; // i.e. indexed color, must do 8-bit
} j2k_inData;


#define OUT_DEFAULT_FORMAT		JP2_TYPE_JPX
#define OUT_DEFAULT_CUSTOM_DEPTH FALSE
#define OUT_DEFAULT_BIT_DEPTH	8
#define OUT_DEFAULT_REVERSIBLE	TRUE
#define OUT_DEFAULT_METHOD		JP2_METHOD_LOSSLESS
#define OUT_DEFAULT_QUALITY		50
#define OUT_DEFAULT_SIZE		50
#define OUT_DEFAULT_ADVANCED	FALSE
#define OUT_DEFAULT_YCC			TRUE
#define OUT_DEFAULT_SUB			JP2_SUBSAMPLE_NONE
#define OUT_DEFAULT_LAYERS		12
#define OUT_DEFAULT_ORDER		JP2_ORDER_RPCL
#define OUT_DEFAULT_TILE_SIZE	1024
#define OUT_DEFAULT_COLOR_SPACE	JP2_COLOR_ICC
#define OUT_DEFAULT_DCI_PROFILE	JP2_DCI_2K
#define OUT_DEFAULT_DATA_RATE	30000
#define OUT_DEFAULT_PER_FRAME	DCI_PER_SECOND
#define OUT_DEFAULT_FRAME_RATE	24
#define OUT_DEFAULT_STEREO		FALSE

typedef struct j2k_outData
{
	A_u_char			version_major;
	A_u_char			version_minor;
	JPEG_Format			format;
	A_Boolean			custom_depth;
	A_u_short			bit_depth;
	A_Boolean			reversible;  // i.e. are we using integers?
	JPEG_Method			method;		// lossless, quality, file size, etc.
	A_u_char			quality;
	A_u_long			size;		// in kilobytes
	A_Boolean			advanced;	// are we showing the big UI?
	A_Boolean			ycc;
	JPEG_Subsampling	sub;
	A_u_char			layers;
	JPEG_Order			order;
	JPEG_Color			color_space;
	A_u_short			tile_size;
	A_u_long			dci_data_rate;
	JPEG_DCI_Profile	dci_profile;
	JPEG_DCI_Per_Frame	dci_per_frame;
	A_char				dci_frame_rate;
	A_Boolean			dci_stereo;
} j2k_outData;


#if !defined(macintosh) && !defined(__GNUC__)
int log2(int input);
#endif


A_Err
j2k_PluginName(char *name);

A_Err
j2k_Init(struct SPBasicSuite *pica_basicP);

A_Err
j2k_ConstructModuleInfo(
	AEIO_ModuleInfo	*info);

A_Err	
j2k_GetInSpecInfo(
	const A_PathType	*file_pathZ,
	j2k_inData		*options,
	AEIO_Verbiage	*verbiageP);

A_Err	
j2k_VerifyFile(
	const A_PathType		*file_pathZ, 
	A_Boolean				*importablePB);

A_Err
j2k_FileInfo(
	AEIO_BasicData	*basic_dataP,
	const A_PathType *file_pathZ,
	FrameSeq_Info	*info,
	j2k_inData		*options);

A_Err	
j2k_DrawSparseFrame(
	AEIO_BasicData					*basic_dataP,
	const AEIO_DrawSparseFramePB	*sparse_framePPB, 
	PF_EffectWorld					*wP,
	AEIO_DrawingFlags				*draw_flagsP,
	const A_PathType				*file_pathZ,
	FrameSeq_Info					*info,
	j2k_inData						*options,
	A_u_char						subsample);
	

A_Err	
j2k_InitInOptions(
	AEIO_BasicData	*basic_dataP,
	j2k_inData	*options);

A_Err	
j2k_ReadOptionsDialog(
	AEIO_BasicData	*basic_dataP,
	j2k_inData	*options,
	A_Boolean		*user_interactedPB0);
	
A_Err
j2k_FlattenInputOptions(
	j2k_inData	*options);

A_Err
j2k_InflateInputOptions(
	j2k_inData	*options);


#ifdef DO_AUX_CHANNELS

A_Err	
j2k_GetNumAuxChannels(
	j2k_inData	*options,
	const A_PathType *file_pathZ,
	A_long			*num_channelsPL);
									
A_Err	
j2k_GetAuxChannelDesc(	
	j2k_inData	*options,
	const A_PathType *file_pathZ,
	A_long			chan_indexL,
	PF_ChannelDesc	*descP);
																
A_Err	
j2k_DrawAuxChannel(
	j2k_inData		*options,
	const A_PathType	*file_pathZ,
	A_long				chan_indexL,
	const AEIO_DrawFramePB	*pbP,
	PF_Point			scale,
	PF_ChannelChunk		*chunkP);

#endif // DO_AUX_CHANNELS

//     input
// =================================
//     output


A_Err	
j2k_GetDepths(
	AEIO_SupportedDepthFlags		*which);
	

A_Err	
j2k_InitOutOptions(
	j2k_outData	*options);

A_Err
j2k_OutputFile(
	AEIO_BasicData		*basic_dataP,
	const A_PathType	*file_pathZ,
	FrameSeq_Info		*info,
	j2k_outData			*options,
	PF_EffectWorld		*wP);

A_Err	
j2k_WriteOptionsDialog(
	AEIO_BasicData		*basic_dataP,
	j2k_outData			*options,
	AEGP_ColorProfileP	color_profile,
	A_Boolean			*user_interactedPB0);

A_Err	
j2k_GetOutSpecInfo(
	const A_PathType *file_pathZ,
	j2k_outData		*options,
	AEIO_Verbiage	*verbiageP);

A_Err	
j2k_GetOutputSuffix(
	j2k_outData			*options,
	A_char				*suffix);

A_Err
j2k_FlattenOutputOptions(
	j2k_outData	*options);

A_Err
j2k_InflateOutputOptions(
	j2k_outData	*options);

#endif // j2k_H
