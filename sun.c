#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sunriset.h"

#define PRINTF(fmt, args...) if (verbose) printf(fmt, ##args)

static int  verbose = 1;
extern char *__progname;

static void convert(double ut, int *h, int *m)
{
	*h = (int)floor(ut);
	*m = (int)(60 * (ut - floor(ut)));
}

static char *ut2str(double ut)
{
	int h, m;
	static char buf[10];

	convert(ut, &h, &m);
	snprintf(buf, sizeof(buf), "%02d:%02d", h, m);

	return buf;
}

static void print(const char *fmt, double up, double dn)
{
	int uh, um, dh, dm;

	convert(up, &uh, &um);
	convert(dn, &dh, &dm);

	PRINTF(fmt, uh, um, dh, dm);
}

static int riset(int mode, double lat, double lon, int year, int month, int day)
{
	double rise, set;

	sun_rise_set(year, month, day, lon, lat, &rise, &set);
	if (mode)
		PRINTF("Sun rises %s", ut2str(rise));
	if (!mode)
		PRINTF("Sun sets %s", ut2str(set));
	if (mode == -1)
		PRINTF(", sets %s", ut2str(set));
	PRINTF(" UTC\n");
//	print("Sun rises %02d:%02d, sets %02d:%02d UTC\n", rise, set);

	return 0;
}

static int sunrise(double lat, double lon, int year, int month, int day)
{
	return riset(1, lat, lon, year, month, day);
}

static int sunset(double lat, double lon, int year, int month, int day)
{
	return riset(0, lat, lon, year, month, day);
}

static int all(double lat, double lon, int year, int month, int day)
{
	double daylen, civlen, nautlen, astrlen;
	double rise, set, civ_start, civ_end, naut_start, naut_end;
	double astr_start, astr_end;
	int rs, civ, naut, astr;

	daylen = day_length(year, month, day, lon, lat);
	civlen = day_civil_twilight_length(year, month, day, lon, lat);
	nautlen = day_nautical_twilight_length(year, month, day, lon, lat);
	astrlen = day_astronomical_twilight_length(year, month, day, lon, lat);

	PRINTF("Day length:                 %5.2f hours\n", daylen);
	PRINTF("With civil twilight         %5.2f hours\n", civlen);
	PRINTF("With nautical twilight      %5.2f hours\n", nautlen);
	PRINTF("With astronomical twilight  %5.2f hours\n", astrlen);
	PRINTF("Length of twilight: civil   %5.2f hours\n", (civlen - daylen) / 2.0);
	PRINTF("                  nautical  %5.2f hours\n", (nautlen - daylen) / 2.0);
	PRINTF("              astronomical  %5.2f hours\n", (astrlen - daylen) / 2.0);

	rs = sun_rise_set(year, month, day, lon, lat, &rise, &set);
	civ = civil_twilight(year, month, day, lon, lat, &civ_start, &civ_end);
	naut = nautical_twilight(year, month, day, lon, lat, &naut_start, &naut_end);
	astr = astronomical_twilight(year, month, day, lon, lat, &astr_start, &astr_end);

	PRINTF("Sun at south %s UTC\n", ut2str((rise + set) / 2.0));

	switch (rs) {
	case 0:
		print("Sun rises %02d:%02d, sets %02d:%02d UTC\n", rise, set);
		break;

	case +1:
		PRINTF("Sun above horizon\n");
		break;

	case -1:
		PRINTF("Sun below horizon\n");
		break;
	}

	switch (civ) {
	case 0:
		print("Civil twilight starts %02d:%02d, ends %02d:%02d UTC\n",
		      civ_start, civ_end);
		break;

	case +1:
		PRINTF("Never darker than civil twilight\n");
		break;

	case -1:
		PRINTF("Never as bright as civil twilight\n");
		break;
	}

	switch (naut) {
	case 0:
		print("Nautical twilight starts %02d:%02d, ends %02d:%02d UTC\n",
		      naut_start, naut_end);
		break;

	case +1:
		PRINTF("Never darker than nautical twilight\n");
		break;

	case -1:
		PRINTF("Never as bright as nautical twilight\n");
		break;
	}

	switch (astr) {
	case 0:
		print("Astronomical twilight starts %02d:%02d, ends %02d:%02d UTC\n",
		      astr_start, astr_end);
		break;

	case +1:
		PRINTF("Never darker than astronomical twilight\n");
		break;

	case -1:
		PRINTF("Never as bright as astronomical twilight\n");
		break;
	}

	return 0;
}

static int interactive(double *lat, double *lon, int *year, int *month, int *day)
{
	char buf[80];

	printf("Latitude (+ is north) and longitude (+ is east) : ");
	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%lf %lf", lat, lon);

	printf("Input date ( yyyy mm dd ) (ctrl-C exits): ");
	fgets(buf, 80, stdin);
	sscanf(buf, "%d %d %d", year, month, day);

	return 1;
}

static int usage(int code)
{
	printf("Usage: %s [-hip] [+/-latitude] [+/-longitude]\n"
	       "\n"
	       "Options:\n"
	       "  -a  Show all relevant times\n"
	       "  -h  This help text\n"
	       "  -i  Interactive mode\n"
	       "  -r  Sunrise mode\n"
	       "  -s  Sunset mode\n"
	       "\n", __progname);

	return code;
}

int main(int argc, char *argv[])
{
	int c, op, ok = 0;
	int year, month, day;
	double lon, lat;

	while ((c = getopt(argc, argv, "ahirsv")) != EOF) {
		switch (c) {
		case 'h':
			return usage(0);

		case 'i':
			verbose++;
			ok = interactive(&lat, &lon, &year, &month, &day);
			break;

		case 'a':
			verbose++;
			/* fallthrough */
		case 'r':
		case 's':
			op = c;
			break;

		case 'v':
			verbose++;
			break;

		case ':':	/* missing param for option */
		case '?':	/* unknown option */
		default:
			return usage(1);
		}
	}

	if (optind + 1 < argc) {
		time_t now;
		struct tm *tm;

		lat = atof(argv[optind++]);
		lon = atof(argv[optind]);

		now = time(NULL);
		tm = localtime(&now);
		year = 1900 + tm->tm_year;
		month = 1 + tm->tm_mon;
		day = tm->tm_mday;

//		PRINTF("latitude %f longitude %f date %d-%02d-%02d %d:%d (%s)\n",
//		       lat, lon, year, month, day, tm->tm_hour, tm->tm_min, tm->tm_zone);
		ok = 1;
	}

	if (!ok)
		return usage(1);

	switch (op) {
	case 'a':
		return all(lat, lon, year, month, day);

	case 'r':
		return sunrise(lat, lon, year, month, day);

	case 's':
		return sunset(lat, lon, year, month, day);

	default:
		verbose++;
		break;
	}

	return riset(-1, lat, lon, year, month, day);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */