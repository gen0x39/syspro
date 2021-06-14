/*
  http-server-.c -- 常に同じ内容を返す HTTP サーバ(forkなし版)
  ~yas/syspro/ipc/http-server.c
*/
#include <stdio.h>
#include <stdlib.h>	/* exit() */
#include <sys/types.h>	/* socket(), wait4() */
#include <sys/socket.h>	/* socket() */
#include <netinet/in.h>	/* struct sockaddr_in */
#include <sys/resource.h> /* wait4() */
#include <sys/wait.h>	/* wait4() */
#include <netdb.h>	/* getnameinfo() */
#include <string.h>	/* strlen() */
#include <unistd.h>	/* getpid(), gethostname() */

extern  void http_server( int portno, int ip_version );
extern	void http_receive_request_and_send_reply( int com );
extern	int  http_receive_request( FILE *in );
extern	void http_send_reply( FILE *out );
extern	void http_send_reply_bad_request( FILE *out );
extern	void print_my_host_port_http( int portno );
extern  char *chomp( char *str );
extern	void tcp_sockaddr_print( int com );
extern	void tcp_peeraddr_print( int com );
extern	void sockaddr_print( struct sockaddr *addrp, socklen_t addr_len );
extern  int  tcp_acc_port( int portno, int ip_version );
extern	int  fdopen_sock( int sock, FILE **inp, FILE **outp );

int
main( int argc, char *argv[] )
{
	int portno, ip_version;

        if( !(argc == 2 || argc==3) ) {
		fprintf(stderr,"Usage: %s portno {ipversion}\n",argv[0] );
		exit( 1 );
	}
	portno = strtol( argv[1],0,10 );
	if( argc == 3 )
		ip_version = strtol( argv[2],0,10 );
	else
		ip_version = 46; /* Both IPv4 and IPv6 by default */
	http_server( portno,ip_version );
}

void
http_server( int portno, int ip_version )
{
	int acc,com ;

	acc = tcp_acc_port( portno, ip_version );
	if( acc<0 )
		exit( -1 );
	print_my_host_port_http( portno );
	tcp_sockaddr_print( acc );
	while( 1 )
	{
		printf("[%d] accepting incoming connections (fd==%d) ...\n",getpid(),acc );
		if( (com = accept( acc,0,0 )) < 0 )
		{
			perror("accept");
			exit( -1 );
		}
		tcp_peeraddr_print( com );
		http_receive_request_and_send_reply( com );
	}
}

#define	BUFFERSIZE	1024

void
http_receive_request_and_send_reply( int com )
{
	FILE *in, *out ;

	if( fdopen_sock(com,&in,&out) < 0 )
	{
		perror("fdooen()");
		exit( -1 );
	}
	if( http_receive_request( in ) )
	{
		http_send_reply( out );
	}
	else
	{
		http_send_reply_bad_request( out );
	}
	printf("[%d] Replied\n",getpid() );
	fclose( in );
	fclose( out );
}

int
http_receive_request( FILE *in )
{
	char requestline[BUFFERSIZE] ;
	char rheader[BUFFERSIZE] ;
	
	if( fgets(requestline,BUFFERSIZE,in) <= 0 )
	{
		printf("No request line.\n");
		return( 0 );
	}
	chomp( requestline ); /* remove \r\n */
	printf("requestline is [%s]\n",requestline );
	while( fgets(rheader,BUFFERSIZE,in) )
	{
		chomp( rheader ); /* remove \r\n */
		if( strcmp(rheader,"") == 0 )
			break;
		printf("Ignored: %s\n",rheader );
	}
	if( strchr(requestline,'<') ||
	    strstr(requestline,"..") )
	{
		printf("Dangerous request line found.\n");
		return( 0 );
	}
	return( 1 );
}

