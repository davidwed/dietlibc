#define _BSD_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <strings.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>

int hardcodedhostname;
char myhostname[100];
int namelen;
struct sockaddr* peer;
socklen_t sl;
/* zeroconf/bonjour */
int s6,s4;
/* llmnr */
int ls6,ls4;
/* nbt */
int nbt;
char ifname[10];

struct sockaddr_in mysa4;
struct sockaddr_in hissa4;
struct sockaddr_in6 mysa6;
struct msghdr mh;

#define PKGSIZE 1500

static int scan_fromhex(unsigned char c) {
  c-='0';
  if (c<=9) return c;
  c&=~0x20;
  c-='A'-'0';
  if (c<6) return c+10;
  return -1;
/* more readable but creates worse code:
  if (c>='0' && c<='9')
    return c-'0';
  else if (c>='A' && c<='F')
    return c-'A'+10;
  else if (c>='a' && c<='f')
    return c-'a'+10;
  return -1;
*/
}

static void getip(int interface) {
  int fd;
  struct cmsghdr* x;
  memset(&mysa4,0,sizeof(mysa4));
  memset(&hissa4,0,sizeof(hissa4));
  hissa4.sin_family=AF_INET;
  hissa4.sin_port=htons(137);
  memset(&mysa6,0,sizeof(mysa6));
  for (x=CMSG_FIRSTHDR(&mh); x; x=CMSG_NXTHDR(&mh,x))
    if (x->cmsg_level==SOL_IP && x->cmsg_type==IP_PKTINFO) {
      mysa4.sin_addr=((struct in_pktinfo*)(CMSG_DATA(x)))->ipi_spec_dst;
      hissa4.sin_addr=((struct in_pktinfo*)(CMSG_DATA(x)))->ipi_addr;
#ifdef DEBUG
      printf("found IP %x\n",mysa4.sin_addr.s_addr);
      printf("found IP %x\n",((struct in_pktinfo*)(CMSG_DATA(x)))->ipi_addr.s_addr);
#endif
    }

  fd=open("/proc/net/if_inet6",O_RDONLY);
  if (fd!=-1) {
    char buf[1024];	/* increase as necessary */
    int i,j,len;
    len=read(fd,buf,sizeof buf);
    if (len>0) {
      int ok;
      char* c=buf;
      char* max=buf+len;
      ok=0;
      /* "fec000000000000102c09ffffe53fc52 01 40 40 00     eth0" */
      while (c<max) {
	int a,b;
	for (i=0; i<16; ++i) {
	  a=scan_fromhex(c[i*2]);
	  b=scan_fromhex(c[i*2+1]);
	  if (a<0 || b<0) goto kaputt;
	  mysa6.sin6_addr.s6_addr[i]=(a<<4)+b;
	}
	ok=1;
	a=scan_fromhex(c[33]);
	b=scan_fromhex(c[34]);
	c+=32;
	if (a<0 || b<0) goto kaputt;
	if ((a<<4)+b == interface) {
	  ok=1;
	  goto kaputt;
	}
	while (c<max && *c!='\n') ++c;
	++c;
      }
kaputt:
      if (!ok) memset(&mysa6,0,sizeof(mysa6));
    }
    close(fd);
  }
}

#ifdef DEBUG
char* method[]={"zeroconf mDNS","LLMNR","NetBIOS"};
#endif

