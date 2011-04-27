typedef unsigned char u_char;
typedef unsigned short u_short;

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef signed char s8;

extern u16 memory[];

extern u16 regs[];
#define pc (regs[7])
extern u16 psw;
