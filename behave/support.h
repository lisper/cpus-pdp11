/* support.h */

#define IOPAGEBASE	017760000

#define IOBASE_TTI	(IOPAGEBASE + 017560)		/* DL11 rcv */
#define IOBASE_TTO	(IOPAGEBASE + 017564)		/* DL11 xmt */
#define VECTOR_TTI	0060
#define VECTOR_TTO	0064
#define IPL_TTI		4
#define IPL_TTO		4

#define IOBASE_CLK	(IOPAGEBASE + 017546)		/* KW11L */
#define VECTOR_CLK	0100
#define IPL_CLK		6

#define IOBASE_PCLK	(IOPAGEBASE + 012540)		/* KW11P */
#define VECTOR_PCLK	0100
#define IPL_PCLK	6

#define IOBASE_RK	(IOPAGEBASE + 017400)		/* RK11 */
#define VECTOR_RK	0220
#define IPL_RK		5

#define IOBASE_SR	(IOPAGEBASE + 017570)		/* SR */

#define IOBASE_PSW	(IOPAGEBASE + 017776)		/* PSW */

u16 io_read(u22 addr);
void io_write(u22 addr, u16 data, int writeb);
