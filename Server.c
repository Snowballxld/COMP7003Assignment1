#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#define BUF_SIZE 1024
static char *socket_path;

// ---------------- Utility Functions ----------------

// Simple Caesar cipher encryption
void caesar_cipher(char *text, int shift)
{
    shift = shift % 26;

    for (int i = 0; text[i]; i++)
    {
        char c = text[i];
        if (c >= 'A' && c <= 'Z')
        {
            text[i] = (c - 'A' + shift + 26) % 26 + 'A';
        }
        else if (c >= 'a' && c <= 'z')
        {
            text[i] = (c - 'a' + shift + 26) % 26 + 'a';
        }
    }
}

// Cleanup on Ctrl+C
void cleanup(int sig)
{
    if (socket_path)
        unlink(socket_path);
    exit(0);
}

// Parses arguments passed into file
void parse_arguments(int argc, char *argv[], char **socket_path)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <message> <shift> <socket_path>\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "h") == 0 || strcmp(argv[1], "-h") == 0)
    {
        fprintf(stderr, "Usage: %s <message> <shift> <socket_path>\n", argv[0]);
        exit(0);
    }

    *socket_path = argv[1];
}

// ---------------- Server Setup ----------------

int create_server_socket(const char *path)
{
    int server_fd;
    struct sockaddr_un addr;

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    unlink(path);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 5) == -1)
    {
        perror("listen");
        exit(1);
    }

    return server_fd;
}

// ---------------- Client Handling ----------------

void handle_client(int client_fd)
{
    char buffer[BUF_SIZE];
    int shift;

    // Read shift
    if (read(client_fd, &shift, sizeof(int)) <= 0)
    {
        perror("read shift");
        close(client_fd);
        return;
    }

    // Read message
    int n = read(client_fd, buffer, BUF_SIZE - 1);
    if (n > 0)
    {
        buffer[n] = '\0';
        caesar_cipher(buffer, shift);
        write(client_fd, buffer, strlen(buffer));
    }

    close(client_fd);
}

// ---------------- Main ----------------

int main(int argc, char *argv[])
{

    char *socket_path;

    parse_arguments(argc, argv, &socket_path);
    signal(SIGINT, cleanup);

    int server_fd = create_server_socket(socket_path);
    printf("Server listening on %s\n", socket_path);

    while (1)
    {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1)
        {
            perror("accept");
            continue;
        }
        handle_client(client_fd);
    }

    cleanup(0);
    return 0;
}
