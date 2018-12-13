
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <limits.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
std::string nickname;



void error(const char *s)
{
    perror(s);
    exit(-1);
}


int Socket(int domain, int type, int protocol)
{
    int rc;

    rc = socket(domain, type, protocol);
    if(rc == -1) error("socket()");

    return rc;
}

void Connect(int socket, const struct sockaddr *addr, socklen_t addrlen)
{
    int rc;

    // rc = connect(socket, addr, addrlen);
    if(connect(socket, addr, addrlen) == -1) error("connect()");
}

void Close(int fd)
{
    int rc;

    for(;;) {
        rc = close(fd);
        if(!rc) break;
        if(errno == EINTR) continue;
        error("close()");
    }
}

void Inet_aton(const char *str, struct in_addr *addr)
{
    int rc;

    rc = inet_aton(str, addr);
    if(!rc) {
        /* Функция inet_aton() не меняет errno в случае ошибки. Чтобы
        сообщение, выводимое error(), было более осмысленным,
        присваиваем errno значение EINVAL. */

        errno = EINVAL;
        error("inet_aton()");
    }
}

int Select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout)
{
    int rc;

    for(;;) {
        rc = select(n, readfds, writefds, exceptfds, timeout);
        if(rc != -1) break;
        if(rc == EINTR) continue;
        error("select()");
    }

    return rc;
}

size_t Read(int fd, void *buf, size_t count)
{
    ssize_t rc;

    for(;;) {
        rc = read(fd, buf, count);
        if(rc != -1) break;
        if(errno == EINTR) continue;
        error("read()");
    }

    return rc;
}

size_t Write(int fd, const void *buf, size_t count)
{
    ssize_t rc;

    for(;;) {
        rc = write(fd, buf, count);
        if(rc != -1) break;
        if(errno == EINTR) continue;
        error("write()");
    }

    return rc;
}


size_t writen(int socket, const char *buf, size_t count)
{
    const char *p;
    size_t n, rc;

    if(buf == NULL) {
        errno = EFAULT;
        error("writen()");
    }


    p = buf;
    n = count;
    while(n) {
        rc = Write(socket, p, n);
        n -= rc;
        p += rc;
    }

    return count;
}

void show_usage()
{
    puts("Usage: client ip_address");
    exit(-1);
}

void do_work(int socket)
{
    int n;
    fd_set readfds;
    char s[256];
    std::string message;
    ssize_t rc;

    n = MAX(STDIN_FILENO, socket) + 1;

    for(;;) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(socket, &readfds);

        Select(n, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(&s[0],0, sizeof(s));
            rc = Read(STDIN_FILENO, s, 256);
            message.clear();
            message = nickname + ":" + s;
            //std::cout << "s:["<< s<<","<< rc <<"]m: ["<< message << "] len: "<< message.length() << " size " << message.size()<<std::endl;
            if(!rc) break;
            std::copy(message.begin(), message.end(), s);
            s[message.size()]= '\0';
            writen(socket, s, message.size());
        }
        if(FD_ISSET(socket, &readfds)) {
            rc = Read(socket, s, 256);
            if(!rc) break;
            Write(STDOUT_FILENO, s, rc);
        }
    }
}
struct sockaddr_in servaddr;
int main(int argc, char **argv)
{
    puts("Hello! You Start Chat!");
    int socket;
    std::cout << "please enter your nickname: "<<std::endl;
    std::cin >> nickname;
    std::cout << "you auth on nickname >> " <<nickname<<std::endl;
    //if(argc != 2) show_usage();

    socket = Socket(PF_INET, SOCK_STREAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1027);
    //Inet_aton(argv[1], &servaddr.sin_addr);
    Inet_aton("127.0.0.1", &servaddr.sin_addr);

    Connect(socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
    do_work(socket);
    Close(socket);

    return 0;
}
