#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <psptypes.h>
#include <pspctrl.h>
#include <pspiofilemgr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/unistd.h>

//#include "../PspLib/syscall.h"
#include "../PspLib/pg.h"
//#include "kor_font.h"
//#include "eng_font.h"
#include "spe_font.h"
//#include "fnt_gulimche.h"
//#include "kor_font_def.h"
#include "han_lib.h"
#include "converters.h"

extern int done;
unsigned char draw_font[1327105];
//unsigned short draw_font[663553];

// return memory block position
unsigned long gfLoadFontFile(char *FilePath, unsigned char *dest, unsigned long size)
{
	int dFd, dFileSize;

	dFd=sceIoOpen(FilePath, PSP_O_RDONLY, 0777);
	//size = 1024;// sceIoLseek(dFd, 0, 1);
	//sceIoLseek(dFd, 0, 0);
	//hBlock = sceKernelAllocPartitionMemory(2, "block", 1, size, 0); 
	//dest = (int *)sceKernelGetBlockHeadAddr(hBlock);
	dFileSize = sceIoRead(dFd, dest, size); 
	sceIoClose(dFd);
	dest[size] = 0;
	return dFileSize;
}

void gfInitFont(char* initPath)
{
	char font_path[512];
	strcpy(font_path, initPath);
	strcat(font_path,  "font/gulimche.fnt");
//	gfLoadFontFile(font_path, (unsigned char*)draw_font, 1769472);
	gfLoadFontFile(font_path, (unsigned char*)draw_font, 1327105);
	//gfLoadFile("Kor_s.fnt", (unsigned char*)kor_font, 11520);
}

// return size.
int ufEuc_kr2Unicode (unsigned int * dest, unsigned char* src)
{
	int index = 0;
	int size = strlen((const char *)src);
	int i;
	unsigned char * cP = src;
	unsigned short code;
		
	for (i = 0; i < size ; i ++)
	{
		if  (0x0d <= cP[i] && cP[i] < 128 ) {
			euc_kr_mbtowc(dest + index++, cP + i, 2);
		}
		else if  ('\t' == cP[i]) {
			euc_kr_mbtowc(dest + index++, cP + i, 2);
		}
		
		else if ( cP[i] >=  128 && cP[i+1] != 0 ){
			euc_kr_mbtowc(dest + index++, cP + i, 2);
			i++;	
		}
		
	} 
	return index;
}

// 유니코드 문자 배열을 숫자의 배열로 변경한다.
int ufUnicode2Buffer(unsigned int * dest, unsigned char* src)
{
	int index = 0;
	int size = strlen(src);
	unsigned char * cP = src;
	int i;
	
	for (i = 0; i < size ; i ++)
	{
		dest[index++] = *(cP + i) *1 + *(cP + i + 1) * 256;
		//euc_kr_mbtowc(dest + index++, cP + i, 2);
		i++;	
	} 
	return index;	
}

int ufUtf8Unicode (unsigned int * dest, unsigned char* src)
{
	int index = 0;
	int size = strlen(src);
	int i;
	int ret;
	unsigned char * cP = src;
	unsigned short code;
		
	for (i = 0; i < size  ; i ++)
	{
		ret = utf8_mbtowc(dest + index ++, cP + i, size - i);
		if (ret >= 2)
			i += ret - 1;
		else	if (ret <= 0 && index > 0)
			index --;
	} 
	return index;
}


int ufUtf16Unicode (unsigned int * dest, unsigned char* src)
{
	int index = 0;
	int size = strlen(src);
	int i;
	int ret;
	unsigned char * cP = src;
	unsigned short code;
	
	while(1)	
	//for (i = 0; i < size  ; i ++)
	{
		ret = utf16_mbtowc(dest + index ++, cP + i, size - i);
		if (ret >= 1)
			i += ret;
		else	
			break;
			
	} 
	return index;
}


