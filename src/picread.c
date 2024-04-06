/****************************************************************************
 * NCSA Mosaic for the X Window System                                      *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 * mosaic@ncsa.uiuc.edu                                                     *
 *                                                                          *
 * Copyright (C) 1993, Board of Trustees of the University of Illinois      *
 *                                                                          *
 * NCSA Mosaic software, both binary and source (hereafter, Software) is    *
 * copyrighted by The Board of Trustees of the University of Illinois       *
 * (UI), and ownership remains with the UI.                                 *
 *                                                                          *
 * The UI grants you (hereafter, Licensee) a license to use the Software    *
 * for academic, research and internal business purposes only, without a    *
 * fee.  Licensee may distribute the binary and source code (if released)   *
 * to third parties provided that the copyright notice and this statement   *
 * appears on all copies and that no charge is associated with such         *
 * copies.                                                                  *
 *                                                                          *
 * Licensee may make derivative works.  However, if Licensee distributes    *
 * any derivative work based on or derived from the Software, then          *
 * Licensee will (1) notify NCSA regarding its distribution of the          *
 * derivative work, and (2) clearly notify users that such derivative       *
 * work is a modified version and not the original NCSA Mosaic              *
 * distributed by the UI.                                                   *
 *                                                                          *
 * Any Licensee wishing to make commercial use of the Software should       *
 * contact the UI, c/o NCSA, to negotiate an appropriate license for such   *
 * commercial use.  Commercial use includes (1) integration of all or       *
 * part of the source code into a product for sale or license by or on      *
 * behalf of Licensee to third parties, or (2) distribution of the binary   *
 * code or source code to third parties that need it to utilize a           *
 * commercial product sold or licensed by or on behalf of Licensee.         *
 *                                                                          *
 * UI MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR   *
 * ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED          *
 * WARRANTY.  THE UI SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY THE    *
 * USERS OF THIS SOFTWARE.                                                  *
 *                                                                          *
 * By using or copying this Software, Licensee agrees to abide by the       *
 * copyright law and all other applicable laws of the U.S. including, but   *
 * not limited to, export control laws, and the terms of this license.      *
 * UI shall have the right to terminate this license immediately by         *
 * written notice upon Licensee's breach of, or non-compliance with, any    *
 * of its terms.  Licensee may be held legally responsible for any          *
 * copyright infringement that is caused or encouraged by Licensee's        *
 * failure to abide by the terms of this license.                           *
 *                                                                          *
 * Comments and questions are welcome and can be sent to                    *
 * mosaic-x@ncsa.uiuc.edu.                                                  *
 ****************************************************************************/
#include <FreeImage.h>

#include "../config.h"
#include "mosaic.h"
#include "picread.h"
#include "gifread.h"
#include "xpmread.h"

#ifdef HAVE_JPEG
#include "readJPEG.h"
#endif

#ifdef HAVE_PNG
#include "readPNG.h"
#endif

#include <X11/Xos.h>

#define DEF_BLACK       BlackPixel(dsp, DefaultScreen(dsp))
#define DEF_WHITE       WhitePixel(dsp, DefaultScreen(dsp))
#define	MAX_LINE	81


/*extern unsigned char *ReadGIF();
extern unsigned char *ReadXpm3Pixmap();
extern unsigned char *ReadJPEG();*/


extern Display *dsp;

extern int installed_colormap;
extern Colormap installed_cmap;

#ifndef DISABLE_TRACE
extern int srcTrace;
#endif

static unsigned DLL_CALLCONV
myReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle) {
 return (unsigned)fread(buffer, size, count, (FILE *)handle);
}
static unsigned DLL_CALLCONV
myWriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle) {
 return (unsigned)fwrite(buffer, size, count, (FILE *)handle);
}
static int DLL_CALLCONV
mySeekProc(fi_handle handle, long offset, int origin) {
 return fseek((FILE *)handle, offset, origin);
}
static long DLL_CALLCONV
myTellProc(fi_handle handle) {
 return ftell((FILE *)handle);
}
unsigned char *ReadIMG_name(char *name, int *w, int*h, XColor *c){
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"[ReadIMG] Entering\n");
	}
#endif

	FreeImage_Initialise(FALSE);

	unsigned char *pixmap;	
	XColor *colors = c;


	FIBITMAP *bitmap;


	FREE_IMAGE_FORMAT filetype = FreeImage_GetFileType(name,0);
	if(filetype == -1){
#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"[ReadIMG_name] Could not identify file type\n");
		}
