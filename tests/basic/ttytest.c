
unsigned char *tpb = (unsigned char *)0177566;
signed char *tps = (unsigned char *)0177564;
unsigned char *kpb = (unsigned char *)0177562;
signed char *kps = (unsigned char *)0177560;

void tpchr(ch)
char ch;
{
	while (*tps > 0);
	*tpb = ch;
}

int kpchr()
{
	while (*kps > 0);
	return *kpb;
}

char msg[] = "\r\nHello world!\r\n";

main()
{
	char *msg;
	char ch;

	while (ch = *msg++) {
		tpchr(ch);
	}
	while (1) {
		ch = kpchr();
		tpchr(ch);
	}
}
