#include "dietfeatures.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <time.h>

#include <stdio.h>

/* This code appears to be subtly wrong depending on the date.
 * However, the documentation I found about the tzfile layout are not
 * sufficient to debug this. */

char* tzname[2]={"GMT","GMT"};

#ifdef WANT_TZFILE_PARSER
static unsigned char *tzfile;
static int tzlen=-1;

void __maplocaltime(void);
void __maplocaltime(void) {
  int fd;
  unsigned int len;
  if (tzlen>=0) return;
  tzlen=0;
  if ((fd=open("/etc/localtime",O_RDONLY|O_CLOEXEC))<0) return;
  len=lseek(fd,0,SEEK_END);
  if ((tzfile=mmap(0,len,PROT_READ,MAP_PRIVATE,fd,0))==MAP_FAILED) {
    close(fd);
    return;
  }
  close(fd);
  if (len<44 || ntohl(*(int*)tzfile) != 0x545a6966) {
    munmap(tzfile,len);
    tzfile=0;
    return;
  }
  tzlen=len;
}

static int32_t __myntohl(const unsigned char* c) {
  return (((uint32_t)c[0])<<24) +
         (((uint32_t)c[1])<<16) +
         (((uint32_t)c[2])<<8) +
         ((uint32_t)c[3]);
}

#include <stdio.h>

time_t __tzfile_map(time_t t, int *isdst, int forward);
time_t __tzfile_map(time_t t, int *isdst, int forward) {
  /* "TZif" plus 16 reserved bytes. */
  unsigned char *tmp;
  int i;
  int tzh_timecnt, tzh_typecnt;
  *isdst=0;
  if (!tzfile) return t;
  tzh_timecnt=ntohl(*(int*)(tzfile+32));
  tzh_typecnt=ntohl(*(int*)(tzfile+36));

//  printf("tzh_timecnt=%d, tzh_typecnt=%d\n",tzh_timecnt,tzh_typecnt);

  tmp=tzfile+20+6*4;
  daylight=(tzh_timecnt>0);
  if (forward) {
    for (i=0; i<tzh_timecnt; ++i) {
//      printf("val=%x t=%x\n",(time_t)__myntohl(tmp+i*4),t);
      if ((time_t)__myntohl(tmp+i*4) >= t)
last:
      {
	unsigned char* tz=tmp;
//        printf("match at %d\n",i);
	tmp+=tzh_timecnt*4;
	i=tmp[i-1];
  /*      printf("using index %d\n",i); */
	tmp+=tzh_timecnt;
	tz+=tzh_timecnt*5+tzh_typecnt*6;
	tmp+=i*6;
  /*      printf("(%lu,%d,%d)\n",ntohl(*(int*)tmp),tmp[4],tmp[5]); */
	*isdst=tmp[4];
	tzname[0]=(char*)(tz+tmp[5]);
	timezone=__myntohl(tmp);
	return t+timezone;
      }
    }
    --i;
    goto last;
  } else {	/* reverse map, for mktime */
    time_t nexttz=0,lastval=0;
//    printf("tzh_timecnt: %d\n",tzh_timecnt);
    for (i=1; i<tzh_timecnt-1; ++i) {
      unsigned char* x, j;
      long k=0;
//      printf("ab %ld: ",__myntohl(tmp+i*4));
      x=tmp+tzh_timecnt*4;
      j=x[i-1];
      nexttz=__myntohl(x+tzh_timecnt+j*6);
//      printf("%ld - %ld (want %ld)\n",lastval,__myntohl(tmp+i*4)-nexttz,t);
      if (lastval <= t && (k=(__myntohl(tmp+i*4)-nexttz)) > t) {
//	printf("FOUND!1!!  Offset %d\n",nexttz);
	return t-nexttz;
      }
      lastval=k;
    }
  }
  return t;
}

void tzset(void) {
  int isdst;
  __maplocaltime();
  __tzfile_map(time(0),&isdst,1);
}

#else
void tzset(void) { }
#endif