#endif	
		//FreeImage_DeInitialise();
		//	return (unsigned char *)NULL;

	}
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"[ReadIMG_name] filetype: %d\n",filetype);
	}
#endif


	bitmap = FreeImage_Load(filetype,name,0);

	if(bitmap == NULL){
#ifndef DISABLE_TRACE
				if(srcTrace){
					fprintf(stderr,"[ReadIMG_name] First attempt FreeImage_LoadFromHandle err format = %d\n",filetype);
				}
#endif

		for(int i = 0; i <= 36; i++){
			bitmap = FreeImage_Load(i,name,0);
			if(bitmap == NULL){
#ifndef DISABLE_TRACE
				if(srcTrace){
					fprintf(stderr,"[ReadIMG_name] FreeImage_LoadFromHandle err format = %d\n",i);
				}
#endif
				continue;
			}
			else{
#ifndef DISABLE_TRACE
				if(srcTrace){
					fprintf(stderr,"[ReadIMG_name] FreeImage_LoadFromHandle success format = %d\n",i);
				}
#endif
				break;
			}

		}	
	}
	if(bitmap == NULL){

#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"[ReadIMG_name] attempt for all format failed\n");
		}
#endif
		FreeImage_DeInitialise();
		return NULL;
	}


	// 画像の幅と高さを取得
	*w = FreeImage_GetWidth(bitmap);
	*h = FreeImage_GetHeight(bitmap);

	int bpp = FreeImage_GetBPP(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"bpp before: %d\n",bpp);
	}
#endif

	if(bpp == 24){  //if high color img, dither to 256 colors
		//FIBITMAP *d_bitmap = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);
		//FIQ_WUQUANT
		//FIQ_NNQUANT
		//FIQ_LFPQUANT
		bitmap = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);
	}
	if(bpp == 32){
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
		bitmap = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);

	}

	bpp = FreeImage_GetBPP(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"bpp after: %d\n",bpp);
 	int numColors = FreeImage_GetColorsUsed(bitmap);
	
		fprintf(stderr,"numColors after: %d\n",numColors);
	}
#endif


	
	RGBQUAD *pal = FreeImage_GetPalette(bitmap);

	if(pal){
 		int nColors = FreeImage_GetColorsUsed(bitmap);
		for(int i = 0; i < (nColors -1); i++){   //パレットのコピー
			colors[i].red = pal[i].rgbRed << 8; //256倍している
			colors[i].green = pal[i].rgbGreen << 8;
			colors[i].blue = pal[i].rgbBlue << 8;
			colors[i].pixel = i;
			colors[i].flags = DoRed|DoGreen|DoBlue;
		}
	}
	else{  //ここまで来てもパレットがない= gray scale
		for(int i = 0; i < 255; i++){   //パレットのコピー
			/*
			   colors[i].red = pal[i].rgbRed << 8; //256倍している
			   colors[i].green = pal[i].rgbGreen << 8;
			   colors[i].blue = pal[i].rgbBlue << 8;
			   */
			colors[i].red = i << 8;
			colors[i].green = i << 8;
			colors[i].blue = i << 8;
			colors[i].pixel = i;
			colors[i].flags = DoRed|DoGreen|DoBlue;
		}

	}


	pixmap = (unsigned char *)malloc((*w)*(*h)*sizeof(unsigned char));
	
	//printf("pixmap sizeサイズ:%lu\n",sizeof(*pixmap));
	//memcpy(pixmap, (unsigned char *)bitmap,sizeof((unsigned char *)bitmap));

	
	unsigned char *bits = (unsigned char *)FreeImage_GetBits(bitmap);
	int size = (*w)*(*h)*sizeof(unsigned char);

	for(int i=0;i<(*h);i++){
		for(int j = 0;j<(*w);j++){
			pixmap[i*(*w)+j] = bits[(*h - i -1)*(*w) + j];
			//pixmap[(i-1)*(*w)+(j-1)] = bits[(*h - (i-1))*(*w) + (j - 1)];
			//pixmap[i*(*w)+j] = bits[i*(*w)+j];
		}

	}
	/*
	for(int i=0;i < size ;i++){
		//pixmap[i] = (unsigned char)uc_bitmap[i];
		pixmap[size - i - 1] = bits[i];
	}
	*/

	// 画像の解放
	FreeImage_Unload(bitmap);

	// FreeImageの終了
	FreeImage_DeInitialise();

	return  pixmap;
}
unsigned char *ReadIMG(FILE *ptr, int *w, int*h, XColor *c){
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"[ReadIMG] Entering\n");
	}
