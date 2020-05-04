/*
	la32 rom table generator, by Jonathan Gevaryahu
	license is 3bsd
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifdef PI
#undef PI
#endif
#define PI 3.141592653589793f

#define NUM_ARGS 2

// from MAME, 3bsd
#define BIT(x,n) (((x)>>(n))&1)

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B7) << 7) | \
		 (BIT(val,B6) << 6) | \
		 (BIT(val,B5) << 5) | \
		 (BIT(val,B4) << 4) | \
		 (BIT(val,B3) << 3) | \
		 (BIT(val,B2) << 2) | \
		 (BIT(val,B1) << 1) | \
		 (BIT(val,B0) << 0))

#define BITSWAP16(val,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))


int main(int argc, char **argv)
{
	if (argc != NUM_ARGS)
	{
		fprintf(stderr,"Invalid number of arguments: need %d, found %d\n", NUM_ARGS-1, argc-1);
		fprintf(stderr,"Usage: exename outputfile.bin\n");
		return 1;
	}

	FILE *out = fopen(argv[1], "wb");
	if (!out)
	{
		fprintf(stderr,"E* Unable to open output file %s!\n", argv[1]);
		return 1;
	}

	// create the sine table; there are 13 data bits, and 64*8 (512) addresses
	int16_t sineTable[512];
	sineTable[0] = 8191;
	for (int i = 1; i < 512; i++)
	{
		sineTable[i] = round(-log(sin((i+0.5)*PI/512/2))/log(2)*1024); // same equation as opl2 and 3, but twice the entries so step size is halved
	}
	// check the first 8 values
	int16_t cSineTable[8] = { 8191, 7950, 7195, 6698, 6327, 6030, 5784, 5572 };
	
	for (int i = 0; i < 8; i++)
	{
		if (sineTable[i] != cSineTable[i])
		{
			int16_t errorAmt = sineTable[i] - cSineTable[i];
			fprintf(stderr,"Error of %d for value %d (expected %d found %d), percentage of %f\n", errorAmt, i, cSineTable[i], sineTable[i], (float)errorAmt/8192.0f);
		}
	}
	// draw the sine table
	fprintf(out,"LA32 Quarter-Sine Table:\n");
	for (int y = 0; y < 64; y++) // row, 6 bits
	{
		for (int x = 0; x < 104; x++) // column, 7 bits
		{
			if ((x%8)==0) fprintf(out, "|");
			/* sine rom address is 9 bits:
			 876543210
			 ||||||\\\- from x bits 2,1,0
			 \\\\\\---- from y bits 5,4,3,2,1,0
			 bits 6,5,4,3 of x choose which data bit is selected
			*/
			int bitnum = x>>3; // 'bit column number'
			int sineEntry = (y<<3)|(x&7);
			if BIT(sineTable[sineEntry], bitnum)
				fprintf(out, "X");
			else
				fprintf(out, ".");
		}
		fprintf(out, "\n");
	}

	fprintf(out, "\n");

	// create the EXP table, there are 12 data bits, and 128*4 (512) addresses
	int16_t expTable[512];
	for (int i = 0; i < 512; i++)
	{
		expTable[i] = round( (1- pow(2,(-(i+1)/512.0f)))*8192.0f-1);
	}
	// check the first 4 values
	int16_t cExpTable[4] = { 10, 21, 32, 43 };
	for (int i = 0; i < 4; i++)
	{
		if (expTable[i] != cExpTable[i])
		{
			int16_t errorAmt = expTable[i] - cExpTable[i];
			fprintf(stderr,"Error of %d for value %d (expected %d found %d), percentage of %f\n", errorAmt, i, cExpTable[i], expTable[i], (float)errorAmt/8192.0f);
		}
	}
	// create the EXP delta table
	int16_t expDeltaTable[512];
	expDeltaTable[0] = 4; // this is right...
	for (int i = 1; i < 512; i++)
	{
		expDeltaTable[i] = 15-(expTable[i]-expTable[i-1]);
	}
	// draw the EXP table
	fprintf(out,"LA32 EXP+delta Table:\n");
	for (int y = 0; y < 128; y++) // row, 7 bits
	{
		for (int x = 0; x < 64; x++) // column, 6 bits
		{
			if ((x%8)==0) fprintf(out, "|");
			if (x < 16) // delta
			{
				int bitnum = (3-(x>>2))&3;
				int expDelEntry = (y<<2)|(x&3);
				if BIT(expDeltaTable[expDelEntry], bitnum)
					fprintf(out, "X");
				else
					fprintf(out, ".");
			}
			else // EXP table
			{
				int bitnum = 11-((x-16)>>2);
				int expEntry = (y<<2)|(x&3);
				if BIT(expTable[expEntry], bitnum)
					fprintf(out, "X");
				else
					fprintf(out, ".");
			}
		}
		fprintf(out, "\n");
	}

	fclose(out);
	fprintf(stderr,"Finished successfully.\n");

	// cleanup
	return 0;
}