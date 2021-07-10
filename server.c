#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define PORT 8000
#define RD_SIZE 10000

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;  
    int opt = 1;
    int addrlen = sizeof(address);
    char req[4096];
    char buffer[1024] = {0};
    char *hello = "Hello from server";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  // creates socket, SOCK_STREAM is for TCP. SOCK_DGRAM for UDP
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // This is to lose the pesky "Address already in use" error message
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt))) // SOL_SOCKET is the socket layer itself
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;  // Address family. For IPv6, it's AF_INET6. 29 others exist like AF_UNIX etc. 
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any IP address - listens from all interfaces.
    address.sin_port = htons( PORT );    // Server port to open. Htons converts to Big Endian - Left to Right. RTL is Little Endian

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Port bind is done. You want to wait for incoming connections and handle them in some way.
    // The process is two step: first you listen(), then you accept()
    while(1)
    {
        if (listen(server_fd, 3) < 0) // 3 is the maximum size of queue - connections you haven't accepted
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        // returns a brand new socket file descriptor to use for this single accepted connection. Once done, use send and recv
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        valread = read(new_socket , req, 4096);  // read infromation received into the buffer
        printf("Request received: %s\n",req);

        int file_count=0, file_name_i=0;
        char *file_names[1000];
        for(int i=0;i<strlen(req);i++)
        {
            if(req[i]==' ')
            {
                file_names[file_count]=malloc(128*sizeof(char));
                memset(file_names[file_count],0,128);
                file_count++;
                file_name_i=0;
            }
            else if(file_count>0)
            {
                file_names[file_count-1][file_name_i]=req[i];
                file_name_i++;
            }
        }

        memset(req,0,4096);
        // printf("Request has been analysed\n");
        // printf("filenames: %s %s\n", file_names[0], file_names[1]);

        for(int i=0;i<file_count;i++)
        {
            char to_send[200000]={0}, send_size[16]={0};
            int fd = open(file_names[i], O_RDONLY);
            int confread;
            char rec_confirm[4096]={0};
            // printf("here\n");
            if (fd < 0)
            {
                strcat(to_send,"File does not exist or is not readable: ");
                strcat(to_send,file_names[i]);
                strcat(to_send,"\n");

                send(new_socket, to_send, strlen(to_send), 0);  // use sendto() and recvfrom() for DGRAM
                confread = read(new_socket, rec_confirm, 4096);

                send(new_socket, file_names[i], strlen(file_names[i]), 0); // use sendto() and recvfrom() for DGRAM
                confread = read(new_socket, rec_confirm, 4096);

                send(new_socket, "0", strlen("0"), 0);      // file size
                confread = read(new_socket, rec_confirm, 4096);

                printf("Requested file %s doesn't exist or is not readable\n",file_names[i]);
            }
            else
            {
                struct stat st;
                stat(file_names[i],&st);


                if(S_ISDIR(st.st_mode))
                {
                    strcat(to_send, "Requested file is a directory: ");
                    strcat(to_send, file_names[i]);
                    strcat(to_send, "\n");

                    send(new_socket, to_send, strlen(to_send), 0); // use sendto() and recvfrom() for DGRAM
                    confread = read(new_socket, rec_confirm, 4096);

                    send(new_socket, file_names[i], strlen(file_names[i]), 0); // use sendto() and recvfrom() for DGRAM
                    confread = read(new_socket, rec_confirm, 4096);

                    send(new_socket, "0", strlen("0"), 0); // file size
                    confread = read(new_socket, rec_confirm, 4096);
                    
                    printf("Requested file %s is a directory\n", file_names[i]);
                }

                else
                {
                    strcat(to_send, "Receiving file: ");
                    strcat(to_send,file_names[i]);
                    strcat(to_send,"\n");
                    // itoa(st.st_size,send_size,10);
                    sprintf(send_size,"%ld",st.st_size);

                    send(new_socket, to_send, strlen(to_send), 0);
                    confread = read(new_socket, rec_confirm, 4096);

                    send(new_socket, file_names[i], strlen(file_names[i]), 0); // use sendto() and recvfrom() for DGRAM
                    confread = read(new_socket, rec_confirm, 4096);

                    send(new_socket, send_size, strlen(send_size), 0);
                    confread = read(new_socket, rec_confirm, 4096);

                    if(rec_confirm[0]=='C')
                    {
                        printf("File transfer of file %s was cancelled by client\n", file_names[i]);
                    }
                    else if(rec_confirm[0]=='R')
                    {
                        printf("Sending file %s\n", file_names[i]);
                        int rem_size=st.st_size, count=0;
                        // printf("\nsend_size: %d\n", rem_size);
                        while(rem_size>0)
                        {
                            // printf("a");
                            memset(to_send,0,200000);
                            // printf("b");
                            int read_size = read(fd, to_send, RD_SIZE);
                            send(new_socket, to_send, read_size, 0);
                            confread = read(new_socket, rec_confirm, 4096);
                            rem_size-=read_size;
                            count++;
                            // printf("%d: %d\n", count, rem_size);
                            fflush(NULL);
                        }
                        send(new_socket, "complete\n", strlen("complete\n"), 0);
                        int confread = read(new_socket, rec_confirm, 4096); // read information received into the buffer
                        printf("File %s received by client\n", file_names[i]);
                    }
                }
            }
            memset(to_send, 0, 200000);
        }
    }

    return 0;
}
