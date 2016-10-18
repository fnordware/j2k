
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

#ifndef FRAMESEQ_H
#define FRAMESEQ_H

#include "AEConfig.h"
#include "AE_IO.h"
#include "AE_Macros.h"
#include "fnord_SuiteHandler.h"

#ifndef MAX
#define MAX(A,B)			( (A) > (B) ? (A) : (B))
#endif
#define AE_RANGE(NUM)		(A_u_short)MIN( MAX( (NUM), 0 ), PF_MAX_CHAN16 )
#define AE8_RANGE(NUM)		(A_u_char)MIN( MAX( (NUM), 0 ), PF_MAX_CHAN8 )
#define SIXTEEN_RANGE(NUM)	(A_u_short)MIN( MAX( (NUM), 0 ), 0xFFFF )

#define AE_TO_FLOAT(NUM)		( (float)(NUM) / (float)PF_MAX_CHAN16 )
#define FLOAT_TO_AE(NUM)		AE_RANGE( ( (NUM) * (float)PF_MAX_CHAN16 ) + 0.5)

#define AE8_TO_FLOAT(NUM)		( (float)(NUM) / (float)PF_MAX_CHAN8 )
#define FLOAT_TO_AE8(NUM)		AE8_RANGE( ( (NUM) * (float)PF_MAX_CHAN8 ) + 0.5)

#define SIXTEEN_TO_FLOAT(NUM)		( (float)(NUM) / (float)0xFFFF )
#define FLOAT_TO_SIXTEEN(NUM)		SIXTEEN_RANGE( ( (NUM) * (float)0xFFFF ) + 0.5)

// addition to AEIO_AlphaType
#define AEIO_Alpha_UNKNOWN	99

// addition to FIEL_Type
#define FIEL_Type_UNKNOWN	88


typedef struct FrameSeq_Info
{
	A_long				width;
	A_long				height;
	A_long				planes;
	A_long				depth;		// 8 or 16 or 32!
	A_FpShort			fps;		// frame rate
	AEIO_AlphaType		alpha_type;
    A_Ratio             pixel_aspect_ratio;
	PF_Pixel			*premult_color;
	FIEL_Label			*field_label;	// if so inclined
	AEGP_ColorProfileP	color_profile;
} FrameSeq_Info;


A_Err
DemoteWorld(AEIO_BasicData *basic_dataP, PF_EffectWorld *world);

A_Err
PromoteWorld(AEIO_BasicData *basic_dataP, PF_EffectWorld *world);


A_Err
FrameSeq_PluginName(char *name);

A_Err
FrameSeq_Init(struct SPBasicSuite *pica_basicP);

A_Err
FrameSeq_ConstructModuleInfo(
	AEIO_ModuleInfo	*info);

A_Err	
FrameSeq_GetInSpecInfo(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH, 
	AEIO_Verbiage	*verbiageP);

A_Err	
FrameSeq_VerifyFileImportable(
	AEIO_BasicData			*basic_dataP,
	AEIO_ModuleSignature	sig, 
	const A_PathType		*file_pathZ, 
	A_Boolean				*importablePB);

A_Err	
FrameSeq_InitInSpecFromFile(
	AEIO_BasicData	*basic_dataP,
	const A_PathType	*file_pathZ,
	AEIO_InSpecH	specH);

A_Err	
FrameSeq_DrawSparseFrame(
	AEIO_BasicData					*basic_dataP,
	AEIO_InSpecH					specH, 
	const AEIO_DrawSparseFramePB	*sparse_framePPB, 
	PF_EffectWorld					*wP,
	AEIO_DrawingFlags				*draw_flagsP);
	
A_Err	
FrameSeq_SeqOptionsDlg(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,  
	A_Boolean		*user_interactedPB0);

A_Err
FrameSeq_DisposeInSpec(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH);

A_Err
FrameSeq_FlattenOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	AEIO_Handle		*flat_optionsPH);

A_Err
FrameSeq_InflateOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	AEIO_Handle		flat_optionsH);


//     input
// =================================
//     output


A_Err	
FrameSeq_GetDepths(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	AEIO_SupportedDepthFlags	*which);

A_Err	
FrameSeq_InitOutputSpec(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	A_Boolean				*user_interacted);
	
A_Err	
FrameSeq_OutputFrame(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH			outH, 
	const PF_EffectWorld	*wP);

A_Err	
FrameSeq_UserOptionsDialog(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH, 
	const PF_EffectWorld	*sample0,
	A_Boolean			*user_interacted0);

A_Err	
FrameSeq_GetOutputInfo(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH,
	AEIO_Verbiage		*verbiageP);

A_Err	
FrameSeq_GetOutputSuffix(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	A_char			*suffix);

A_Err	
FrameSeq_DisposeOutputOptions(
	AEIO_BasicData	*basic_dataP,
	void			*optionsPV);

A_Err	
FrameSeq_GetFlatOutputOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	AEIO_Handle		*flat_optionsPH);

#endif // FRAMESEQ_H