void ufUnicodePrintString(int x, int y, 	unsigned long color, unsigned int * data)
{
	int   cursorX = x;
	int   cursorY = y;
	int i; unsigned int code;
	for ( i = 1; i <= data[0]; i ++){
		code = data[i];
		
		if (code == 0x0020 )
			cursorX += GLAN_FONT_DRAW_W;
		else if  (code == (unsigned int)'\t')
			cursorX += GLAN_FONT_DRAW_W * 4;
 		else if ( 0x000d < code &&  code < 256){
			gfUnicodePutKorChar(cursorX,cursorY,code,color,0,0);
			cursorX += GLAN_FONT_DRAW_W;
		}else if (code >= 256 && code < 55296){
			gfUnicodePutKorChar(cursorX,cursorY,code,color,0,0);
			cursorX += GLAN_FONT_DRAW_W*2;
		}

	    if (code ==  (unsigned int)'\n') // 0x000d
	    {
	        cursorX = x;
	        cursorY += GLAN_FONT_DRAW_H + 3;
	    }else if (cursorX > 457 ){
			cursorX = x;
			cursorY += GLAN_FONT_DRAW_H + 3;
		}
	}
}

/*void ufUnicodePrintString (unsigned long x, unsigned long y,	unsigned long color, unsigned char *string) 
{
	int   cursorX;
	int   cursorY;
	unsigned char* cP = string;
	int   index;
	
	unsigned int code;
	
	cursorX = x;// * 12;
	cursorY = y;// * 14;

// Parse the String
	int offset = 0;
	while (offset < 30) //*cP != '\0')
	{ 
		offset ++;
		code = *(cP+ 0)  +  *(cP + 1)  * 256;
		cP+= 2;	
		//cP++;	
		if (code == 0x0020 )
			cursorX += GLAN_FONT_DRAW_W;
		else if  (code == (unsigned int)'\t')
			cursorX += GLAN_FONT_DRAW_W * 4;
 		else if ( 0x000d < code &&  code < 256){
			gfUnicodePutKorChar(cursorX,cursorY,code,color,0,0);
			cursorX += GLAN_FONT_DRAW_W;
		}else if (code >= 256 && code < 55296){
			gfUnicodePutKorChar(cursorX,cursorY,code,color,0,0);
			cursorX += GLAN_FONT_DRAW_W*2;
		}

	    if (code == 0x000d)
	    {
	        cursorX = x;
	        cursorY += GLAN_FONT_DRAW_H + 3;
	    }else if (cursorX > 454 ){
			cursorX = x;
			cursorY += GLAN_FONT_DRAW_H + 3;
		}
	
	//if (cursorY > 470)
	//	break;
	} 
}*/


// euc-kr 문자열을 유니코드로 변환 하여 출력한다.
void ufPrintString (int x, int y, unsigned long color, unsigned char *string) 
{
	unsigned int data[2048];
	data[0] = ufEuc_kr2Unicode(data + 1, string);
	ufUnicodePrintString(x, y + 1, color, data);
}



void ufUtf16PrintString (int x, int y, unsigned long color, unsigned char *string) 
{
	unsigned int data[2048];
	data[0] = ufUtf16Unicode(data + 1, string);
	ufUnicodePrintString(x, y + 1, color, data);
}


