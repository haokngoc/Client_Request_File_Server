#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/md5.h>
#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void calculate_md5(char *filename, char *md5_str) {
    unsigned char c[MD5_DIGEST_LENGTH];
    // char *filename="file.txt";
    int i;
    FILE *inFile = fopen (filename, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", filename);
        //return 0;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
    {
        MD5_Update (&mdContext, data, bytes);
    }
        
    MD5_Final (c,&mdContext);

    //char md5_str[MD5_DIGEST_LENGTH*2 + 1]; // khai báo mảng chuỗi kích thước đủ lớn
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5_str[i*2], "%02x", c[i]); // chuyển đổi từng phần tử unsigned char sang chuỗi hexa và nối chúng lại
    }
    md5_str[MD5_DIGEST_LENGTH*2] = '\0'; // thêm kí tự null terminator

    //printf ("%s %s\n", md5_str, filename); // xuất chuỗi MD5 và tên file
    md5_str[i*2] = '\0';
    fclose (inFile);
}

int main(int argc, char *argv[]) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_address.sin_addr) != 1) {
        perror("Failed to convert server address");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    char message[BUFFER_SIZE];

    // gửi message: "Request_Dowload_File"
    strcpy(message, "Client request: Request_Dowload_File");
    message[strcspn(message, "\n")] = '\0';
    if (send(client_socket, message, strlen(message), 0) == -1) {
        perror("Failed to send message");
        exit(EXIT_FAILURE);
    }

    // clear message buffer
    memset(message, 0, BUFFER_SIZE);



    char response[BUFFER_SIZE];

    // receive server response to first request
    if (recv(client_socket, response, BUFFER_SIZE, 0) == -1) {
        perror("Failed to receive response");
        exit(EXIT_FAILURE);
    }
    printf("Server response: %s\n", response);

    // client gửi tên file cần tải đến client
    fgets(message, BUFFER_SIZE, stdin);
    message[strcspn(message, "\n")] = '\0';
    
    if (send(client_socket, message, strlen(message), 0) == -1) {
        perror("Failed to send message");
        exit(EXIT_FAILURE);
    }

    // clear response buffer
    memset(response, 0, BUFFER_SIZE);

    // Nhận thông tin file từ server
    recv(client_socket, response, BUFFER_SIZE, 0);

    char *file_name_ptr = strtok(response, ",");
    char *file_size_ptr = strtok(NULL, ",");
    char *md5_str_ptr = strtok(NULL, ",");
    int file_size = atoi(file_size_ptr);
    printf("Info file: \n");
    printf("  File name: %s\n", file_name_ptr);
    printf("  File size: %d\n", file_size);
    printf("  MD5sum: %s\n",md5_str_ptr);
    
    memset(response, 0, BUFFER_SIZE);

    // Gửi request yêu cầu gửi file đến server
    strcpy(message, "Please send File");
    message[strcspn(message, "\n")] = '\0';

    if (send(client_socket, message, strlen(message), 0) == -1) {
        perror("Failed to send message");
        exit(EXIT_FAILURE);
    }
    int file_fd = open("received_file.txt", O_CREAT | O_WRONLY, 0666);

    if (file_fd == -1) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    off_t offset = 0;
    int bytes_received;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        int bytes_written = write(file_fd, buffer, bytes_received);
        if (bytes_written == -1) {
            perror("Failed to write to file");
            exit(EXIT_FAILURE);
        }
        offset += bytes_written;
        printf("Received %d bytes\n", bytes_received);
    }
    memset(buffer, 0, BUFFER_SIZE);

    close(file_fd);
    // tính giá trị MD5sum và so sánh
    char *md5_File_recv;
    calculate_md5("received_file.txt", md5_File_recv);
    printf("%s\n",md5_File_recv);

    if(strcmp(md5_File_recv,md5_str_ptr) == 0) {
        printf("File received successfully\n");
        send(client_socket, "Dowload Done", strlen("Dowload Done"), 0);
    }
    else {
        printf("File received Falsed\n");
        send(client_socket, "Need to resend", strlen("Need to resend"), 0);
    }
    close(client_socket);
    return 0;
}