#endif

	FreeImage_Initialise(FALSE);

	unsigned char *pixmap;	
	FILE *fp = ptr;
	XColor *colors = c;


	// initialize your own IO functions
	FreeImageIO io;
	io.read_proc = myReadProc;
	io.write_proc = myWriteProc;
	io.seek_proc = mySeekProc;
	io.tell_proc = myTellProc;

	FIBITMAP *bitmap;

	if(fp == NULL){
#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"[fi_ReadPNG] fp err\n");
		}
#endif
		FreeImage_DeInitialise();
		return NULL;
	}

	FREE_IMAGE_FORMAT filetype = FreeImage_GetFileTypeFromHandle(&io,(fi_handle)fp,0);
	//FREE_IMAGE_FORMAT filetype = FreeImage_GetFileType();
	if(filetype == -1){
#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"[ReadIMG] Could not identify file type\n");
		}
#endif	
		//FreeImage_DeInitialise();
		//	return (unsigned char *)NULL;

	}
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"[ReadIMG] filetype: %d\n",filetype);
	}
#endif


	bitmap = FreeImage_LoadFromHandle(filetype, &io, (fi_handle)fp, 0);

	if(bitmap == NULL){
#ifndef DISABLE_TRACE
				if(srcTrace){
					fprintf(stderr,"[ReadIMG] First attempt FreeImage_LoadFromHandle err format = %d\n",filetype);
				}
#endif

		for(int i = 0; i <= 36; i++){
			bitmap = FreeImage_LoadFromHandle((FREE_IMAGE_FORMAT)i, &io, (fi_handle)fp, 0);	
			if(bitmap == NULL){
#ifndef DISABLE_TRACE
				if(srcTrace){
					fprintf(stderr,"[ReadIMG] FreeImage_LoadFromHandle err format = %d\n",i);
				}
#endif
				continue;
			}
			else{
#ifndef DISABLE_TRACE
				if(srcTrace){
					fprintf(stderr,"[ReadIMG] FreeImage_LoadFromHandle success format = %d\n",i);
				}
#endif
				break;
			}

#ifndef DISABLE_TRACE
				if(srcTrace){
					fprintf(stderr,"[ReadIMG] attempt for all format failed\n");
				}
#endif
			FreeImage_DeInitialise();
			return NULL;
		}	
	}


	/*
	FIBITMAP *bitmap = FreeImage_LoadFromHandle(filetype, &io, (fi_handle)fp, 0);

	if(!bitmap){
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"[ReadIMG] FreeImage_LoadFromHandle err\n");
	}
#endif

		return NULL;
	}
	*/



	
	// 画像の幅と高さを取得
	*w = FreeImage_GetWidth(bitmap);
	*h = FreeImage_GetHeight(bitmap);

	int bpp = FreeImage_GetBPP(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"bpp before: %d\n",bpp);
	}
#endif

	if((bpp == 24) |(bpp == 32)){  //if high color img, dither to 256 colors
		//FIBITMAP *d_bitmap = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);
		//FIQ_WUQUANT
		//FIQ_NNQUANT
		//FIQ_LFPQUANT
		bitmap = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);
	}

	bpp = FreeImage_GetBPP(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"bpp after: %d\n",bpp);
 	int numColors = FreeImage_GetColorsUsed(bitmap);
	
		fprintf(stderr,"numColors after: %d\n",numColors);
	}
