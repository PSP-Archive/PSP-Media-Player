#ifndef _HAN_LIB_H
#define _HAN_LIB_H

void gfInitFont(char* initPath);
void ufUnicodePrintString(int x, int y, 	unsigned long color, unsigned int * data);
//void ufUnicodePrintString (unsigned long x, unsigned long y,	unsigned long color, unsigned char *string) ;
void gfUnicodePutKorChar(unsigned long x,unsigned long y, int index,int col,int backcol,int fill);

int ufUnicode2Buffer(unsigned int * dest, unsigned char* src);

void ufPrintString (int x, int y, unsigned long color, unsigned char *string);
//void gfPutEngChar(unsigned long x,unsigned long y, int index,int col,int backcol,int fill);
//void gfPutKorChar(unsigned long x,unsigned long y, int index,int col,int backcol,int fill);
void gfPutSpeChar(unsigned long x,unsigned long y, int index,int col,int backcol,int fill);

int gfHanMessageBox(char * message1,  char* message2, int okmode);


#endif

