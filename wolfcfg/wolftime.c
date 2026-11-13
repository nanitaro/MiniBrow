/*
 * wolftime.c - time functions for wolfSSL in the MiniBrow research build (TIME_OVERRIDES in user_settings.h)
 *
 * Open Watcom's time() and gmtime() call tzset() every time, and tzset() looks for the TZ environment
 * variable with a case-insensitive multi-byte compare (one CharUpper call per character). wolfSSL asks
 * for the time twice per certificate (notBefore / notAfter), so loading cacert.pem spent more than half
 * of its time there. These two functions do not touch the C library's time zone code.
 */
#include <windows.h>
#include <time.h>

/* seconds since 1970-01-01 00:00:00 UTC */
time_t XTIME(time_t *timer)
{
    FILETIME         ft;
    unsigned __int64 q;
    time_t           t;

    GetSystemTimeAsFileTime(&ft);       /* 100 ns units since 1601-01-01 UTC */
    q = ((unsigned __int64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    t = (time_t)((q - 116444736000000000ui64) / 10000000ui64);
    if (timer != NULL) {
        *timer = t;
    }
    return t;
}

/* UTC calendar time (no time zone, no daylight saving time) */
struct tm *XGMTIME(const time_t *timer, struct tm *tmp)
{
    static const int mdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    unsigned long    t, days, rem;
    int              year = 1970, mon, leap;

    if (timer == NULL || tmp == NULL) {
        return NULL;
    }
    t    = (unsigned long)*timer;
    days = t / 86400UL;
    rem  = t % 86400UL;
    tmp->tm_hour = (int)(rem / 3600UL);
    tmp->tm_min  = (int)(rem % 3600UL / 60UL);
    tmp->tm_sec  = (int)(rem % 60UL);
    tmp->tm_wday = (int)((days + 4UL) % 7UL);   /* 1970-01-01 was a Thursday */
    for (;;) {
        unsigned long yd;
        leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        yd   = leap ? 366UL : 365UL;
        if (days < yd) {
            break;
        }
        days -= yd;
        year++;
    }
    tmp->tm_year = year - 1900;
    tmp->tm_yday = (int)days;
    for (mon = 0; mon < 11; mon++) {
        unsigned long md = (unsigned long)mdays[mon] + (mon == 1 && leap ? 1UL : 0UL);
        if (days < md) {
            break;
        }
        days -= md;
    }
    tmp->tm_mon   = mon;
    tmp->tm_mday  = (int)days + 1;
    tmp->tm_isdst = 0;
    return tmp;
}
