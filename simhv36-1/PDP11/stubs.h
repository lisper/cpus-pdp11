void simh_record_mem_read_word(unsigned int pa, unsigned short data);
void simh_record_io_read_word(unsigned int pa, unsigned short data);
void simh_record_mem_write_word(unsigned int pa, unsigned short data);
void simh_record_io_write_word(unsigned int pa, unsigned short data);
void simh_record_mem_read_byte(unsigned int pa, unsigned short data);
void simh_record_io_read_byte(unsigned int pa, unsigned short data);
void simh_record_mem_write_byte(unsigned int pa, unsigned short data);
void simh_record_io_write_byte(unsigned int pa, unsigned short data);
void simh_report_pc(int PC, int IR);

