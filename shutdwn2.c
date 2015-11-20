/* Shutdown for OS/2

  version    : 1.5
  compiler   : Borland C++ v1.0 for OS/2

  created on : 29 March 1993
               Santiago Crespo, 2:341/24@fidonet.org
  modified   : 29 December 1993
               Steven van.Loef, 2:512/45.16@fidonet.org
               added option to shutdown after specified seconds
               added option to shutdown on specified date and/or time
*/

#define USAGE "Shutdown v1.5 for OS/2 2.x\n\n" \
"Usage      : SHUTDOWN.EXE [[/axxxx] | [[/dyyyymmdd] [/thhmm]]]\n" \
"             /a - shutdown after xxxx seconds\n" \
"             /d - shutdown on YYYYMMDD (Year, Month, Day)\n" \
"             /t - shutdown on HHMM (24 hour format)\n" \
"             /? - show this help\n" \
"             SHUTDOWN without options will shutdown the system immediately\n\n"

#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#define INCL_NOPMAPI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os2.h>

#define OPT_STRING      "a~d~t~?"
/* Meaning:
 *
 * -? takes no arguments.
 * -a, -d and -t take mandatory arguments (I set 'optneed' to '~', below).
 */

#define OPT_CHARS       "-/"
/* Meaning:
 *
 * Options can begin with '-', or '/'.
 */

/* Global variables for egetopt(): */
extern int optneed;     /* character used for mandatory arguments */
extern int optmaybe;    /* character used for optional arguments */
extern int optchar;     /* character which begins a given argument */
extern int optbad;      /* what egetopt() returns for a bad option */
extern int opterrfd;    /* where egetopt() error messages go */
extern char *optstart;  /* string which contains valid option start chars */
extern int optind;      /* index of current argv[] */
extern int optopt;      /* the actual option pointed to */
extern int opterr;      /* set to 0 to suppress egetopt's error messages */
extern char *optarg;    /* the argument of the option */

/* Functions */
extern int egetopt(int, char **, char *);
void sdWaitSec(ULONG);
void sdWaitDayTime(DATETIME *);
void Usage(void);

void main(int argc, char *argv[])
{
    DATETIME dtSdTime;
    ULONG    seconds = 0,
             sdate,
             stime;
    USHORT   rc;
    UCHAR    method = 0;
    int      ch;
    char     months[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    opterrfd = fileno(stderr);      /* errors to stderr */
    opterr = 1;             /* set this to 1 to get egetopt's error msgs */
    optbad = '!';           /* return '!' instead of '?' on error */
    optneed = '~';          /* mandatory arg identifier (in OPT_STRING) */
    optstart = OPT_CHARS;   /* characters that can start options */

    DosGetDateTime (&dtSdTime);
    dtSdTime.hours = dtSdTime.minutes = 0;

    /* process command line arguments */
    while ((ch = egetopt(argc, argv, OPT_STRING)) != EOF)
    {
        switch (ch)
        {
            case '?':
                Usage();
                break;

            case 'a':
                if (method > 1 || strlen(optarg) > 4)
                    Usage();
                seconds = atol(optarg);
                method = 1; /* in x seconds */
                break;

            case 'd':
                if (seconds != 0 || strlen(optarg) != 8)
                    Usage();
                sdate = atol(optarg);
                dtSdTime.year  = sdate / 10000;
                dtSdTime.month = (sdate - dtSdTime.year * 10000) / 100;
                dtSdTime.day   = (sdate - (dtSdTime.year * 10000) -
                                 (dtSdTime.month * 100));
                if (method == 3)
                    method = 4;   /* date & time */
                else
                    method = 2;   /* date only */
                break;

            case 't':
                if (seconds != 0 || strlen(optarg) != 4)
                    Usage();
                stime = atol(optarg);
                dtSdTime.hours   = stime / 100;
                dtSdTime.minutes = (stime - dtSdTime.hours * 100);
                if (method == 2)
                    method = 4;   /* date & time */
                else
                    method = 3;   /* time only */
                break;

            case '!':
                Usage();
                break;

            default:
                Usage();
                break;
        }
    }

    fprintf(stderr, "Shutdown v1.5 for OS/2 2.x\n\n");
    switch (method)
    {
        case 1:
            fprintf(stderr, "Shutting down the system in %4d seconds", seconds);
            break;

        case 2:
        case 3:
        case 4:
            fprintf(stderr, "The system will shutdown on %02d %s %4d at " \
                    "%02d:%02d\n",
                    dtSdTime.day, months[dtSdTime.month - 1], dtSdTime.year,
                    dtSdTime.hours, dtSdTime.minutes);
            break;

        default:
            fprintf(stderr, "The system will shutdown immediately...\n");
    }

    /* Wait until shutdown time */
    if (method != 0)
    {
        if (method == 1)
            sdWaitSec(seconds);
        else
            sdWaitDayTime(&dtSdTime);
    }

    /* Perform the shutdown */
    rc = DosShutdown(0L);

    /* Display result of shutdown */
    switch (rc)
    {
        case NO_ERROR:
            fprintf(stderr, "\nThe system was shutdown succesfully, you can " \
                            "now turn of the power");
            break;

        case ERROR_ALREADY_SHUTDOWN:
            fprintf(stderr, "\nThe system was already shutdown, therefor it " \
                            "is safe to turn of the power now");
            break;

        case ERROR_INVALID_PARAMETER:
            fprintf(stderr, "\nInvalid paramater to DosShutdown, please " \
                            "contact the author");
            break;

        default:
            fprintf(stderr, "\nUnknown return code [%lu], please contact " \
                            "the author", rc);
            break;
    }
}

void sdWaitSec(ULONG seconds)
{
    DATETIME dtTime;
    ULONG    sdtime,
             total,
             diff;

    diff = sdtime = 0;

    /* enter loop until shutdown time */
    while (diff < seconds)
    {
        DosGetDateTime (&dtTime);
        total = (dtTime.hours) * 3600 +
                (dtTime.minutes * 60) +
                (dtTime.seconds);

        if (sdtime == 0)
            sdtime = total;

        diff = (total - sdtime);
        fprintf (stderr, "%4d seconds", (seconds - diff));
        DosSleep(500);
    }
    fprintf(stderr, "\n");
}

void sdWaitDayTime(DATETIME *dtSdTime)
{
    DATETIME dtTime;

    while (1)
    {
        DosGetDateTime (&dtTime);
        if (dtTime.year == dtSdTime->year)
            if (dtTime.month == dtSdTime->month)
                if (dtTime.day == dtSdTime->day)
                    if (dtTime.hours == dtSdTime->hours)
                        if (dtTime.minutes == dtSdTime->minutes)
                            break;
        DosSleep(500);
    }
}

/* Display usage information if wrong argument supplied or /? issued */
void Usage(void)
{
    fprintf(stderr, USAGE);
    exit(0);
}

