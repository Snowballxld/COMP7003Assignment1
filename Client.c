#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUF_SIZE 1024

// ---------------- Utility Functions ----------------

// Caesar cipher decryption (reverse shift)
void caesar_cipher(char *text, int shift)
{
    for (int i = 0; text[i]; i++)
    {
        shift = shift % 26;

        char c = text[i];
        if (c >= 'A' && c <= 'Z')
        {
            text[i] = (c - 'A' - shift + 26) % 26 + 'A';
        }
        else if (c >= 'a' && c <= 'z')
        {
            text[i] = (c - 'a' - shift + 26) % 26 + 'a';
        }
    }
}

// Parses arguments passed into file
void parse_arguments(int argc, char *argv[], char **message, int *shift, char **socket_path)
{
    if (argc != 4)
    {

        if (argc == 2 && (strcmp(argv[1], "h") == 0 || strcmp(argv[1], "-h") == 0))
        {
            fprintf(stderr, "Usage: %s <message> <shift> <socket_path>\n", argv[0]);
            exit(0);
        }
        else
        {
            fprintf(stderr, "Usage: %s <message> <shift> <socket_path>\n", argv[0]);
            exit(1);
        }
    }

    *message = argv[1];
    *shift = atoi(argv[2]);
    *socket_path = argv[3];
}

// ---------------- Socket Setup ----------------

int create_client_socket(const char *socket_path)
{
    int sock;
    struct sockaddr_un addr;

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect");
        close(sock);
        exit(1);
    }

    return sock;
}

// ---------------- Messaging ----------------

void send_message(int sock, int shift, const char *message)
{
    // Send shift first
    if (write(sock, &shift, sizeof(int)) == -1)
    {
        perror("write shift");
        exit(1);
    }

    // Then send message
    if (write(sock, message, strlen(message)) == -1)
    {
        perror("write message");
        exit(1);
    }
}

void receive_and_decrypt(int sock, int shift)
{
    char buffer[BUF_SIZE];
    int n = read(sock, buffer, BUF_SIZE - 1);

    if (n > 0)
    {
        buffer[n] = '\0';
        printf("Encrypted: %s\n", buffer);

        caesar_cipher(buffer, shift);
        printf("Decrypted: %s\n", buffer);
    }
    else
    {
        perror("read");
    }
}

// ---------------- Main ----------------

int main(int argc, char *argv[])
{
    char *message;
    int shift;
    char *socket_path;

    parse_arguments(argc, argv, &message, &shift, &socket_path);

    int sock = create_client_socket(socket_path);
    send_message(sock, shift, message);
    receive_and_decrypt(sock, shift);

    close(sock);
    return 0;
}
