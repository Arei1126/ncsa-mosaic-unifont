#include "../config.h"
#include "mosaic.h"
#include <FreeImage.h>
#include <X11/Xos.h>
#include "readIMG.h"

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