#endif



	
	RGBQUAD *pal = FreeImage_GetPalette(bitmap);

	if(pal){
 		int nColors = FreeImage_GetColorsUsed(bitmap);
		for(int i = 0; i < (nColors -1); i++){   //パレットのコピー
			colors[i].red = pal[i].rgbRed << 8; //256倍している
			colors[i].green = pal[i].rgbGreen << 8;
			colors[i].blue = pal[i].rgbBlue << 8;
			colors[i].pixel = i;
			colors[i].flags = DoRed|DoGreen|DoBlue;
		}
	}
	else{  //ここまで来てもパレットがない= gray scale
		for(int i = 0; i < 255; i++){   //パレットのコピー
			/*
			   colors[i].red = pal[i].rgbRed << 8; //256倍している
			   colors[i].green = pal[i].rgbGreen << 8;
			   colors[i].blue = pal[i].rgbBlue << 8;
			   */
			colors[i].red = i << 8;
			colors[i].green = i << 8;
			colors[i].blue = i << 8;
			colors[i].pixel = i;
			colors[i].flags = DoRed|DoGreen|DoBlue;
		}

	}


	pixmap = (unsigned char *)malloc((*w)*(*h)*sizeof(unsigned char));
	
	//printf("pixmap sizeサイズ:%lu\n",sizeof(*pixmap));
	//memcpy(pixmap, (unsigned char *)bitmap,sizeof((unsigned char *)bitmap));

	
	unsigned char *bits = (unsigned char *)FreeImage_GetBits(bitmap);
	int size = (*w)*(*h)*sizeof(unsigned char);

	for(int i=0;i<(*h);i++){
		for(int j = 0;j<(*w);j++){
			pixmap[i*(*w)+j] = bits[(*h - i -1)*(*w) + j];
	//		pixmap[(i-1)*(*w)+(j-1)] = bits[(*h - (i-1))*(*w) + (j - 1)];
			//pixmap[i*(*w)+j] = bits[i*(*w)+j];
		}

	}
	/*
	for(int i=0;i < size ;i++){
		//pixmap[i] = (unsigned char)uc_bitmap[i];
		pixmap[size - i - 1] = bits[i];
	}
	*/

	// 画像の解放
	FreeImage_Unload(bitmap);

	// FreeImageの終了
	FreeImage_DeInitialise();

	return  pixmap;
}
unsigned char *fi_ReadPNG(FILE *ptr, int *w, int*h, XColor *c){
	unsigned char *pixmap;	
	FILE *fp = ptr;
	XColor *colors = c;

	FreeImage_Initialise(FALSE);
	// initialize your own IO functions
	FreeImageIO io;
	io.read_proc = myReadProc;
	io.write_proc = myWriteProc;
	io.seek_proc = mySeekProc;
	io.tell_proc = myTellProc;
	

	if(fp == NULL){
#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"[fi_ReadPNG] fp err\n");
		}
#endif
		FreeImage_DeInitialise();
		return NULL;
	}

	FIBITMAP *bitmap = FreeImage_LoadFromHandle(FIF_PNG, &io, (fi_handle)fp, 0);
	
	if(!bitmap){
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"[fi_ReadPNG] FreeImage_LoadFromHandle err\n");
	}
#endif
		FreeImage_DeInitialise();
		return NULL;
	}
	
	// 画像の幅と高さを取得
	*w = FreeImage_GetWidth(bitmap);
	*h = FreeImage_GetHeight(bitmap);

	int bpp = FreeImage_GetBPP(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"bpp before: %d\n",bpp);
	}
#endif

	if((bpp == 24) |(bpp == 32)){  //if high color img, dither to 256 colors
		//FIBITMAP *d_bitmap = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);
		//FIQ_WUQUANT
		//FIQ_NNQUANT
		//FIQ_LFPQUANT
		bitmap = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);
	}

	bpp = FreeImage_GetBPP(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"bpp after: %d\n",bpp);
 	int numColors = FreeImage_GetColorsUsed(bitmap);
	
		fprintf(stderr,"numColors after: %d\n",numColors);
	}
#endif



	
	RGBQUAD *pal = FreeImage_GetPalette(bitmap);

	if(pal){
		int nColors = FreeImage_GetColorsUsed(bitmap);
		for(int i = 0; i < (nColors -1); i++){   //パレットのコピー
			colors[i].red = pal[i].rgbRed << 8; //256倍している
			colors[i].green = pal[i].rgbGreen << 8;
			colors[i].blue = pal[i].rgbBlue << 8;
			colors[i].pixel = i;
			colors[i].flags = DoRed|DoGreen|DoBlue;
		}
	}
	else{  //ここまで来てもパレットがない= gray scale
		for(int i = 0; i < 256; i++){   //パレットのコピー
			/*
			   colors[i].red = pal[i].rgbRed << 8; //256倍している
			   colors[i].green = pal[i].rgbGreen << 8;
			   colors[i].blue = pal[i].rgbBlue << 8;
			   */
			colors[i].red = i << 8;
			colors[i].green = i << 8;
			colors[i].blue = i << 8;
			colors[i].pixel = i;
			colors[i].flags = DoRed|DoGreen|DoBlue;
		}

	}


	pixmap = (unsigned char *)malloc((*w)*(*h)*sizeof(unsigned char));

	if(FreeImage_IsTransparent(bitmap)){
		unsigned char *bits = (unsigned char *)FreeImage_GetBits(bitmap);
		int size = (*w)*(*h)*sizeof(unsigned char);

		for(int i=0;i<(*h);i++){
			for(int j = 0;j<(*w);j++){
				pixmap[i*(*w)+j] = bits[(*h - i -1)*(*w) + j];
			}

		}


	}else{

		unsigned char *bits = (unsigned char *)FreeImage_GetBits(bitmap);
		int size = (*w)*(*h)*sizeof(unsigned char);

		for(int i=0;i<(*h);i++){
			for(int j = 0;j<(*w);j++){
			//pixmap[(i-1)*(*w)+(j-1)] = bits[(*h - (i-1))*(*w) + (j - 1)];
			pixmap[i*(*w)+j] = bits[(*h - i -1)*(*w) + j];
			}

		}
	}

	// 画像の解放
	FreeImage_Unload(bitmap);

	// FreeImageの終了
	FreeImage_DeInitialise();

	return  pixmap;
}



