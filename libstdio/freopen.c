#include <sys/types.h>
#include <dietstdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <endian.h>

/* this is needed so the libpthread wrapper can initialize the mutex,
 * not to lock it */

FILE *freopen_unlocked(const char *path, const char *mode, FILE *stream) {
  if (stream) {
    int f=__stdio_parse_mode(mode)
#ifdef O_LARGEFILE
                                   | O_LARGEFILE
#endif
                                                ;
    fflush_unlocked(stream);
    __libc_close(stream->fd);
    if ((stream->fd=__libc_open(path,f,0666))!=-1) {
      struct stat st;
      fstat(stream->fd,&st);
      stream->flags=(S_ISFIFO(st.st_mode))?FDPIPE:0;
      switch (f&3) {
      case O_RDWR: stream->flags|=CANWRITE;	/* fall through */
      case O_RDONLY: stream->flags|=CANREAD; break;
      case O_WRONLY: stream->flags|=CANWRITE;
      }
    }
  } else errno=EINVAL;
  return stream;
}

FILE *freopen(const char *path, const char *mode, FILE *stream) __attribute__((weak,alias("freopen_unlocked")));
#if __WORDSIZE == 32
FILE *freopen64(const char *path, const char *mode, FILE *stream) __attribute__((weak,alias("freopen_unlocked")));
#endif