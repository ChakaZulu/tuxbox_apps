/*
   (c) rasc


*/



unsigned long getBits (u_char *buf, int byte_offset, int startbit, int bitlen);

void print_name (int verbose, u_char *b, u_int len);
void print_name2 (int verbose, u_char *b, u_int len);
void print_time40 (int verbose, u_long mjd, u_long utc);

long str2i (char *s);