static void handle(int s,char* buf,int len,int interface,int llmnr) {
  int q;
  char* obuf=buf;
  char* after;
  int olen=len;
  unsigned int type;
  int slen;
#if 0
  if (s==nbt) {
    size_t i;
    for (i=0; i<len; ++i) {
      printf("%02x%s",(unsigned char)(buf[i]),(i&15)==15?"\n":" ");
    }
    printf("\n\n");
//    return;
  }
#endif
#if 0
  if (interface==0)
    printf("called with interface==0!\n");
#endif
  if (len<8*2) return;			/* too short */
  buf[len]=0;
  if (llmnr==2) {	/* netbios */
    if (buf[2]!=1) return;
  } else
    if ((buf[2]&(llmnr?0xfd:0xf8)) != 0) return;		/* not query */
  q=(((unsigned int)(buf[4])) << 8) | buf[5];
  if (q!=1) return;			/* cannot handle more than 1 query */
  if (buf[6] || buf[7]) return;		/* answer count must be 0 */
  if (buf[8] || buf[9]) return;		/* name server count must be 0 */
  if (buf[10] || buf[11]) return;	/* additional record count must be 0 */
  buf+=12; len-=12;
#ifdef DEBUG
  printf("got %s request for \"%.*s\"!\n",method[llmnr],(int)(unsigned char)buf[0],buf+1);
#endif
  if (!llmnr && buf[2]==(char)0x80) buf[2]=0;
    /* we ignore the "answer multicast plz" bit and always do unicast */
  if (llmnr==2) {	/* NetBIOS; decode name */
    unsigned char* x;
    size_t i;
    if (buf[0]&1) return;	/* needs to be multiple of two */
    x=alloca(namelen/2+2);
    x[0]=buf[0]/2;
    for (i=0; i<x[0]; ++i)
      x[i+1]=((buf[i*2+1]-'A')<<4) +
	      (buf[i*2+2]-'A');
    x[x[0]]=0;
    {
      unsigned char* y;
      for (y=x+1; *y!=' ' && *y!=0; ++y) ;
      *y=0;
      x[0]=y-x-1;
    }
#ifdef DEBUG
    printf("  aka \"%s\"\n",x+1);
#endif
#if 1
    if (x[0]!=namelen || strncasecmp((const char*)x+1,myhostname,namelen))
      return;
#endif
    buf+=buf[0]+2;
  } else if (buf[0]==namelen && !strncasecmp(buf+1,myhostname,namelen)) {
    buf+=namelen+1;
    if (!*buf)
      ++buf;
    else if (!strcmp(buf,"\x05local"))
      buf+=7;
    else
      return;
  }

//    if (((unsigned long)buf)&1) ++buf;
  if (buf[0] || buf[2]) return;	/* all supported types and classes fit in 8 bit */
  if (buf[3]!=1) return;		/* we only support IN queries */
  type=(unsigned char)buf[1];
  slen=buf-obuf+4;
  obuf[2]|=0x80; 	/* it's answer; we don't support recursion */
  if (s==nbt) {
    if (type!=32)
      return;
  } else if (type!=1 && type!=28 && type!=255)
      return;

  getip(interface);

  if (type==1 || type==32 || type==255) {		/* A or ANY, we can do that */
    struct ifreq ifr;
    static int v4sock=-1;
    unsigned int ofs=12;
    ++obuf[7];			/* one answer */
    if (type==32) {
      /* netbios demands special attention */
      memcpy(obuf+slen,"\xc0\x0c" /* goofy compression */
		    "\x00\x20" /* NB */
		    "\x00\x01" /* IN */
		    "\x00\x00\x02\x30" /* ttl; 0x230, about 9.3 minutes */
		    "\x00\x06" /* 6 bytes payload */
		    "\x60\x00" /* H-node, unique */
		    ,14);
      ofs=14;
    } else
      memcpy(obuf+slen,"\xc0\x0c" /* goofy compression */
		    "\x00\x01" /* A */
		    "\x00\x01" /* IN */
		    "\x00\x00\x02\x30" /* ttl; 0x230, about 9.3 minutes */
		    "\x00\x04" /* 4 bytes payload */
		    ,12);
     /* Now we need to find out our IP address.  This is easier said
      * than done.  There are basically two methods.  The easy way is
      * to ask the kernel to give us the destination address the packet
      * went to.  If the kernel comes through, it's in mysa4.  This
      * only works if the packet came in via ipv4.  If it is set, use
      * it. */
    if (mysa4.sin_addr.s_addr)
      memcpy(obuf+slen+ofs,&mysa4.sin_addr,4);
    else {
      int tempsock=socket(AF_INET,SOCK_DGRAM,0);
      if (tempsock==-1) return;
      if (connect(tempsock,(struct sockaddr*)&hissa4,sizeof(hissa4))==0) {
	socklen_t l=sizeof(mysa4);
	/* if the connect worked, we can call getsockname to figure out
	 * our IP for talking to this guy */
	if (getsockname(tempsock,(struct sockaddr*)&mysa4,&l)==0) {
	  memcpy(obuf+slen+ofs,&mysa4.sin_addr,4);
	  close(tempsock);
	  goto wehavetheip;
	}
      }
      close(tempsock);
       /* if the address was 0, then the packet probably came in via
	* IPv6.  In that case we have no choice but to use the network
	* interface number and ask the kernel for the IP address
	* configured at that interface.
	* That is done via
	*   ioctl(somesock,SIOCGIFADDR,struct ifreq)
	* Unfortunately, we need to put the interface _name_ in that
	* struct, not the index.  So we must first call
	*   ioctl(somesock,SIOCGIFINDEX,struct ifreq)
	* Note that this method might return the wrong IP address if
	* the interface has more than one configured.  */
      if (v4sock==-1) v4sock=s4;
      if (v4sock==-1) v4sock=socket(AF_INET,SOCK_DGRAM,0);
      if (v4sock==-1) return;
      ifr.ifr_ifindex=interface;
      if (ioctl(v4sock,SIOCGIFNAME,&ifr)==-1) return;
      ifr.ifr_addr.sa_family=AF_INET;
      if (ioctl(v4sock,SIOCGIFADDR,&ifr)==-1) return;	/* can't happen */
      memcpy(obuf+slen+ofs,&((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr,4);
    }
wehavetheip:
    slen+=4+ofs;
  }
  if (type==28 || type==255) {	/* AAAA or ANY */
    if (!IN6_IS_ADDR_UNSPECIFIED(mysa6.sin6_addr.s6_addr32)) {
      memcpy(obuf+slen,"\xc0\x0c" /* goofy DNS compression */
		      "\x00\x1c" /* AAAA */
		      "\x00\x01" /* IN */
		      "\x00\x00\x02\x30" /* ttl */
		      "\x00\x10" /* 16 bytes payload */
		      ,12);
      memcpy(obuf+slen+12,&mysa6.sin6_addr,16);
      slen+=28;
      ++obuf[7];
    }
  }
  if (obuf[7])
    sendto(s,obuf,slen,0,peer,sl);

}

struct sockaddr_in sa4;
struct sockaddr_in6 sa6;
struct pollfd pfd[5];

struct iovec iv;
char abuf[100];
char buf[PKGSIZE+1];

static int v4if() {
  struct cmsghdr* x;
  for (x=CMSG_FIRSTHDR(&mh); x; x=CMSG_NXTHDR(&mh,x))
    if (x->cmsg_level==SOL_IP && x->cmsg_type==IP_PKTINFO)
      return ((struct in_pktinfo*)(CMSG_DATA(x)))->ipi_ifindex;
  return 0;
}

static void recv4(int s) {
  int len;

  mh.msg_name=&sa4;
  mh.msg_namelen=sizeof(sa4);
  if ((len=recvmsg(s,&mh,0))==-1) {
    perror("recvmsg");
    exit(3);
  }
  peer=(struct sockaddr*)&sa4;
  sl=sizeof(sa4);

#ifdef DEBUG
  printf("v4: ");
#endif

  handle(s,buf,len,v4if(),s==nbt?2:s==ls4);
}

static void recv6(int s) {
  int len,interface;
#ifdef DEBUG
  char addrbuf[INET6_ADDRSTRLEN];
  char ifbuf[IFNAMSIZ];
#endif

  mh.msg_name=&sa6;
  mh.msg_namelen=sizeof(sa6);

#if 0
  mh.msg_control=abuf;
  mh.msg_controllen=sizeof(abuf);
#endif

  if ((len=recvmsg(s,&mh,0))==-1) {
    perror("recvmsg");
    exit(3);
  }
  peer=(struct sockaddr*)&sa6;
  sl=sizeof(sa6);

  if (IN6_IS_ADDR_V4MAPPED(sa6.sin6_addr.s6_addr32)) {
    interface=v4if();
#ifdef DEBUG
    inet_ntop(AF_INET,(char*)(sa6.sin6_addr.s6_addr)+12,addrbuf,sizeof addrbuf);
    if_indextoname(interface,ifbuf);
    printf("v4: %s: ",ifbuf);
#endif
  } else {
    interface=sa6.sin6_scope_id;
#ifdef DEBUG
    inet_ntop(AF_INET6,sa6.sin6_addr.s6_addr,addrbuf,sizeof addrbuf);
    if_indextoname(interface,ifbuf);
    printf("v6: %s: ",ifbuf);
#endif
  }

  handle(s,buf,len,interface,s==nbt?2:s==ls6);
#ifdef DEBUG
  printf(" from %s\n",addrbuf);
#endif
}

static void init_sockets(int* sock6,int* sock4,int port,char* v6ip,char* v4ip) {
  int _s4,_s6;
  int one=1;
  *sock4=-1; if (sock6) *sock6=-1;
  _s6=sock6?socket(PF_INET6,SOCK_DGRAM,IPPROTO_UDP):-1;
  _s4=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
  if (_s4==-1 && _s6==-1) {
    perror("socket");
    return;
  }
  if (_s6!=-1) {
    setsockopt(_s6,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
    memset(&sa6,0,sizeof(sa6));
    sa6.sin6_family=PF_INET6;
    sa6.sin6_port=htons(port);
    if (bind(_s6,(struct sockaddr*)&sa6,sizeof(struct sockaddr_in6))==-1) {
      perror("bind IPv6");
      close(_s6);
      _s6=-1;
    }
  }
  if (_s4!=-1) {
    setsockopt(_s4,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
    memset(&sa4,0,sizeof(sa4));
    sa4.sin_family=PF_INET;
    sa4.sin_port=htons(port);
    if (bind(_s4,(struct sockaddr*)&sa4,sizeof(struct sockaddr_in))==-1) {
      if (errno!=EADDRINUSE || _s6==-1)
	perror("bind IPv4");
      close(_s4);
      _s4=-1;
    }
  }
  if (_s4==-1 && _s6==-1) return;

  {
    int val=255;
    if (_s6!=-1) {
      struct ipv6_mreq opt;
      if (port!=137) {
	setsockopt(_s6,IPPROTO_IPV6,IPV6_UNICAST_HOPS,&val,sizeof(val));
	setsockopt(_s6,IPPROTO_IPV6,IPV6_MULTICAST_LOOP,&one,sizeof(one));
	memcpy(&opt.ipv6mr_multiaddr,v6ip,16);
	opt.ipv6mr_interface=0;
	setsockopt(_s6,IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP,&opt,sizeof opt);
      }
      setsockopt(_s6,IPPROTO_IPV6,IPV6_PKTINFO,&one,sizeof one);
    }
    {
      struct ip_mreq opt;
      int s=(_s4==-1?_s6:_s4);
      if (port!=137) {
	setsockopt(s,SOL_IP,IP_TTL,&val,sizeof(val));
	memcpy(&opt.imr_multiaddr.s_addr,v4ip,4);
	opt.imr_interface.s_addr=0;
	setsockopt(s,IPPROTO_IP,IP_ADD_MEMBERSHIP,&opt,sizeof(opt));
      }
      setsockopt(s,SOL_IP,IP_PKTINFO,&one,sizeof one);
    }
  }

  *sock4=_s4;
  if (sock6) *sock6=_s6;
}

int main(int argc,char* argv[]) {
  int n=-1;
  mh.msg_name=&sa4;
  mh.msg_namelen=sizeof(sa4);
  mh.msg_iov=&iv;
  mh.msg_iovlen=1;
  iv.iov_base=buf;
  iv.iov_len=PKGSIZE;
  mh.msg_control=abuf;
  mh.msg_controllen=sizeof(abuf);

  if (argc>1) {
    if (strlen(argv[1])>64) {
      puts("dnsd: error: host name longer than 64 chars");
      return 1;
    }
    strcpy(myhostname,argv[1]);
  } else {
    if (gethostname(myhostname,64)==-1) {
      perror("gethostname");
      return 1;
    }
  }
  namelen=strlen(myhostname);

  init_sockets(&s6,&s4,5353,"\xff\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xfb","\xe0\x00\x00\xfb");
  init_sockets(&ls6,&ls4,5355,"\xff\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x03","\xe0\x00\x00\xfc");
  init_sockets(NULL,&nbt,137,NULL,"\x00\x00\x00\x00");

  pfd[0].events=pfd[1].events=pfd[2].events=pfd[3].events=pfd[4].events=POLLIN;
  if (s6!=-1) pfd[++n].fd=s6;
  if (s4!=-1) pfd[++n].fd=s4;
  if (ls6!=-1) pfd[++n].fd=ls6;
  if (ls4!=-1) pfd[++n].fd=ls4;
  if (nbt!=-1) pfd[++n].fd=nbt;
  if (!++n)
    return 2;
  for (;;) {
    int i;
    switch (poll(pfd,n,5*1000)) {
    case -1:
      if (errno==EINTR) continue;
      perror("poll");
      return 1;
    case 0:
      continue;
    }
    for (i=0; i<n; ++i)
      if (pfd[i].revents & POLLIN) {
	if (pfd[i].fd==s6 || pfd[i].fd==ls6)
	  recv6(pfd[i].fd);
	else
	  recv4(pfd[i].fd);
      }
  }
  return 0;
}
