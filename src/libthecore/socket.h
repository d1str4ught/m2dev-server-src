#pragma once

#ifdef OS_WINDOWS
typedef int socklen_t;
#else
#define INVALID_SOCKET -1
#endif

int			socket_read(socket_t desc, char* read_point, size_t space_left);
int			socket_write(socket_t desc, const char *data, size_t length);

int			socket_udp_read(socket_t desc, char * read_point, size_t space_left, struct sockaddr * from, socklen_t * fromlen);
int			socket_tcp_bind(const char * ip, int port);
int			socket_udp_bind(const char * ip, int port);

socket_t	socket_accept(socket_t s, struct sockaddr_in *peer);
void		socket_close(socket_t s);
socket_t	socket_connect(const char* host, WORD port);

void		socket_nonblock(socket_t s);
void		socket_block(socket_t s);
void		socket_dontroute(socket_t s);
void		socket_lingeroff(socket_t s);
void		socket_lingeron(socket_t s);

void		socket_sndbuf(socket_t s, unsigned int opt);
void		socket_rcvbuf(socket_t s, unsigned int opt);

