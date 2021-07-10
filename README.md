## How to run:

1. First compile and run the server file.
	- gcc -o server server.c
	- ./server
2. Compile and run the client file. The files to be downloaded must be passed as command-line arguments when running ./client.
	- gcc -o client client.c
	- ./client "filenames"


I have implemented client.c such that it makes requests using command-line arguments. The request sent to the server is made within client.c and matches the format given in problem statement. Since the client has been implemented to function using command-line arguments, the "exit" command for clinets has not been implemented.