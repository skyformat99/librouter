/*
 * device.c
 *
 *  Created on: Jun 23, 2010
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "options.h"
#include "defines.h"
#include "device.h"
#include "args.h"
#include "str.h"

/* type, cish_string, linux_string */
dev_family _devices[] = {
	{ eth, "ethernet", "ethernet" },
	{ lo, "loopback", "loopback" },
	{ tun, "tun", "tun" },
	{ ipsec, "ipsec", "ipsec" },
	{ ppp, "m3G", "ppp" },
	{ none, NULL, NULL }
};

/**
 * ex.: name = 'ethernet'
 * retorna (device_family *){ethernet, "eth", "ethernet"},
 *
 * @param name
 * @return
 */
dev_family *libconfig_device_get_family(const char *name)
{
	int crsr;
	for (crsr = 0; _devices[crsr].cish_string != NULL; crsr++) {
		if (strncasecmp(_devices[crsr].cish_string, name, strlen(
		                _devices[crsr].cish_string)) == 0) {
			return &_devices[crsr];
		}
	}
	return NULL;
}

/**
 * ex.: device = 'serial', major = 0, minor = 16
 * retorna 'serial0.16'
 *
 * @param device
 * @param major
 * @param minor
 * @return
 */
char *libconfig_device_convert(const char *device, int major, int minor)
{
	//função modificada, onde a variavel linux_string era anteriormente cish_string

	char *result;
	dev_family *fam = libconfig_device_get_family(device);

	if (fam) {
		switch (fam->type) {
		case eth:
		case lo:
		case tun:
		case ipsec:
		case ppp:
		default:
			if (minor >= 0) {
				result = (char *) malloc(strlen(
				                fam->linux_string) + 12);
				sprintf(result, "%s%i.%i", fam->linux_string,
				                major, minor);
				return result;
			} else {
				result = (char *) malloc(strlen(
				                fam->linux_string) + 6);
				sprintf(result, "%s%i", fam->linux_string,
				                major);
				return result;
			}
			break;
		}
	} else {
		fprintf(stderr, "%% Unknown device family: %s\n", device);
		return (strdup("null0"));
	}
}

/**
 * ex.: osdev = 'serial0.16'
 * retorna 'serial 0.16' se mode=0,
 * ou      'serial0.16'  se mode=1.
 *
 * @param osdev
 * @param mode
 * @return
 */
char *libconfig_device_convert_os(const char *osdev, int mode)
{
	static char dev[64];
	char odev[16];
	int i, crsr;
	const char *cishdev;

	crsr = 0;
	while ((crsr < 8) && (osdev[crsr] > 32) && (!isdigit(osdev[crsr])))
		++crsr;
	memcpy(odev, osdev, crsr);
	odev[crsr] = 0;
	while ((osdev[crsr]) && (!isdigit(osdev[crsr])))
		++crsr; /* skip space! */

	cishdev = NULL;
	for (i = 0; _devices[i].linux_string != NULL; i++) {
		if (strcmp(_devices[i].linux_string, odev) == 0) {
			cishdev = _devices[i].cish_string;
			break;
		}
	}

	if (!cishdev)
		return NULL;

#ifdef OPTION_IPSEC
	if (_devices[i].type == ipsec) {
		char filename[32];
		char iface[16];
		FILE *f;

		sprintf(filename, "/var/run/%s", osdev);
		if ((f = fopen(filename, "r")) != NULL) {
			fgets(iface, 16, f);
			fclose(f);
			libconfig_str_striplf(iface);
			crsr = 0;
			while ((crsr < 8) && (iface[crsr] > 32) && (!isdigit(
			                iface[crsr])))
				++crsr; /* !!! ethernet == 8 */
			memcpy(odev, iface, crsr);
			odev[crsr] = 0;
			sprintf(dev, "crypto-%s%s%s", odev, mode ? "" : " ",
			                iface + crsr);
		} else
			return NULL;
	} else
#endif
		sprintf(dev, "%s%s%s", cishdev, mode ? "" : " ", osdev + crsr);

	/* Make ethernet -> Ethernet */
	if (mode)
		dev[0] = toupper(dev[0]);

	return dev;
}

