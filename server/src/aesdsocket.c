#include "stdlib.h"
#include "syslog.h"
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <unistd.h>
// #include <arpa/inet.h>
// #include <netinet/in.h>

#define BACKLOG (50)
volatile bool terminate = false;

void sighandler(int signal) {
  switch (signal) {
  case SIGINT:
  case SIGTERM:
    terminate = true;
    break;
  default:
    // nothing
    break;
  }
}

void setup_signal_handler(void) {
  struct sigaction act = {
      .sa_handler = sighandler,
  };
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char *argv[]) {
  openlog(NULL, 0, LOG_USER);
  struct addrinfo hints = {};
  struct addrinfo *res = NULL;
  int status = -1, sockfd = -1, new_fd = -1;
  FILE *fp = NULL;
  const int reuse = 1;
  bool daemon = false;
  int opt;

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill in my IP for me

  while ((opt = getopt(argc, argv, "d")) != -1) {
    switch (opt) {
      case 'd':
        daemon = true;
        break;
      default:
        syslog(LOG_PERROR, "Usage: %s [-d]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if ((status = getaddrinfo(NULL, "9000", &hints, &res)) != 0) {
    syslog(LOG_PERROR, "getaddrinfo: %s\n", gai_strerror(status));
    status = -1;
    goto done;
  }

  if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) <
      0) {
    syslog(LOG_PERROR, "socket: %s\n", strerror(errno));
    status = -1;
    goto done;
  }

  if ((status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                           sizeof reuse)) != 0) {
    syslog(LOG_PERROR, "setsockopt: %s\n", strerror(errno));
    status = -1;
    goto done;
  }

  if ((status = bind(sockfd, res->ai_addr, res->ai_addrlen)) != 0) {
    syslog(LOG_PERROR, "bind: %s\n", strerror(errno));
    status = -1;
    goto done;
  }

  if (daemon) {
    switch (fork()) {
      case -1:
        syslog(LOG_PERROR, "fork: %s\n", strerror(errno));
        goto done;
      case 0:
        setsid();
        chdir("/");
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        break;
      default:
        exit(EXIT_SUCCESS);
    }
  }

  if ((status = listen(sockfd, BACKLOG)) != 0) {
    syslog(LOG_PERROR, "listen: %s\n", strerror(errno));
    status = -1;
    goto done;
  }

  if ((fp = fopen("/var/tmp/aesdsocketdata", "w+")) == NULL) {
    syslog(LOG_PERROR, "fopen: %s\n", strerror(errno));
    status = -1;
    goto done;
  }

  while (!terminate) {
    struct sockaddr_storage their_addr = {};
    socklen_t addr_size = sizeof their_addr;
    char buf[1024] = {0};
    ssize_t recv_bytes = 0;

    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) <
        0) {
      syslog(LOG_PERROR, "accept: %s\n", strerror(errno));
      status = -1;
      goto done;
    }

    syslog(LOG_INFO, "Accepted connection from %d\n",
           ntohl(((struct sockaddr_in *)&their_addr)->sin_addr.s_addr));

    while ((recv_bytes = recv(new_fd, buf, sizeof buf - 1, MSG_PEEK)) != -1) {
      buf[recv_bytes + 1] = '\0';
      char *newline_ptr = NULL;

      if ((newline_ptr = strchr(buf, '\n')) != NULL) {
        recv_bytes = newline_ptr - buf + 1;
      }

      assert((recv_bytes) == recv(new_fd, buf, recv_bytes, 0));

      if ((status = fseek(fp, 0L, SEEK_END)) != 0) {
        syslog(LOG_PERROR, "fseek: %s\n", strerror(errno));
        status = -1;
        goto done;
      }

      assert(recv_bytes == fwrite(buf, sizeof(buf[0]), recv_bytes, fp));

      if (newline_ptr != NULL) {
        off_t offset = 0;
        size_t count = 0;
        if ((status = fseek(fp, 0L, SEEK_END)) != 0) {
          syslog(LOG_PERROR, "fseek: %s\n", strerror(errno));
          status = -1;
          goto done;
        }
        count = ftell(fp);

        char *ptr = mmap(NULL, count, PROT_READ, MAP_PRIVATE, fileno(fp), 0);
        assert(ptr != NULL);

        assert(count == send(new_fd, ptr, count, 0));

        munmap(ptr, count);

        break;
      }
    }
    close(new_fd);
    new_fd = -1;
    syslog(LOG_INFO, "Closed connection from %d\n",
           ntohl(((struct sockaddr_in *)&their_addr)->sin_addr.s_addr));
  }
  syslog(LOG_INFO, "Caught signal, exiting");

done:
  if (fp != NULL) {
    fflush(fp);
    fclose(fp);
    (void)unlink("/var/tmp/aesdsocketdata");
  }
  if (new_fd != -1) {
    close(new_fd);
  }
  if (sockfd != -1) {
    close(sockfd);
  }
  if (res != NULL) {
    freeaddrinfo(res);
  }
  closelog();
  return status;
}
