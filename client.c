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

#define RD_SIZE 10000
#define PORT 8000

int main(int argc, char const *argv[])
{
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char req[4096] = {0};
    char buffer[200000] = {0};
    if(argc>1)
    {
        memset(req,0,4096);
        // printf("req: %s\n", req);
        // for(int i=0;i<4096;i++)
        // {
        //     req[i]='\0';
        // }
        strcat(req,"get");
        for(int i=1;i<argc;i++)
        {
            strcat(req," ");
            strcat(req,argv[i]);
            // printf("argv %d: %s", i, argv[i]);
            // memset(argv[i],0,strlen(argv[i]));
        }
        strcat(req, "\0\0\0");
        // printf("req: %s\n", req);
        // write(1,req,strlen(req));
    }
    else
    {
        printf("\nERROR: No arguments, please mention which files you require as command-line arguments\n\n");
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr)); // to make sure the struct is empty. Essentially sets sin_zero as 0
                                                // which is meant to be, and rest is defined below

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    

    // Converts an IP address in numbers-and-dots notation into either a 
    // struct in_addr or a struct in6_addr depending on whether you specify AF_INET or AF_INET6.
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  // connect to the server address
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    // printf("sent req: %s\n", req);
    send(sock , req , strlen(req) , 0 );  // send the message.
    printf("Request sent\n");
    memset(req,0,4096);


    for(int i=0;i<argc-1;i++)
    {
        char server_message[4096]={0}, size_of_file[16]={0}, file_name[128]={0};

        int messread = read(sock, server_message, 4096);
        send(sock,"Received\n",sizeof("Received\n"),0);

        int nameread = read(sock, file_name, 128);
        send(sock, "Received\n", sizeof("Received\n"), 0);

        int sizeread = read(sock, size_of_file, 16);

        int filesize=0, backup_flsz;
        filesize=atoi(size_of_file);
        // printf("\nfilesize: %d\n", filesize);
        backup_flsz=filesize;
        printf("%s",server_message);
        // printf("%d and %d\n", filesize, backup_flsz);
        if(filesize>0)
        {
            int fd = open(file_name,O_CREAT | O_WRONLY | O_TRUNC, 0666);
            // printf("fd: %d\n", fd);
            if(fd<0)
            {
                printf("Requested file %s cannot be created or already exists and cannot be written to\n", file_name);
                send(sock, "Cancel\n", sizeof("Cancel\n"), 0);
            }
            else
            {
                // printf("aaaaaaa\n");
                send(sock, "Received\n", sizeof("Received\n"), 0);
                // printf("bbbbbbbbbb\n");
                int count=0;
                while(filesize>0)
                {
                    char contents[200000]={0};
                    // printf("cccccccccccc\n");
                    sizeread=read(sock,contents,RD_SIZE);
                    // printf("ddddddddd\n");
                    // printf("contents: %s\n", contents);
                    send(sock, "Received\n", sizeof("Received\n"), 0);
                    write(fd,contents,sizeread);
                    filesize-=sizeread;
                    count++;
                    // printf("%d: %d\n", count, filesize);
                    // printf("%d alsoooo %d\n", backup_flsz-filesize,backup_flsz);
                    float perc = (float)100*(float)(((float)backup_flsz-(float)filesize)/(float)backup_flsz);
                    printf("\rDownloading file %s: %f%%", file_name, perc);
                }
                printf("\rDownload of file %s complete                        \n",file_name);
                char complete[100]={0};
                sizeread = read(sock, complete, RD_SIZE);
                send(sock, "complete\n", sizeof("complete\n"), 0);
                // printf("compleeeted\n");
            }
            close(fd);
        }
        else
        {
            send(sock, "Received\n", sizeof("Received\n"), 0);
        }
    }


    // valread = read(sock , buffer, 1024);  // receive message back from server, into the buffer
    // printf("%s\n",buffer);
    memset(req,0,4096);
    printf("End\n");
    return 0;
}
