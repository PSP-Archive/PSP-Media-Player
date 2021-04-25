#ifndef me_h
#define me_h

struct me_struct
	{
	int start;
	int end;
	void (*func)(int);
	int param;
	};


void me_startproc(u32 func, u32 param);
struct me_struct *me_struct_init();
void me_start(volatile struct me_struct *nocache, int func, int param);
void me_wait(volatile struct me_struct *nocache);
void me_function(int p);
void me_sceKernelDcacheWritebackInvalidateAll();

#endif