/*void ufPrintString (unsigned long x, unsigned long y,	unsigned long color, unsigned char *string) 
{
 	int   cursorX;
    int   cursorY;
    char* cP = string;
    int   index;

    unsigned  code;
    GLANbyte  codeInitial,  codeMedial,  codeFinal;
    GLANbyte  indexInitial, indexMedial, indexFinal;

 	cursorX = x;// * 12;
    cursorY = y;// * 14;

// Parse the String
    while (*cP != '\0')
    {
        // Visible characters
        if ( ' ' == *cP ) //&& !(mode & GLAN_TEXTBOX_DRAW_SPACES))
        {
            cursorX += GLAN_FONT_DRAW_W;
        }
        // drawing english
        else if ( ' ' <= *cP && *cP <= '~' )
        {
            index = *cP;
            gfPutEngChar(cursorX,cursorY,index,color,0,0);
            cursorX += GLAN_FONT_DRAW_W;
        }
        // drawing hangul
        else if ( *cP < 0 && *(cP+1) != 0 )
        {
            // to do. 
		    code = glanWord( *cP, *(cP+1) );
			 code = ConvertExtKS_( code );
           cP++;
            
            codeInitial = glantCodeFont_[0][glanGetHanInitial(code)];
            codeMedial  = glantCodeFont_[1][glanGetHanMedial (code)];
            codeFinal   = glantCodeFont_[2][glanGetHanFinal  (code)];
            
            indexFinal  = glantFormFinal_[codeMedial];
            
            if ( !codeFinal )
            {
                indexInitial = glantFormInitial_[0][codeMedial];
                indexMedial  = glantFormMedial_ [0][codeInitial];
            }
            else
            {
                indexInitial = glantFormInitial_[1][codeMedial];
                indexMedial  = glantFormMedial_ [1][codeInitial];
            }
            gfPutKorChar(cursorX,cursorY, 128 - 128 + indexInitial*20 + codeInitial , color,0,0);
			gfPutKorChar(cursorX,cursorY, 288 - 128 + indexMedial *22 + codeMedial  , color,0,0);
			gfPutKorChar(cursorX,cursorY, 376 - 128 + indexFinal  *28 + codeFinal   , color,0,0);
			    
            cursorX += GLAN_FONT_DRAW_W*2;
        }
        
        if ( *cP == '\n')
        {
            cursorX = x;
            cursorY += GLAN_FONT_DRAW_H + 3;
            //if (maxFlag && cursorY + GLAN_FONT_DRAW_H > maxY) break;
        }else if (cursorX > 454 ){
			cursorX = x;
			cursorY += GLAN_FONT_DRAW_H + 3;
		}
	
        cP++;
		//if (cursorY > 470)
		//	break;
    }
}*/

/*void gfPutEngChar(unsigned long x,unsigned long y, int index,int col,int backcol,int fill)
{
	unsigned short *vr;
	unsigned char *font;
	unsigned char pt;
	int x1,y1;

	if (index * 16 >= 4096) return;
	//if (x > 480 || y > 272) return;

	// mapping
	font = (unsigned char *)(eng_font + index * 16); 

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y + 1);
	for(y1=0;y1<16;y1++) {
		pt = *font++;
		for(x1=0;x1<8;x1++) {
			if (pt & 0x80) {
				*vr = col;
			} else {
				if (fill) *vr = backcol;
			}
			vr++;
			pt = pt << 1;
		}
		vr += LINESIZE - 8; 
	}
}*/

void gfUnicodePutKorChar(unsigned long x,unsigned long y, int index,int col,int backcol,int fill)
{
	unsigned short *vr;
	unsigned short fnt;
	unsigned char *font;
	unsigned short pt;
	int x1,y1;

	if (index * 24 >= 1769472 || index < 0 ) return;
	//if (x > 480 || y > 272) return;

	// mapping
	font = (unsigned char *)(draw_font + index * 24); // * 16;

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<12;y1++) {
		pt =  font[0] * 256 + font[1];	
		font = font + 2;
		for(x1=0;x1<16;x1++) {
			if (pt & 0x8000) {
				*vr = col;
			} else {
				if (fill) *vr = backcol;
			}
			vr++;
			pt = pt << 1;
		}
		vr += LINESIZE - 16; 
	}
}


/*void gfPutKorChar(unsigned long x,unsigned long y, int index,int col,int backcol,int fill)
{
	unsigned short *vr;
	unsigned short fnt;
	unsigned char *font;
	unsigned short pt;
	int x1,y1;

	if (index * 32 >= 11520) return;
	//if (x > 480 || y > 272) return;

	// mapping
	font = (unsigned char *)(kor_font+index * 32); // * 16;

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<16;y1++) {
		pt =  font[0] * 256 + font[1];	
		font = font + 2;
		for(x1=0;x1<16;x1++) {
			if (pt & 0x8000) {
				*vr = col;
			} else {
				if (fill) *vr = backcol;
			}
			vr++;
			pt = pt << 1;
		}
		vr += LINESIZE - 16; 
	}
}*/

