/*
 *  $Id: cpuid-amd.c,v 1.24 2001/08/10 11:34:57 davej Exp $
 *  This file is part of x86info.
 *  (C) 2001 Dave Jones.
 *
 *  Licensed under the terms of the GNU GPL License version 2.
 *
 *  AMD-specific information
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "x86info.h"

extern int show_registers;
extern int show_cacheinfo;
extern int show_msr;
extern int show_bluesmoke;

static void do_assoc(unsigned long assoc)
{
	if ((assoc & 0xff) == 255)
		printf ("Fully");
	else
		printf ("%ld-way", assoc);
	printf (" associative. ");
}

static void decode_AMD_cacheinfo(int cpunum, unsigned long maxei)
{
	unsigned long eax, ebx, ecx, edx;

	if (maxei >= 0x80000005) {
		/* TLB and cache info */
		cpuid (cpunum, 0x80000005, &eax, &ebx, &ecx, &edx);

		printf ("Instruction TLB: ");
		do_assoc ((ebx >> 8) & 0xff);
		printf ("%ld entries.\n", ebx & 0xff);

		printf ("Data TLB: ");
		do_assoc (ebx>>24);
		printf ("%ld entries.\n", (ebx >> 16) & 0xff);

		printf ("L1 Data cache:\n\t");
		printf ("Size: %ldKb\t", ecx >> 24);
		do_assoc ((ecx >> 16) & 0xff);
		printf ("\n\t");
		printf ("lines per tag=%ld\t", (ecx >> 8) & 0xff);
		printf ("line size=%ld bytes.\n", ecx & 0xff);

		printf ("L1 Instruction cache:\n\t");
		printf ("Size: %ldKb\t", edx >> 24);
		do_assoc ((edx >> 16) & 0xff);
		printf ("\n\t");
		printf ("lines per tag=%ld\t", (edx >> 8) & 0xff);
		printf ("line size=%ld bytes.\n", edx & 0xff);
	}

	/* check K6-III (and later?) on-chip L2 cache size */
	if (maxei >= 0x80000006) {
		cpuid (cpunum, 0x80000006, &eax, &ebx, &ecx, &edx);
		printf ("L2 (on CPU) cache:\n\t");
		printf ("Size %ldKb\t", ecx >> 16);
		do_assoc ((ecx >> 12) & 0x0f);
		printf ("\n\t");
		printf ("lines per tag=%ld\t", (ecx >> 8) & 0x0f);
		printf ("line size=%ld bytes.\n", ecx & 0xff);
	}
	printf ("\n");
}


static void dump_extended_AMD_regs(int cpunum, unsigned long maxei)
{
	unsigned long eax, ebx, ecx, edx;
	unsigned int i;

	/* Dump extended info in raw hex */
	for (i = 0x80000000; i <= maxei; i++) {
		cpuid (cpunum, i, &eax, &ebx, &ecx, &edx);
		printf ("eax in: 0x%x, eax = %08lx ebx = %08lx ecx = %08lx edx = %08lx\n", i, eax, ebx, ecx, edx);
	}
	printf ("\n");
}

static void dump_athlon_MSR(int cpunum)
{
	if (getuid()!=0) {
		printf ("Need to be root to access MSRs.\n");
		return;
	}

	printf("\t\t\t\t31       23       15       7 \n");
	dumpmsr(cpunum, 0x2A);
	dumpmsr(cpunum, 0xC0000080);
	dumpmsr(cpunum, 0xC0010010);
	dumpmsr(cpunum, 0xC0010015);
	dumpmsr(cpunum, 0xC001001B);
	printf ("\n");
}


