/* mem.h */


/* read with byte address */
u16 read_mem(u22 addr);
void write_mem(u22 addr, u16 data);

/* for debug only */
u16 raw_read_memory(u22 addr);
void raw_write_memory(u22 addr, u16 data);