void gfPutSpeChar(unsigned long x,unsigned long y, int index,int col,int backcol,int fill)
{
	unsigned short *vr;
	unsigned short fnt;
	unsigned char *font;
	unsigned short pt;
	int x1,y1;

	if (index * 32 >= 640) return;
	//if (x > 480 || y > 272) return;

	// mapping
	font = (unsigned char *)(spe_font+index * 32); // * 16;

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<16;y1++) {
		pt =  font[0] * 256 + font[1];	
		font = font + 2;
		for(x1=0;x1<16;x1++) {
			if (pt & 0x8000) {
				*vr = col;
			} else {
				if (fill) *vr = backcol;
			}
			vr++;
			pt = pt << 1;
		}
		vr += LINESIZE - 16; 
	}
}
/*
GLANword  ConvertExtKS_      ( GLANword Code )
{

#if defined(__GLAN_EXTEND_KS)
        GLANword newCode;
        GLANword index;
        GLANbyte high, low;

        newCode = ConvertKS_( Code );
        if( newCode == 0 )
        {
            high = glanHighByte(Code);
            low  = glanLowByte (Code);
            
            if      ( high >= 0x81 && high <= 0xA0 )
            {
                index = (high - 0x81) * 178;
                
                if      ( low >= 0x41 && low <= 0x5A )  index += (low - 0x41);
                else if ( low >= 0x61 && low <= 0x7A )  index += (low - 0x61 + 26);
                else                                    index += (low - 0x81 + 52);
            }
            else if ( high >= 0xA1 && high <= 0xC5 )
            {
                index = (high - 0xA1) * 84 + 5696;
                
                if      ( low >= 0x41 && low <= 0x5A )  index += (low - 0x41);
                else if ( low >= 0x61 && low <= 0x7A )  index += (low - 0x61 + 26);
                else                                    index += (low - 0x81 + 52);
            }
            else if ( high == 0xC6 && low >= 0x41 && low <= 0x52 )
                index = (low - 0x41 + 8804);
                
            if ( index >= 8822 ) return 0;
            
            newCode = glanGetCodeEXTKS(index);
        }
        
        return newCode;
        

#else    // __GLAN_EXTEND_KS

        return ConvertKS_( Code );
        
#endif   // __GLAN_EXTEND_KS

}




GLANword ConvertKS_ ( GLANword code )
{
        GLANbyte high, low;
        GLANbyte temp;
        int      index;
        int      mod, rem;

        high = glanHighByte(code);
        low  = glanLowByte (code);

        if ( high == 0xa4 && low > 0xa0 && low < 0xd4 )
        {   //  낱글자인 경우 
            return (glantSingleKSSM_[low - 0xa1]);
        }
        else if ( high >= 0xa1 && high <= 0xac )
        {   //  특수 문자 변환1
        
            if ( low < 0xa1 || low > 0xfe )  return 0;
            
            mod = (high - 0xa1) >> 1;
            rem = (high - 0xa1) & 0x01;
            
            if ( rem ) temp = low;
            else
            {
                temp = low - 0x70;
                if ( temp > 0x7e ) temp += 18;
            }
            
            return (((mod + 0xd9) << 8) + temp);
        }
        else if( high == 0xad )
        {   //  특수 문자 변환2
        
            if ( low < 0xa1 || low > 0xfe )  return 0;
            temp = low - 0x21;
            
            return ((0xd4 << 8) + temp);
        }
        else if( high == 0xae )
        {   //  특수 문자 변환3
        
            if( low < 0xa1 || low > 0xc1 )
            {
                if( low == 0xc2 )  return 0xd4ff;
                
                return 0;
            }
            
            temp = low + 0x3d;

            return((0xd4 << 8) + temp);
        }
        else if ( high >= 0xb0 && high <= 0xc8 )
        {   //  한글 변환
        
            if ( low < 0xa1 || low > 0xfe ) return(0);
            
            index = (high - 0xb0) * 94 + low - 0xa1;
     
            return glanGetCodeKS(index);
        }
        else if ( high >= 0xca && high <= 0xfd )
        {   //  한자 변환
        
            if ( low < 0xa1 || low > 0xfe ) return(0);
            
            mod = (high - 0xca) >> 1;
            rem = (high - 0xca) & 0x01;
            
            if ( rem ) temp = low;
            else
            {
                temp = low - 0x70;
                if( temp > 0x7e ) temp += 18;
            }
			// 현재 한문 처리가 불가능해서.. 0으로 리턴한다.
			return 0;
            //return (((mod + 0xe0) << 8) + temp);
        }

        index = ((code >> 8) - 0xb0) * 94 + (code & 0x00ff) - 0xa1;
        if ( index < 0 || index >= 2350 )  return 0;
        
        return glanGetCodeKS(index);
}
*/