char nibMask[8] = {
	1, 2, 4, 8, 16, 32, 64, 128
};



unsigned char *ReadXpmPixmap(fp, datafile, w, h, colrs, Colors, CharsPP)
FILE *fp;
char *datafile;
int *w, *h;
XColor *colrs;
int Colors, CharsPP;
{
	unsigned char *pixels;
	char **Color_Vals;
	XColor tmpcolr;
	int i, j, k;
	int /*value,*/ found;
	char line[BUFSIZ], name_and_type[MAX_LINE];
	unsigned char *dataP;
	unsigned char *bitp;
	int tchar;
	char *t;
	char *t2;

	if (Colors == 0)
	{
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr, "Can't find Colors.\n");
		}
#endif

		return((unsigned char *)NULL);
	}
	if (*w == 0)
	{
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr, "Can't read image.\n");
		}
#endif

		return((unsigned char *)NULL);
	}
	if (*h == 0)
	{
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr, "Can't read image.\n");
		}
#endif

		return((unsigned char *)NULL);
	}

	Color_Vals = (char **)malloc(sizeof(char *) * Colors);
	for (i=0; i<Colors; i++)
	{
		tchar = getc(fp);
		while ((tchar != '"')&&(tchar != EOF))
		{
			tchar = getc(fp);
		}
		Color_Vals[i] = (char *)malloc(sizeof(char) * (CharsPP + 1));
		j = 0;
		tchar = getc(fp);
		while ((tchar != '"')&&(tchar != EOF)&&(j < CharsPP))
		{
			Color_Vals[i][j] = (char)tchar;
			tchar = getc(fp);
			j++;
		}
		Color_Vals[i][j] = '\0';
		if (tchar != '"')
		{
			tchar = getc(fp);
			while ((tchar != '"')&&(tchar != EOF))
			{
				tchar = getc(fp);
			}
		}
		tchar = getc(fp);
		while ((tchar != '"')&&(tchar != EOF))
		{
			tchar = getc(fp);
		}
		j = 0;
		tchar = getc(fp);
		while ((tchar != '"')&&(tchar != EOF))
		{
			line[j] = (char)tchar;
			tchar = getc(fp);
			j++;
		}
		line[j] = '\0';
		XParseColor(dsp, (installed_colormap ?
				  installed_cmap :
				  DefaultColormap(dsp, DefaultScreen(dsp))),
			line, &tmpcolr);
		colrs[i].red = tmpcolr.red;
		colrs[i].green = tmpcolr.green;
		colrs[i].blue = tmpcolr.blue;
		colrs[i].pixel = i;
		colrs[i].flags = DoRed|DoGreen|DoBlue;
	}
	for (i=Colors; i<256; i++)
	{
		colrs[i].red = 0;
		colrs[i].green = 0;
		colrs[i].blue = 0;
		colrs[i].pixel = i;
		colrs[i].flags = DoRed|DoGreen|DoBlue;
	}
	tchar = getc(fp);
	while ((tchar != ';')&&(tchar != EOF))
	{
		tchar = getc(fp);
	}

	for ( ; ; )
	{
		if (!(fgets(line, MAX_LINE, fp)))
		{
#ifndef DISABLE_TRACE
			if (srcTrace) {
				fprintf(stderr, "Can't find Pixels\n");
			}
#endif

			return((unsigned char *)NULL);
		}
		else if (sscanf(line,"static char * %s = {",name_and_type) == 1)
		{
			if ((t = strrchr(name_and_type, '_')) == NULL)
			{
				t = name_and_type;
			}
			else
			{
				t++;
			}
			if ((t2 = strchr(name_and_type, '[')) != NULL)
			{
				*t2 = '\0';
			}
			if (!strcmp("pixels", t))
			{
				break;
			}
		}
	}
	pixels = (unsigned char *)malloc((*w) * (*h));
	if (pixels == NULL)
	{
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr, "Not enough memory for data.\n");
		}
