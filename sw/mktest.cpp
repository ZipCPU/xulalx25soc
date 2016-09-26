#include <stdio.h>

#define	BUFSZ	512

int main(int argc, char **argv) {
	short	sbuf[BUFSZ];
	int	dbuf[BUFSZ], nr;
	FILE	*fpin, *fpout;

	fpin = fopen(argv[1], "r");
	fpout = fopen(argv[2], "w");

	while((nr=fread(sbuf, sizeof(short), BUFSZ, fpin))>0) {
		for(int i=0; i<BUFSZ; i++) {
			int	v = (unsigned short)sbuf[i];
			v |= (v << 16);
			v ^= 0x05555;
			dbuf[i] = v;
		}

		fwrite(dbuf, sizeof(int), BUFSZ, fpout);
	}

	fclose(fpin);
	fclose(fpout);
}