void _gfDrawButton(int xPos, int yPos, char* message, int buttonIndex)
{
	int width = 80;
	int height = 18;
	gfDrawFillBox(xPos, yPos, xPos + width, yPos + height, pgRgb2col(77,97,133));// yPos  + height, 
	
	gfDrawBox(xPos, yPos, xPos + width, yPos +  height, pgRgb2col(0,0,0));
	gfDrawLine(xPos + 1, yPos + 1, xPos + width  - 1, yPos + 1, pgRgb2col(192,192,192));
	gfDrawLine(xPos + 1, yPos + 1, xPos + 1, yPos + height - 1, pgRgb2col(192,192,192));
	
	gfDrawLine(xPos + width - 1, yPos + 1, xPos + width  - 1, yPos + height - 1, pgRgb2col(97,104,83));
	gfDrawLine(xPos +  1, yPos + height - 1, xPos + width - 1, yPos + height - 1, pgRgb2col(97,104,83));
	if (buttonIndex == 1 )
    	gfPutSpeChar ( xPos + 8, yPos + 4,  buttonIndex, pgRgb2col(247,164,157), 0, 0);
    else if (buttonIndex == 4 )
	    gfPutSpeChar ( xPos + 8, yPos + 4, 4,  pgRgb2col(225,227,223), 0, 0);
	
    		
	ufPrintString ( xPos + 24, yPos + 4,  pgRgb2col(255,255,255), message);
}

// okmode == 1     display 0k, cancel button and wait key
//                 0    not display.
int gfHanMessageBox(char * message1,  char* message2, int okmode)
{
	int width = 300;
	int height =  120;
	int xPos =(int)( (SCREEN_WIDTH - width) / 2);
	int yPos = (int)( (SCREEN_HEIGHT - height) / 2);
	unsigned long key;
	
	// back.
	gfDrawFillBox(xPos, yPos, xPos + width, yPos + 18, pgRgb2col(77,97,133));// yPos  + height, 
	gfDrawFillBox(xPos, yPos + 18, xPos + width, yPos  + height, pgRgb2col(194,211,252));// yPos  + height, 
	
	//border
	gfDrawBox(xPos, yPos, xPos + width, yPos +  height, pgRgb2col(0,0,0));
	gfDrawLine(xPos + 1, yPos + 1, xPos + width  - 1, yPos + 1, pgRgb2col(192,192,192));
	gfDrawLine(xPos + 1, yPos + 1, xPos + 1, yPos + height - 1, pgRgb2col(192,192,192));
	
	gfDrawLine(xPos + width - 1, yPos + 1, xPos + width  - 1, yPos + height - 1, pgRgb2col(97,104,83));
	gfDrawLine(xPos +  1, yPos + height - 1, xPos + width - 1, yPos + height - 1, pgRgb2col(97,104,83));
	
	ufPrintString ( xPos + 4, yPos + 3,  pgRgb2col(255,255,255), "확인");
	ufPrintString ( xPos + 10, yPos + 24,  pgRgb2col(0,0,0), message1);
	ufPrintString ( xPos + 10, yPos + 24 + 16,  pgRgb2col(0,0,0), message2);
	
	// draw box
	if (okmode == 1 ){
		_gfDrawButton( xPos + 60, yPos + height - 30, " : 확인", 1);
		_gfDrawButton( xPos + 160, yPos + height - 30, " : 취소", 4);
	}
	
//	pgScreenFlipV();
	pgUpdateScreen();
	if (okmode == 1 ){
//		while (1){
		while (!done)
		{
			key = pgGetKey();
			if (key & PSP_CTRL_CIRCLE) 
				return 1;
			else if (key & PSP_CTRL_CROSS)
				return 0;
		}
	}
	return 0;
}