#endif

		return((unsigned char *)NULL);
	}

	line[0] = '\0';
	t = line;
	dataP = pixels;
	tchar = getc(fp);
	while ((tchar != '"')&&(tchar != EOF))
	{
		tchar = getc(fp);
	}
	tchar = getc(fp);
	for (j=0; j<(*h); j++)
	{
		for (i=0; i<(*w); i++)
		{
			k = 0;
			while ((tchar != '"')&&(tchar != EOF)&&(k < CharsPP))
			{
				line[k] = (char)tchar;
				tchar = getc(fp);
				k++;
			}
			if ((k == 0)&&(tchar == '"'))
			{
				tchar = getc(fp);
				while ((tchar != '"')&&(tchar != EOF))
				{
					tchar = getc(fp);
				}
				k = 0;
				tchar = getc(fp);
				while ((tchar != '"')&&(tchar != EOF)&&
					(k < CharsPP))
				{
					line[k] = (char)tchar;
					tchar = getc(fp);
					k++;
				}
			}
			line[k] = '\0';
			found = 0;
			for (k=0; k<Colors; k++)
			{
				if (strncmp(Color_Vals[k], line, CharsPP) == 0)
				{
					*dataP++ = (unsigned char)k;
					found = 1;
					break;
				}
			}
			if (found == 0)
			{
#ifndef DISABLE_TRACE
				if (srcTrace) {
					fprintf(stderr, "Invalid Pixel (%2s) in file %s\n", line, datafile);
				}
#endif

				*dataP++ = (unsigned char)0;
			}
		}
	}

	bitp = pixels;
	for (i=0; i<((*w) * (*h)); i++)
	{
		if ((int)*bitp > (256 - 1))
			*bitp = (unsigned char)0;
		bitp++;
	}

	for (i=0; i<Colors; i++)
	{
		free((char *)Color_Vals[i]);
	}
	free((char *)Color_Vals);
	return(pixels);
}