/**
 * Recebe uma linha de comando com interfaces no estilo cish (ex.: 'ethernet 0')
 * e devolve a linha de comando com as interfaces traduzidos para estilo linux
 * (ex.: 'eth0').
 *
 * @param cmdline
 * @return
 */
char *libconfig_device_to_linux_cmdline(char *cmdline)
{
	static char new_cmdline[2048];
	arglist *args;
	int i;
	dev_family *fam;

	new_cmdline[0] = 0;
	args = libconfig_make_args(cmdline);
	for (i = 0; i < args->argc; i++) {
		fam = libconfig_device_get_family(args->argv[i]);
		if (fam) {
			//			strcat(new_cmdline, fam->cish_string);
			strcat(new_cmdline, fam->linux_string);
			i++;
			if (i >= args->argc)
				break;
		}
		strcat(new_cmdline, args->argv[i]);
		strcat(new_cmdline, " ");
	}
	libconfig_destroy_args(args);
	return new_cmdline;
}

/**
 * Recebe uma linha de comando com interfaces no estilo linux (ex.: 'eth0')
 * e devolve a linha de comando com as interfaces traduzidos para estilo cish
 * (ex.: 'ethernet 0').
 *
 * @param cmdline
 * @return
 */
char *libconfig_device_from_linux_cmdline(char *cmdline)
{
	static char new_cmdline[2048];
	arglist *args;
	int i;
	char *dev;

	new_cmdline[0] = 0;
	if (libconfig_str_is_empty(cmdline))
		return new_cmdline;
	args = libconfig_make_args(cmdline);
	for (i = 0; i < args->argc; i++) {
		dev = libconfig_device_convert_os(args->argv[i], 0);
		if (dev)
			strcat(new_cmdline, dev);
		else
			strcat(new_cmdline, args->argv[i]);
		strcat(new_cmdline, " ");
	}
	libconfig_destroy_args(args);
	return new_cmdline;
}


/**
 * Recebe uma linha de comando com redes no estilo zebra (ex.: '10.0.0.0/8')
 * e devolve a linha de comando com as redes traduzidas para estilo linux
 * (ex.: '10.0.0.0 255.0.0.0').
 *
 * @param cmdline
 * @return
 */
char *libconfig_zebra_to_linux_cmdline(char *cmdline)
{
	static char new_cmdline[2048];
	arglist *args;
	int i;
	char addr_net[64];

	new_cmdline[0] = 0;
	if (libconfig_str_is_empty(cmdline))
		return new_cmdline;

	args = libconfig_make_args(cmdline);

	for (i = 0; i < args->argc; i++) {
		if (cidr_to_classic(args->argv[i], addr_net) == 0)
			strcat(new_cmdline, addr_net);
		else
			strcat(new_cmdline, args->argv[i]);
		strcat(new_cmdline, " ");
	}

	libconfig_destroy_args(args);
	return new_cmdline;
}

/**
 * Recebe uma linha de comando com redes no estilo linux
 * (ex.: '10.0.0.0 255.0.0.0') e devolve a linha de comando
 * com as redes traduzidas para estilo zebra (ex.: '10.0.0.0/8')
 *
 * @param cmdline
 * @return
 */
char *libconfig_zebra_from_linux_cmdline(char *cmdline)
{
	static char new_cmdline[2048];
	arglist *args;
	int i;
	char buf[64];

	new_cmdline[0] = 0;
	if (libconfig_str_is_empty(cmdline))
		return new_cmdline;

	args = libconfig_make_args(cmdline);

	for (i = 0; i < (args->argc - 1); i++) {
		if ((validateip(args->argv[i]) == 0) && (classic_to_cidr(
		                args->argv[i], args->argv[i + 1], buf) == 0)) {
			strcat(new_cmdline, buf);
			i++;
		} else {
			strcat(new_cmdline, args->argv[i]);
		}
		strcat(new_cmdline, " ");
	}
	if (i < args->argc)
		strcat(new_cmdline, args->argv[i]);

	libconfig_destroy_args(args);
	return new_cmdline;
}
