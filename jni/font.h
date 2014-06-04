/*
 * font.h
 *
 *  Created on: 2014年4月15日
 *      Author: e303
 */

#ifndef FONT_H_
#define FONT_H_
#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library library; /* 库的句柄 */
FT_Face face; /* face对象的句柄 */
int initfont();
int freefont();
int setfontsize(int x,int y);
int setftchar(int charcode);
#endif /* FONT_H_ */