unsigned char *ReadXbmBitmap(fp, datafile, w, h, colrs)
FILE *fp;
char *datafile;
int *w, *h;
XColor *colrs;
{
	char line[MAX_LINE], name_and_type[MAX_LINE];
	char *t;
	char *t2;
	unsigned char *ptr, *dataP;
	int bytes_per_line, version10p, raster_length, padding;
	int i, bytes, temp, value;
	int Ncolors, charspp, xpmformat;
        static unsigned long fg_pixel, bg_pixel;
        static int done_fetch_colors = 0;
        extern XColor fg_color, bg_color;
        extern Widget view;
        extern int Vclass;
	int blackbit;
	int whitebit;

        if (!done_fetch_colors)
          {
            /* First, go fetch the pixels. */
            XtVaGetValues (view, XtNforeground, &fg_pixel,
                         XtNbackground, &bg_pixel, NULL);

            /* Now, load up fg_color and bg_color. */
            fg_color.pixel = fg_pixel;
            bg_color.pixel = bg_pixel;

            /* Now query for the full color info. */
            XQueryColor
              (XtDisplay (view),
               (installed_colormap ?
		installed_cmap :
		DefaultColormap (XtDisplay (view),
                                DefaultScreen (XtDisplay (view)))),
               &fg_color);
            XQueryColor
              (XtDisplay (view),
               (installed_colormap ?
		installed_cmap :
		DefaultColormap (XtDisplay (view),
                                DefaultScreen (XtDisplay (view)))),
               &bg_color);

            done_fetch_colors = 1;

	    /*
	     * For a TrueColor visual, we can't use the pixel value as
	     * the color index because it is > 255.  Arbitrarily assign
	     * 0 to foreground, and 1 to background.
	     */
	    if ((Vclass == TrueColor) ||(Vclass == DirectColor))
	      {
		fg_color.pixel = 0;
		bg_color.pixel = 1;
	      }

          }

        if (get_pref_boolean(eREVERSE_INLINED_BITMAP_COLORS))
          {
            blackbit = bg_color.pixel;
            whitebit = fg_color.pixel;
          }
        else
          {
            blackbit = fg_color.pixel;
            whitebit = bg_color.pixel;
          }

	/*
	 * Error out here on visuals we can't handle so we won't core dump
	 * later.
	 */
	if (((blackbit > 255)||(whitebit > 255))&&(Vclass != TrueColor))
	  {
		fprintf(stderr, "Error:  cannot deal with default colormap that is deeper than 8, and not TrueColor\n");
                fprintf(stderr, "        If you actually have such a system, please notify mosaic-x@ncsa.uiuc.edu.\n");
                fprintf(stderr, "        We thank you for your support.\n");
		exit(1);
	  }

        if (get_pref_boolean(eREVERSE_INLINED_BITMAP_COLORS))
          {
            colrs[blackbit].red = bg_color.red;
            colrs[blackbit].green = bg_color.green;
            colrs[blackbit].blue = bg_color.blue;
            colrs[blackbit].pixel = bg_color.pixel;
            colrs[blackbit].flags = DoRed|DoGreen|DoBlue;

            colrs[whitebit].red = fg_color.red;
            colrs[whitebit].green = fg_color.green;
            colrs[whitebit].blue = fg_color.blue;
            colrs[whitebit].pixel = fg_color.pixel;
            colrs[whitebit].flags = DoRed|DoGreen|DoBlue;
          }
        else
          {
            colrs[blackbit].red = fg_color.red;
            colrs[blackbit].green = fg_color.green;
            colrs[blackbit].blue = fg_color.blue;
            colrs[blackbit].pixel = fg_color.pixel;
            colrs[blackbit].flags = DoRed|DoGreen|DoBlue;

            colrs[whitebit].red = bg_color.red;
            colrs[whitebit].green = bg_color.green;
            colrs[whitebit].blue = bg_color.blue;
            colrs[whitebit].pixel = bg_color.pixel;
            colrs[whitebit].flags = DoRed|DoGreen|DoBlue;
          }

	*w = 0;
	*h = 0;
	Ncolors = 0;
	charspp = 0;
	xpmformat = 0;
	for ( ; ; )
	{
		if (!(fgets(line, MAX_LINE, fp)))
			break;
		if (strlen(line) == (MAX_LINE - 1))
		{
#ifndef DISABLE_TRACE
			if (srcTrace) {
				fprintf(stderr, "Line too long.\n");
			}
#endif

			return((unsigned char *)NULL);
		}
		if (sscanf(line, "#define %s %d", name_and_type, &value) == 2)
		{
			if (!(t = strrchr(name_and_type, '_')))
				t = name_and_type;
			else
				t++;
			if (!strcmp("width", t))
				*w= value;
			if (!strcmp("height", t))
				*h= value;
			if (!strcmp("ncolors", t))
				Ncolors = value;
			if (!strcmp("pixel", t))
				charspp = value;
			continue;
		}
		if (sscanf(line, "static short %s = {", name_and_type) == 1)
		{
			version10p = 1;
			break;
		}
		else if (sscanf(line,"static char * %s = {",name_and_type) == 1)
		{
			xpmformat = 1;
			if (!(t = strrchr(name_and_type, '_')))
				t = name_and_type;
			else
				t++;
			if ((t2 = strchr(name_and_type, '[')) != NULL)
				*t2 = '\0';
			if (!strcmp("mono", t))
				continue;
			else
				break;
		}
		else if (sscanf(line, "static char %s = {", name_and_type) == 1)
		{
			version10p = 0;
			break;
		}
		else if (sscanf(line, "static unsigned char %s = {", name_and_type) == 1)
		{
			version10p = 0;
			break;
		}
		else
			continue;
	}
	if (xpmformat)
	{
		dataP = ReadXpmPixmap(fp, datafile, w, h, colrs, Ncolors, charspp);
		return(dataP);
	}
	if (*w == 0)
	{
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr, "Can't read image.\n");
		}
#endif

		return((unsigned char *)NULL);
	}
	if (*h == 0)
	{
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr, "Can't read image.\n");
		}
#endif

		return((unsigned char *)NULL);
	}
	padding = 0;
	if (((*w % 16) >= 1)&&((*w % 16) <= 8)&&version10p)
	{
		padding = 1;
	}
	bytes_per_line = ((*w + 7) / 8) + padding;
	raster_length =  bytes_per_line * *h;
	dataP = (unsigned char *)malloc((*w) * (*h));
	if (dataP == NULL)
	{
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr, "Not enough memory.\n");
		}
