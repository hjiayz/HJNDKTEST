/*
 * font.c

 *
 *  Created on: 2014年4月15日
 *      Author: hjiayz
 */
#include "font.h"
#include <android/log.h>
#define LOGEf(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"font", __VA_ARGS__))

char* fontpath;
int initfont() {
	int error;
	error = FT_Init_FreeType( &library );
	if ( error ) {
		return 1;
		//.初始化错误.//
	}
	if (fontpath==NULL) {
		fontpath="/system/fonts/DroidSansFallback.ttf";
		//LOGEf("%s",fontpath);
	}
	error = FT_New_Face( library,
			fontpath,
			0,
			&face );
//	if ( error == FT_Err_Unknown_File_Format )
	//{
	/* 可以打开和读这个文件，但不支持它的字体格式*/
		//return 2;
	//}
	if ( error )
	{
		return 3;
	//... 其它的错误码意味着这个字体文件不能打开和读，或者简单的说它损坏了...//
	}
	return 0;
}
int setfontsize(int x,int y) {
	return FT_Set_Pixel_Sizes(
	face, /* face对象句柄 */
	x, /* 象素宽度 */
	y); /* 象素高度 */
}
int setftchar(int charcode) {
	return FT_Load_Char(
			face, /* face对象的句柄 */
			charcode, /* 字形索引 */
			FT_LOAD_RENDER); /* 装载标志，参考下面 */
	/*FT_LOAD_MONOCHROME,单色位图，FT_LOAD_RENDER多重采样抗锯齿，FT_LOAD_DEFAULT默认方式*/
}
int freefont() {
	FT_Done_Glyph(face->glyph);
	face->glyph=NULL;
	FT_Done_Face(face);
	face=NULL;
	FT_Done_FreeType(library);
	library=NULL;
	return 0;
}
