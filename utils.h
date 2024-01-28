#ifndef utils_h
#define utils_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

// Maksymalna liczba klientów obsługiwanych przez serwer
#define MAX_CLIENTS     20

// Rozmiar bufora używanego do odbierania i wysyłania danych
#define BUFFER_SIZE     512

// Maksymalna długość nazwy pliku
#define FILENAME_SIZE   32

// Symbol końca nagłówka
#define END_SYMBOL      '#'

// Pusty symbol używany do separacji danych w nagłówku
#define EMPTY_SYMBOL    '\0'

// Symbol potwierdzenia
#define CONFIRM         '%'

// Separator używany do oddzielenia danych w nagłówku
#define SEPARATOR       ":"

// Ścieżka do katalogu głównego serwera
#define PATH_SERVER     "/home/alan/Desktop/sieci/"

// Ścieżka do katalogu w którym zapisywane są odebrane pliki
#define PATH_DOWNLOAD   "/home/alan/Desktop/sieci/downloads"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

// Struktura reprezentująca informacje o pliku
typedef struct File {
    int     size;       // Rozmiar pliku
    char*   filename;   // Nazwa pliku
} File;

// Struktura reprezentująca listę plików
typedef struct Files {
    int     size;       // Liczba plików w liście
    File*   files;      // Tablica plików
} Files;

// Struktura reprezentująca klienta
typedef struct client {
    int         file_descriptor;    // Deskryptor pliku klienta
    sockaddr_in address;           // Adres klienta
} Client;

void*   handle_incoming_connection(void* client);

int     receive_header(char* buffer, int file_descriptor);

int     send_header(char* buffer, int file_descriptor);

Files*  decode_header(char* buffer, int* to_save, int* to_send);

int     encode_header(char* buffer, Files* files, int to_save, int to_send);

int     compress_files(Files* files, uint64_t thread_id);

int     receive_file(int file_descriptor, File* file, uint64_t thread_id);

int     receive_files(int file_descriptor, Files* files, uint64_t thread_id);

int     send_file(int file_descriptor, File* file);

int     send_files(int file_descriptor, Files* files);

int     send_confirmation(int file_descriptor);

int     receive_confirmation(int file_descriptor);

void    free_files(Files* files);

int     remove_file(uint64_t thread_id);

int     build_directory(void);

int     move_files(uint64_t thread_id);

int     remove_directory(uint64_t thread_id);

#endif /* utils_h */
