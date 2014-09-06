#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

#include <batterystat.c>

char *tzeastern = "America/New_York";
char *tzargentina = "America/Buenos_Aires";
char *tzutc = "UTC";
char *tzberlin = "Europe/Berlin";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	memset(buf, 0, sizeof(buf));
	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		perror("localtime");
		exit(1);
	}

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		exit(1);
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0) {
		perror("getloadavg");
		exit(1);
	}

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char *
getnet(void)
{
  FILE *fp;
  char netid[40];
  int len;

  fp = popen("iwgetid | awk {'print $2'} | sed 's/ESSID:\"//g' | sed 's/\"//g'","r");
  fgets(netid, sizeof(netid),fp);
  pclose(fp);
  if(netid[1] == NULL){
    return "Not Connected";
  }else{
    len = strlen(netid);
    if( netid[len-1] == '\n' )
          netid[len-1] = 0;
    return strcat(netid, " Connected");
  }
}

int
main(void)
{
	char *status;
	char *avgs;
	char *tmar;
	char *tmutc;
	char *tmbln;
  char *tmeast;
  char *batt;
  char *net;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(60)) {
		avgs = loadavg();
		tmar = mktimes("%I:%M", tzargentina);
		tmutc = mktimes("%I:%M", tzutc);
		tmeast = mktimes("%I:%M", tzeastern);
		tmbln = mktimes("KW %W %a %d %b %R:%M %Z %Y", tzberlin);
    batt = getbattery();
    net = getnet();

		status = smprintf("::[ %s ]::[ BATT: %s ]::[ %s ]::", tmeast, batt,net);
		setstatus(status);
		free(avgs);
		free(tmar);
		free(tmutc);
		free(tmbln);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

