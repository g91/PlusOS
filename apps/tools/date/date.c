/* date - the current date and time (also installed as "time"). */
#include <plusos/plusos.h>
#include <plusrom/OSUtil.h>

int main(void)
{
    ULONGINT secs;
    ReadDateTime(&secs);
    DateTimeRec d;
    SecondsToDate(secs, &d);
    static const char *dn[] = { "", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char *mn[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    printf("%s %s %2d %04d  %02d:%02d:%02d\n",
           (d.dayOfWeek >= 1 && d.dayOfWeek <= 7) ? dn[d.dayOfWeek] : "",
           (d.month >= 1 && d.month <= 12) ? mn[d.month] : "", d.day, d.year,
           d.hour, d.minute, d.second);
    return 0;
}
