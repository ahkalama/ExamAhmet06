#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

int client[4096];
char *messages[4096];
fd_set activefd, writefd, readfd;
int serverfd;

void ft_error(const char *str)
{
    write(2, str, strlen(str));
    if (serverfd > 0)
        close(serverfd);
    exit(1);
}

void ft_send(int fd, int maxfd, const char *str)
{
    for (int i = serverfd + 1; i <= maxfd; i++)
        if (client[i] != -1 && i != fd && FD_ISSET(i, &writefd))
            send(i, str, strlen(str), 0);
}

int extract_message(char **buf, char **msg)
{
    char *newline = strchr(*buf, '\n');
    if (!newline)
        return 0;
    *newline = '\0';
    *msg = *buf;
    *buf = strdup(newline + 1);
    return 1;
}

void append_message(char **buf, const char *add)
{
    char *newbuf = realloc(*buf, strlen(*buf) + strlen(add) + 1);
    if (newbuf)
    {
        strcat(newbuf, add);
        *buf = newbuf;
    }
}

int main(int ac, char **av)
{
    if (ac != 2)
        ft_error("Wrong number of arguments\n");
    
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1)
        ft_error("Fatal error\n");

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servaddr.sin_port = htons(atoi(av[1]));
    if (bind(serverfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0 ||
        listen(serverfd, 128) != 0)
        ft_error("Fatal error\n");

    FD_ZERO(&activefd);
    FD_SET(serverfd, &activefd);
    int maxfd = serverfd, id = 0;
    while (1)
    {
        readfd = writefd = activefd;
        if (select(maxfd + 1, &readfd, &writefd, NULL, NULL) < 0)
            continue;

        if (FD_ISSET(serverfd, &readfd))
        {
            int clientfd = accept(serverfd, NULL, NULL);
            if (clientfd < 0)
                continue;

            FD_SET(clientfd, &activefd);
            char buf[64];
            sprintf(buf, "server: client %d just arrived\n", id);
            client[clientfd] = id++;
            messages[clientfd] = strdup("");
            maxfd = (clientfd > maxfd) ? clientfd : maxfd;
            ft_send(clientfd, maxfd, buf);
        }
        for (int fd = serverfd + 1; fd <= maxfd; fd++)
        {
            if (FD_ISSET(fd, &readfd))
            {
                char buffer[4095];
                int bytes = recv(fd, buffer, 4094, 0);
                if (bytes <= 0)
                {
                    FD_CLR(fd, &activefd);
                    char buf[64];
                    sprintf(buf, "server: client %d just left\n", client[fd]);
                    ft_send(fd, maxfd, buf);
                    client[fd] = -1;
                    free(messages[fd]);
                    close(fd);
                }
                else
                {
                    buffer[bytes] = '\0';
                    append_message(&messages[fd], buffer);
                    char *msg;
                    while (extract_message(&messages[fd], &msg))
                    {
                        char buf[64 + strlen(msg)];
                        sprintf(buf, "client %d: %s\n", client[fd], msg);
                        ft_send(fd, maxfd, buf);
                        free(msg);
                    }
                }
            }
        }
    }
    return 0;    
}