#endif

		return((unsigned char *)NULL);
	}
	ptr = dataP;
	if (version10p)
	{
		int cnt = 0;
		int lim = (bytes_per_line - padding) * 8;
		for (bytes = 0; bytes < raster_length; bytes += 2)
		{
			if (fscanf(fp, " 0x%x%*[,}]%*[ \r\n]", &value) != 1)
			{
#ifndef DISABLE_TRACE
				if (srcTrace) {
					fprintf(stderr, "Error scanning bits item.\n");
				}
#endif

				return((unsigned char *)NULL);
			}
			temp = value;
			value = temp & 0xff;
			for (i = 0; i < 8; i++)
			{
				if (cnt < (*w))
				{
					if (value & nibMask[i])
						*ptr++ = blackbit;
					else
						*ptr++ = whitebit;
				}
				if (++cnt >= lim)
					cnt = 0;
			}
			if ((!padding)||((bytes+2) % bytes_per_line))
			{
				value = temp >> 8;
				for (i = 0; i < 8; i++)
				{
					if (cnt < (*w))
					{
						if (value & nibMask[i])
							*ptr++ = blackbit;
						else
							*ptr++ = whitebit;
					}
					if (++cnt >= lim)
						cnt = 0;
				}
			}
		}
	}
	else
	{
		int cnt = 0;
		int lim = bytes_per_line * 8;
		for (bytes = 0; bytes < raster_length; bytes++)
		{
			if (fscanf(fp, " 0x%x%*[,}]%*[ \r\n]", &value) != 1)
			{
#ifndef DISABLE_TRACE
				if (srcTrace) {
					fprintf(stderr, "Error scanning bits item.\n");
				}
#endif

				return((unsigned char *)NULL);
			}
			for (i = 0; i < 8; i++)
			{
				if (cnt < (*w))
				{
					if (value & nibMask[i])
						*ptr++ = blackbit;
					else
						*ptr++ = whitebit;
				}
				if (++cnt >= lim)
					cnt = 0;
			}
		}
	}
	return(dataP);
}


unsigned char *ReadBitmap(datafile, w, h, colrs, bg)
char *datafile;
int *w, *h;
XColor *colrs;
int *bg;
{    
    unsigned char *bit_data;
    if (!(datafile == NULL)){
	    bit_data = ReadIMG_name(datafile, w, h, colrs);
	    if (bit_data != NULL)
	    {
		    return(bit_data);
	    }
	    return NULL;
    }
    
	/*
    unsigned char *bit_data;
    FILE *fp;

    *bg = -1;

    // Obviously this isn't going to work. 
    if ((datafile == NULL)||(datafile[0] == '\0'))
	{
	    fp = NULL;
	}
    else
	{
	    fp = fopen(datafile, "r");
	}




    if (fp != NULL)
	{

	    bit_data = ReadGIF(fp, w, h, colrs, bg);
	    if (bit_data != NULL)
		{
		    if (fp != stdin) fclose(fp);
		    return(bit_data);
		}
	    rewind(fp);

	    bit_data = ReadXbmBitmap(fp, datafile, w, h, colrs);
	    if (bit_data != NULL)
		{
		    if (fp != stdin) fclose(fp);
		    return(bit_data);
		}
	    rewind(fp);

	    bit_data = ReadXpm3Pixmap(fp, datafile, w, h, colrs, bg);
	    if (bit_data != NULL)
		{
		    if (fp != stdin) fclose(fp);
		    return(bit_data);
		}
	    rewind(fp);

#ifdef HAVE_PNG
///* I can't believe Mosaic works this way... - DXP 
///* I have to put this BEFORE ReadJPEG, because that code
   //screws up the file pointer by closing it if there is an error - go fig. 
	    //bit_data = ReadPNG(fp, w, h, colrs);
	    bit_data = fi_ReadPNG(fp,w,h,colrs);
	    if (bit_data != NULL) // ie. it was able to read the image
		{
		    if (fp != stdin) fclose(fp);
		    return(bit_data);
		}
	    rewind(fp);

#endif
#ifdef HAVE_JPEG
	    bit_data = ReadJPEG(fp, w, h, colrs);
	    if (bit_data != NULL)
		{
		    if (fp != stdin) fclose(fp);
		    return(bit_data);
		}
	    rewind(fp);
#endif
	    bit_data = ReadIMG(fp,w,h,colrs);
	    if (bit_data != NULL) // ie. it was able to read the image
		{
		    //if (fp != stdin) fclose(fp);
		    //if (fp != NULL) fclose(fp);
		    return(bit_data);
		}

	}
    //if ((fp != NULL) && (fp != stdin)) fclose(fp);  // If ReadPNG have err, fp already closed, so it is DOUBLE FREE

#ifndef DISABLE_TRACE
    if(srcTrace){
    fprintf(stderr,"[ReadBitmap] Unable to load img.\n");
    }
#endif
    if (fp != stdin) fclose(fp);


    return((unsigned char *)NULL);
    */
}

