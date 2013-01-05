
#include "qutility.h"

#ifdef T_WINNT
#include <errno.h>
/*
 * Number of micro-seconds between the beginning of the Windows epoch
 * (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970).
 *
 * This assumes all Win32 compilers have 64-bit support.
 */
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS) || defined(__WATCOMC__)
  #define DELTA_EPOCH_IN_USEC  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_USEC  11644473600000000ULL
#endif

static usecs_t filetime_to_unix_epoch (const FILETIME *ft)
{
    usecs_t res = (usecs_t) ft->dwHighDateTime << 32;

    res |= ft->dwLowDateTime;
    res /= 10;                   /* from 100 nano-sec periods to usec */
    res -= DELTA_EPOCH_IN_USEC;  /* from Win epoch to Unix epoch */
    return (res);
}

int gettimeofday (struct timeval *tv, void *tz )
{
    FILETIME  ft;
    usecs_t tim;

    if (!tv) {
        errno = EINVAL;
        return (-1);
    }
    GetSystemTimeAsFileTime (&ft);
    tim = filetime_to_unix_epoch (&ft);
    tv->tv_sec  = (long) (tim / 1000000L);
    tv->tv_usec = (long) (tim % 1000000L);
    return (0);
}
#endif //T_WINNT

int fwritex( const void *ptr, int count, FILE * stream )
{
  char *ptr1 = (char*)ptr;
  int bytes, total_bytes = 0;
  printx("\nfwritex(), count: %d, remainder: %d\n", count, count % 1024);
  do {
    if( count >= 1024 )
    {
      bytes = fwrite( ptr1, 1, 1024, stream );
      //printx("fwritex: %d\n", bytes);
      ptr1 += 1024;
      count -= 1024;
      total_bytes += bytes;
    }
    else
    {
      bytes = fwrite( ptr1, 1, count, stream );
      printx("\nfwritex: %d\n", bytes);
      count = 0;
      total_bytes += bytes;
    }
  } while (count);
  return total_bytes;
}