void Identify_AMD (unsigned int maxi, struct cpudata *cpu)
{
	char *nameptr;
	unsigned long eax, ebx, ecx, edx;

	nameptr = cpu->name;

	cpu->vendor = VENDOR_AMD;

	if (maxi >= 0x00000001) {
		cpuid (cpu->number, 0x00000001, &eax, &ebx, &ecx, &edx);
		cpu->stepping = eax & 0xf;
		cpu->model = (eax >> 4) & 0xf;
		cpu->family = (eax >> 8) & 0xf;

		switch (cpu->family) {
		case 4:
			switch (cpu->model) {
				case 3:	sprintf (cpu->name, "%s", "Am486DX2-WT");	break;
				case 7:	sprintf (cpu->name, "%s", "Am486DX2-WB");	break;
				case 8:	sprintf (cpu->name, "%s", "Am486DX4-WT / Am5x86-WT");	break;
				case 9:	sprintf (cpu->name, "%s", "Am486DX4-WB / Am5x86-WB");	break;
				case 10:sprintf (cpu->name, "%s", "Elan SC400");	break;
				case 0xe:sprintf (cpu->name, "%s", "Am5x86-WT");	break;
				case 0xf:sprintf (cpu->name, "%s", "Am5x86-WB");	break;
				default:sprintf (cpu->name, "%s", "Unknown CPU");	break;
			}
			break;

		case 5:
			switch (cpu->model) {
				case 0:	sprintf (cpu->name, "%s", "SSA5 (PR75/PR90/PR100)");	break;
				case 1:	sprintf (cpu->name, "%s", "K5 (PR120/PR133)");			break;
				case 2:	sprintf (cpu->name, "%s", "K5 (PR166)");				break;
				case 3:	sprintf (cpu->name, "%s", "K5 (PR200)");				break;
				case 6:	sprintf (cpu->name, "%s", "K6 (0.30 um)");				break;
				case 7:	sprintf (cpu->name, "%s", "K6 (0.25 um)");				break;
				case 8:	nameptr += sprintf (cpu->name, "%s", "K6-2");
						if (cpu->stepping >= 8)
							printf (nameptr, "%s", " (CXT core)");
						break;
				case 9:	sprintf (cpu->name, "%s", "K6-III");			break;
				case 12:sprintf (cpu->name, "%s", "K6-2+ (0.18um)");	break;
				case 13:sprintf (cpu->name, "%s", "K6-3+ (0.18um)");	break;
				default:sprintf (cpu->name, "%s", "Unknown CPU");		break;
			}
			break;

		case 6:
			switch (cpu->model) {
				case 0:	sprintf (cpu->name, "%s", "K7 ES");		break;
				case 1:	nameptr += sprintf (cpu->name, "%s", "Athlon (0.25um)");
						switch (cpu->stepping) {
							case 1:	sprintf (nameptr, "%s", " Rev C1");	break;
							case 2:	sprintf (nameptr, "%s", " Rev C2");	break;
						}
					break;
				case 2:	nameptr += sprintf (cpu->name, "%s", "Athlon (0.18um)");
						switch (cpu->stepping) {
							case 1:	sprintf (nameptr, "%s", " Rev A1");	break;
							case 2:	sprintf (nameptr, "%s", " Rev A2");	break;
						}
						break;
				case 3:	nameptr += sprintf (cpu->name, "%s", "Duron");
						switch (cpu->stepping) {
							case 0:	sprintf (nameptr, "%s", " Rev A0");	break;
							case 1:	sprintf (nameptr, "%s", " Rev A2");	break;
						}
						break;
				case 4:	nameptr += sprintf (cpu->name, "%s", "Thunderbird");
						switch (cpu->stepping) {
							case 0:	sprintf (nameptr, "%s", " Rev A1");		break;
							case 1:	sprintf (nameptr, "%s", " Rev A2");		break;
							case 2:	sprintf (nameptr, "%s", " Rev A4-A8");	break;
							case 3:	sprintf (nameptr, "%s", " Rev A9");		break;
						}
						break;
				case 6:	nameptr += sprintf (cpu->name, "%s", "Mobile ");
						switch (cpu->stepping) {
							case 0:	sprintf (nameptr, "%s", "Athlon Rev A0-A1");	break;
							case 1:	sprintf (nameptr, "%s", "Athlon Rev A2");		break;
							case 2:	sprintf (nameptr, "%s", "Duron");				break;
						}
						break;
				default:sprintf (cpu->name, "%s", "Unknown CPU");
				break;
			}
		}
	}
}


void display_AMD_info(unsigned int maxei, struct cpudata *cpu)
{
	unsigned int i;
	unsigned long tmp;
	unsigned long eax, ebx, ecx, edx;

	if (show_registers && (maxei != 0))
		dump_extended_AMD_regs(cpu->number, maxei);

	if (show_msr)
		dump_athlon_MSR(cpu->number);

	if (show_bluesmoke)
		decode_bluesmoke(cpu->number);

	if (maxei >= 0x80000001) {
		cpuid (cpu->number, 0x00000001, &eax, &ebx, &ecx, &tmp);
		cpuid (cpu->number, 0x80000001, &eax, &ebx, &ecx, &edx);
		edx |= tmp;
		decode_feature_flags (cpu, edx);
	}

	if (maxei >= 0x80000002) {
		/* Processor identification string */
		char namestring[49], *cp;
		unsigned int j;
		cp = namestring;
		for (j = 0x80000002; j <= 0x80000004; j++) {
			cpuid (cpu->number, j, &eax, &ebx, &ecx, &edx);

			for (i = 0; i < 4; i++)
				*cp++ = eax >> (8 * i);
			for (i = 0; i < 4; i++)
				*cp++ = ebx >> (8 * i);
			for (i = 0; i < 4; i++)
				*cp++ = ecx >> (8 * i);
			for (i = 0; i < 4; i++)
				*cp++ = edx >> (8 * i);
		}
		printf ("Processor name string: %s\n\n", namestring);
	}

	if (show_cacheinfo)
		decode_AMD_cacheinfo(cpu->number, maxei);

	if (maxei >= 0x80000007) {
		cpuid (cpu->number, 0x80000007, &eax, &ebx, &ecx, &edx);
		printf ("PowerNOW! Technology information\n");
		printf ("Available features:");
		if (edx & 1<<1)
			printf ("\n\tBus divisor control");
		if (edx & 1<<2)
			printf ("\n\tVoltage ID control\n");
		if (!(edx & (1<<1 | 1<<2)))
			printf (" None");
		printf ("\n\n");
	}
}