void
http_send_reply( FILE *out )
{
	fprintf(out,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
    	fprintf(out,"<html><head></head><body>hello.</body></html>\n");
}

void
http_send_reply_bad_request( FILE *out )
{
	fprintf(out,"HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\n\r\n");
    	fprintf(out,"<html><head></head><body>400 Bad Request</body></html>\n");
}

#define HOST_NAME_MAX 256
void
print_my_host_port_http( int portno )
{
	char hostname[HOST_NAME_MAX+1] ;

	gethostname( hostname,HOST_NAME_MAX );
	hostname[HOST_NAME_MAX] = 0 ;
	printf("open http://%s:%d/index.html\n", hostname, portno );
}

char *
chomp( char *str )
{
	int len ;

	len = strlen( str );
	if( len>=2 && str[len-2] == '\r' && str[len-1] == '\n' )
	{
		str[len-2] = str[len-1] = 0;
	}
	else if( len >= 1 && (str[len-1] == '\r' || str[len-1] == '\n') )
	{
		str[len-1] = 0;
	}
	return( str );
}

void
tcp_sockaddr_print( int com )
{
	struct sockaddr_storage addr ;
	socklen_t addr_len ; /* MacOSX: __uint32_t */

	addr_len = sizeof( addr );
    	if( getsockname( com, (struct sockaddr *)&addr, &addr_len  )<0 )
	{
		perror("tcp_peeraddr_print");
		return;
	}
    	printf("[%d] accepting (fd==%d) to ",getpid(),com );
	sockaddr_print( (struct sockaddr *)&addr, addr_len );
	printf("\n");
}

void
tcp_peeraddr_print( int com )
{
	struct sockaddr_storage addr ;
	socklen_t addr_len ; /* MacOSX: __uint32_t */

	addr_len = sizeof( addr );
    	if( getpeername( com, (struct sockaddr *)&addr, &addr_len  )<0 )
	{
		perror("tcp_peeraddr_print");
		return;
	}
    	printf("[%d] connection (fd==%d) from ",getpid(),com );
	sockaddr_print( (struct sockaddr *)&addr, addr_len );
	printf("\n");
}

void
sockaddr_print( struct sockaddr *addrp, socklen_t addr_len )
{
	char host[BUFFERSIZE] ;
	char port[BUFFERSIZE] ;

	if( getnameinfo(addrp, addr_len, host, sizeof(host),
			port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV)<0 )
		return;
	if( addrp->sa_family == PF_INET )
		printf("%s:%s", host, port );
	else
		printf("[%s]:%s", host, port );
}

#define PORTNO_BUFSIZE 30


int
tcp_acc_port( int portno, int ip_version )
{
	struct addrinfo hints, *ai;
	char portno_str[PORTNO_BUFSIZE];
	int err, s, on, pf;

    	switch( ip_version )
	{
	case 4:
		pf = PF_INET;
		break;
	case 6:
		pf = PF_INET6;
		break;
	case 0:
	case 64:
	case 46:
		pf = 0;
		break;
	default:
		fprintf(stderr,"bad IP version: %d.  4 or 6 is allowed.\n",
			ip_version );
		goto error0;
	}
	snprintf( portno_str,sizeof(portno_str),"%d",portno );
	memset( &hints, 0, sizeof(hints) );
	ai = NULL;
	hints.ai_family   = pf ;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM ;
	if( (err = getaddrinfo( NULL, portno_str, &hints, &ai )) )
	{
		fprintf(stderr,"bad portno %d? (%s)\n",portno,
			gai_strerror(err) );
		goto error0;
	}
	if( (s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0 )
	{
		perror("socket");
		goto error1;
	}

#ifdef	IPV6_V6ONLY
	if( ai->ai_family == PF_INET6 && ip_version == 6 )
	{
		on = 1;
		if( setsockopt(s,IPPROTO_IPV6, IPV6_V6ONLY,&on,sizeof(on)) < 0 )
		{
			perror("setsockopt(,,IPV6_V6ONLY)");
			goto error1;
		}
	}
#endif	/*IPV6_V6ONLY*/

	if( bind(s,ai->ai_addr,ai->ai_addrlen) < 0 )
	{
		perror("bind");
		fprintf(stderr,"Port number %d\n", portno );
		goto error2;
	}
	on = 1;
	if( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0 )
	{
		perror("setsockopt(,,SO_REUSEADDR)");
		goto error2;
	}
	if( listen( s, 5 ) < 0 )
	{
		perror("listen");
		goto error2;
	}
	freeaddrinfo( ai );
	return( s );

error2:
	close( s );	
error1:
	freeaddrinfo( ai );
error0:
	return( -1 );
}

int
fdopen_sock( int sock, FILE **inp, FILE **outp )
{
	int sock2 ;

	if( (sock2=dup(sock)) < 0 )
	{
		return( -1 );
	}
	if( (*inp = fdopen( sock2, "r" )) == NULL )
	{
		close( sock2 );
		return( -1 );
	}
	if( (*outp = fdopen( sock, "w" )) == NULL )
	{
		fclose( *inp );
		*inp = 0 ;
		return( -1 );
	}
	setvbuf(*outp, (char *)NULL, _IONBF, 0);
	return( 0 );
}