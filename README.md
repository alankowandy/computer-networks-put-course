# Remote File Compression System

Respository created for the Computer Networks course on the 5th semester on computer science's major.

# Description of the communication protocol

1. The server waits for a connection.
2. The client connects to the server and selects a file/directory to compress. The client then sends a header consisting of: the header size, selected options for saving and receiving, the number of files, file names along with their sizes, and a header terminator, which is "#". Values are separated by ":".
3. The client waits for confirmation of header reception by the server.
4. Sending files for remote compression.
5. The server creates a directory and places the files in it, then compresses them.
6. The server removes the folder with the files and leaves the compressed file, then checks which options were selected - whether .zip is to be sent back and whether it is to be saved on the server.
7. If the option to send the compressed file back was selected, the server creates a header for the client and sends it.
8. The server receives confirmation of header reception by the client and sends the file.
9. If the option to save the compressed file on the server was selected, the server creates a directory and places the file in the previously defined path.

# Compilation

For the server:
1. Open a console in the server's location.
2. Enter the following commands:

gcc -Wall -pthread main.c utils.c -o server

Run for the standard port 12345:

./server

Run for port X:

./serwer X

For the client:
1. Run the client.exe file or open a console in the client's location and enter the command:

python client.py
