#include "utils.h"

// Funkcja obsługująca przychodzące połączenie w nowym wątku
void* handle_incoming_connection(void* _client) {
    // Konwersja argumentu do struktury Client
    Client client = *((Client*)_client);

    // Pobranie identyfikatora wątku
    pid_t thread_id = gettid();
    
    printf("[thread_id=%d] Obsługuje połączenie z %s \n", thread_id, inet_ntoa(client.address.sin_addr));
    
    // Alokacja bufora na dane
    char* buffer = malloc(sizeof(char) * BUFFER_SIZE + 1);
    
    printf("[thread_id=%d] Oczekuje na nagłówek wiadomości \n", thread_id);
    
    // Odbiór nagłówka
    if (receive_header(buffer, client.file_descriptor) == -1) {
        printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd nagłówka", thread_id);
        free(buffer);
        close(client.file_descriptor);
        return NULL;
    }
    printf("[thread_id=%d] Otrzymałem nagłówek \n", thread_id);
    
    
    printf("[thread_id=%d] Dekoduje nagłówek \n", thread_id);
    
    int have_to_save = 0;
    int have_to_send = 0;
    
    // Dekodowanie nagłówka
    Files* files = decode_header(buffer, &have_to_save, &have_to_send);
    
    // Sprawdzenie poprawności dekodowania nagłówka
    if (files == NULL) {
        printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd kodowania nagłówka", thread_id);
        free(buffer);
        close(client.file_descriptor);
        return NULL;
    }
    
    free(buffer);

    printf("[thread_id=%d] Zdekodowałem nagłówek \n", thread_id);
    

    printf("[thread_id=%d] Wysyłam potwierdzenie \n", thread_id);
    
    // Wysłanie potwierdzenia
    if (send_confirmation(client.file_descriptor) == -1) {
        printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd wysyłania potwierdzenia", thread_id);
        
        free_files(files);
        free(files);
        close(client.file_descriptor);
        return NULL;
    }
    
    printf("[thread_id=%d] Wysłałem potwierdzenie \n", thread_id);
    printf("[thread_id=%d] Odbieram pliki \n", thread_id);
    
    // Odbiór plików
    if (receive_files(client.file_descriptor, files, thread_id) < 0) {
        printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd odbierania plików", thread_id);
        free_files(files);
        free(files);
        close(client.file_descriptor);
        return NULL;
    }
    
    printf("[thread_id=%d] Odebrałem wszystkie pliki \n", thread_id);
    
    printf("[thread_id=%d] Kompresuje pliki \n", thread_id);
    
    // Kompresja plików
    if (compress_files(files, thread_id) != 0)  {
        printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd kompresji", thread_id);
        free_files(files);
        free(files);
        close(client.file_descriptor);
        return NULL;
    }
    
    printf("[thread_id=%d] Skompresowałem pliki \n", thread_id);
    
    printf("[thread_id=%d] Usuwam pliki sprzed kompresji \n", thread_id);
    
    // Usunięcie plików przed kompresją
    if (remove_directory(thread_id) < 0) {
        printf("[thread_id=%d] Problem z usunieciem katalogu \n", thread_id);
        free(files);
        close(client.file_descriptor);
        return NULL;
    }
    printf("[thread_id=%d] Pliki usunięte \n", thread_id);
    
    // Sprawdzenie czy należy wysłać skompresowany plik
    if (have_to_send) {
        printf("[thread_id=%d] Będe wysyłał plik skompresowany \n", thread_id);
        
        // Alokacja bufora na nowy nagłówek
        char* buffer = malloc(sizeof(char) * BUFFER_SIZE);
        
        // Kodowanie nagłówka dla pliku skompresowanego
        if (encode_header(buffer, files, 0, 0) < 0) {
            printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd kodowania nagłówka", thread_id);
            free(buffer);
            free(files);
            close(client.file_descriptor);
            return NULL;
        }
        
        // Wysłanie nowego nagłówka
        printf("[thread_id=%d] Wysyłam nagłówek \n", thread_id);
        if (send_header(buffer, client.file_descriptor) < 0) {
            printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd wysyłania nagłówka", thread_id);
            free(buffer);
            free(files);
            close(client.file_descriptor);
            return NULL;
        }
        printf("[thread_id=%d] Wysyłałem nagłówek \n", thread_id);
        
        printf("[thread_id=%d] Czekam na potwierdzenie \n", thread_id);
        
        // Odbiór potwierdzenia od klienta
        if (receive_confirmation(client.file_descriptor) < 0) {
            printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd w oczekiwaniu na potwierdzenie", thread_id);
            free(buffer);
            free(files);
            close(client.file_descriptor);
            return NULL;
        }
        
        free(buffer);
        
        printf("[thread_id=%d] Dostałem potwierdzenie \n", thread_id);

        
        printf("[thread_id=%d] Wysyłam plik zip \n", thread_id);
        
        // Wysłanie skompresowanego pliku
        if (send_file(client.file_descriptor, files -> files) < 0) {
            printf("[thread_id=%d] Kończę pracę ze wzlędu na błąd wysyłania pliku", thread_id);
            free(files);
            close(client.file_descriptor);
            return NULL;
        }
        
        printf("[thread_id=%d] Wysłałem plik zip \n", thread_id);
        
        // Odebranie potwierdzenia po wysłaniu skompresowanego pliku
        printf("[thread_id=%d] Czekam na potwierdzenie \n", thread_id);
        if (receive_confirmation(client.file_descriptor) < 0) {
            printf("[thread_id=%d] Kończę pracę ze wzlędu na wysyłania nagłówka", thread_id);
            free(files);
            close(client.file_descriptor);
            return NULL;
        }
        printf("[thread_id=%d] Dostałem potwierdzenie \n", thread_id);
        
    }
    else {
        printf("[thread_id=%d] Nie wysyłam plików, zgodnie z życzeniem \n", thread_id);
    }
    
    // Sprawdzenie, czy należy zapisać pliki
    if (have_to_save) {
        printf("[thread_id=%d] Zapsisuje pliki w katalogu downloads \n", thread_id);
        if (move_files(thread_id) == -1) {
            printf("[thread_id=%d] Problem z przeniesieniem pliku \n", thread_id);
            free(files);
            close(client.file_descriptor);
            return NULL;
        }
        printf("[thread_id=%d] Zapisalem pliki \n", thread_id);
    }
    else {
        printf("[thread_id=%d] Usuwam zip bo nie mialy być zapisane \n", thread_id);
        if (remove_file(thread_id) == -1) {
            printf("[thread_id=%d] Problem z usunieciem archiwum \n", thread_id);
            free(files);
            close(client.file_descriptor);
            return NULL;
        }

    }
    
    printf("[thread_id=%d] Skończyłem swoje zadanie \n", thread_id);
    
    // Zamknięcie połączenia i zwolnienie zasobów
    close(client.file_descriptor);
    free(files);
    return NULL;
}

// Funkcja odbierania nagłówka do której przekazujemy bufor oraz fd
int receive_header(char* buffer, int file_descriptor) {
    memset(buffer, EMPTY_SYMBOL, BUFFER_SIZE);
    int bytes_counter = 0;
    
    if ((bytes_counter = (int)recv(file_descriptor, buffer, BUFFER_SIZE, 0)) < 0) {
        perror("Błąd odbierania nagłówka \n");
        return -1;
    }
    
    if (bytes_counter == 0) {
        perror("Błąd odbierania nagłówka - brak nagłówka \n");
        return -1;
    }
    
    return bytes_counter;
}

// Funkcja wysyłania nagłówka do której przekazujemy bufor oraz deskryptor pliku
int send_header(char* buffer, int file_descriptor) {
    int bytes = (int)strlen(buffer);
    int bytes_counter = 0;
    
    // Wysyłanie nagłówka za pomocą funkcji send
    if ((bytes_counter = (int)send(file_descriptor, buffer, bytes, 0)) < 0) {
        perror("Błąd wysyłania nagłówka - nie wysłano \n");
        return -1;
    } 
    // Sprawdzenie czy wysłano cały nagłówek
    else if (bytes_counter != bytes) {
        printf("Wysłano tylko %d bajtów z %d \n", bytes_counter, bytes);
        perror("Błąd wysyłania nagłówka - nie wysłano całego nagłówka \n");
        return -1;
    }
    return 0;
}

// Funkcja dekodowania nagłówka, do której przekazujemy bufor oraz flagi zapisu i wysyłki
Files* decode_header(char* buffer, int* to_save, int* to_send) {
    // Przytnij bufor do znaku końca nagłówka
    char* token = strchr(buffer, END_SYMBOL);
    if (token == NULL) {
        perror("Błąd kodowania nagłówka - brak znaku końca \n");
        return NULL;
    }
    
    token[1] = EMPTY_SYMBOL;
        
    int header_size_calculated = (int)strlen(buffer); 
    token = strtok(buffer, SEPARATOR);
    int header_size = atoi(token);
    
    
    token = strtok(NULL, SEPARATOR);
    *to_save = atoi(token);
    token = strtok(NULL, SEPARATOR);
    *to_send = atoi(token);
    
    if (header_size_calculated != header_size) {
        perror("Błąd kodowania nagłówka - błędny rozmiar nagłówka \n");
        return NULL;
    }
    
    token = strtok(NULL, SEPARATOR);
    
    // Alokacja struktury Files
    Files* files = malloc(sizeof(Files));
    files -> size = atoi(token);
    files -> files = malloc(sizeof(File) * (files -> size));
    
    for (int i = 0; i < files -> size; i++) {
        File* file = malloc(sizeof(File));
        token = strtok(NULL, SEPARATOR);
        int temporary_name_size = (int)strlen(token);
        file -> filename = malloc(sizeof(char) * temporary_name_size + 1);
        file -> filename[temporary_name_size] = EMPTY_SYMBOL;
        memcpy(file -> filename, token, temporary_name_size);
        
        token = strtok(NULL, SEPARATOR);
        file -> size = atoi(token);
        
        files -> files[i] = *file;
    }
    
    token = strtok(NULL, SEPARATOR);
    
    if (token[0] == END_SYMBOL) {
        return files;
    }
    else {
        free_files(files);
        free(files);
        
        perror("Błąd kodowania nagłówka - niezdefiniowane dane \n");
        return NULL;
    }
}

// Funkcja tworząca nagłówek dla klienta
int encode_header(char* buffer, Files* files, int to_save, int to_send) {
    memset(buffer, EMPTY_SYMBOL, sizeof(BUFFER_SIZE));
    char number_buffer[10] = {EMPTY_SYMBOL};
    char* temp_header = calloc(BUFFER_SIZE, sizeof(char));
    
    strcat(temp_header, SEPARATOR);
    sprintf(number_buffer, "%d", to_save);
    strcat(temp_header, number_buffer);
    
    strcat(temp_header, SEPARATOR);
    sprintf(number_buffer, "%d", to_send);
    strcat(temp_header, number_buffer);
    
    strcat(temp_header, SEPARATOR);
    sprintf(number_buffer, "%d", files -> size);
    strcat(temp_header, number_buffer);
    strcat(temp_header, SEPARATOR);
    
    for (int i = 0; i < files -> size; i++) {
        strcat(temp_header, files -> files[i].filename);
        strcat(temp_header, SEPARATOR);
        sprintf(number_buffer, "%d", files -> files[i].size);
        strcat(temp_header, number_buffer);
        strcat(temp_header, SEPARATOR);
    }
    
    // Tymczasowy rozmiar nagłówka
    int temp_header_size = (int)strlen(temp_header);
    // Dodaj znak konca i zwieksz rozmiar o ten znak (o 1)
    temp_header[temp_header_size] = END_SYMBOL;
    temp_header_size++;    
    // Zamien rozmiar na znaki
    sprintf(number_buffer, "%d", temp_header_size);
    int size_of_size = (int)strlen(number_buffer);
    // Dodaj rozmiar rozmiaru do rozmiaru
    temp_header_size += size_of_size;
    
    // Sprawdz nowy rozmiar czy ma taka sama ilosc znakow
    sprintf(number_buffer, "%d", temp_header_size);
    if (size_of_size < (int)strlen(number_buffer)) {
        temp_header_size++;
    }
    // jesli nie to znaczy ze potrzeba jednego znaku wiecej np zmiana z 99 na 100
    sprintf(number_buffer, "%d", temp_header_size);
    
    strcat(buffer, number_buffer);
    strcat(buffer, temp_header);
    
    free(temp_header);
    return 0;
}

// Funkcja kompresująca pliki poprzez wbudowaną komendę w systemie Linux
int compress_files(Files* files, uint64_t thread_id) {
    // Deklaracje zmiennych
    char command[BUFFER_SIZE] = {EMPTY_SYMBOL};
    File* compressed = malloc(sizeof(File));
    char directory_name[FILENAME_SIZE] = {EMPTY_SYMBOL};
    char* filename = calloc(sizeof(directory_name), sizeof(char));

    // Tworzenie nazwy katalogu na podstawie identyfikatora wątku
    sprintf(directory_name, "%lu", thread_id);

    // Ścieżka do katalogu, gdzie znajdują się pliki
    char directory_path[BUFFER_SIZE] = {EMPTY_SYMBOL};
    memcpy(directory_path, PATH_SERVER, sizeof(PATH_SERVER));

    // Budowanie komendy do kompresji plików za pomocą polecenia zip -qq -r
    strcat(command, "cd ");
    strcat(command, directory_path);
    strcat(command, " && zip -qq -r ");
    strcat(command, directory_name);
    strcat(command, ".zip ");
    strcat(command, directory_name);
    strcat(command, "/");

    // Kompresowanie plików za pomocą polecenia systemowego
    if (system(command) != 0) {
        perror("Błąd kompresji \n");
        return -1;
    }

    // Dodanie rozszerzenia ".zip" do nazwy katalogu
    strcat(directory_name, ".zip");

    // Pełna ścieżka do skompresowanego pliku
    char path_to_zip[BUFFER_SIZE];
    memcpy(path_to_zip, PATH_SERVER, sizeof(PATH_SERVER));
    strcat(path_to_zip, directory_name);

    // Pobranie statystyk skompresowanego pliku
    struct stat stat_buffer;
    if (stat(path_to_zip, &stat_buffer) == -1) {
        perror("Błąd statystyk skompresowanego pliku \n");
        return -1;
    }

    // Przygotowanie nazwy pliku skompresowanego do przekazania
    memcpy(filename, directory_name, sizeof(directory_name));
    compressed->size = (int)stat_buffer.st_size;
    compressed->filename = filename;
    
    // Zwolnienie pamięci i ustawienie skompresowanego pliku w strukturze Files
    free_files(files);
    files->size = 1;
    files->files = compressed;

    return 0;
}

// Funkcja odbierająca pliki
int receive_file(int file_descriptor, File* file, uint64_t thread_id) {
    char buffer[BUFFER_SIZE] = {EMPTY_SYMBOL};
    char path[BUFFER_SIZE] = {EMPTY_SYMBOL};
    
    char directory_name[FILENAME_SIZE] = {EMPTY_SYMBOL};
    char directory_path [BUFFER_SIZE] = {EMPTY_SYMBOL};
    
    memcpy(path, PATH_SERVER, sizeof(PATH_SERVER));
    
    sprintf(directory_name, "%lu", thread_id);
    strcat(directory_path, path);
    strcat(directory_path, directory_name);
    
    memcpy(path, directory_path, sizeof(directory_path));
    strcat(path, "/");
    strcat(path, file -> filename);
    
    
    FILE* source = fopen(path, "wb");
    
    if (!source) {
        if (mkdir(directory_path, 0700) == -1) {
            perror("Nie mogłem utworzyc katalogu \n");
            return -1;
        }
        source = fopen(path, "wb");
        if (!source) {
            perror("Nie mogłem otworzyć pliku \n");
            return -1;
        }
    }
    
    int bytes_counter = file -> size;
    int bytes_received = 0;
    
    while (bytes_counter) {
        if ((bytes_received = (int)recv(file_descriptor, buffer, BUFFER_SIZE, 0)) < 0) {
            perror("Błąd odbierania pliku \n");
            fclose(source);
            return -1;
        } else if (bytes_received > bytes_counter) {
            fwrite(buffer, sizeof(char), bytes_counter, source);
            bytes_counter = 0;
        } else {
            bytes_counter -= bytes_received;
            fwrite(buffer, sizeof(char), bytes_received, source);
        }
    }
    fclose(source);
    return 0;
}

int receive_files(int file_descriptor, Files* files, uint64_t thread_id) {
    for (int i = 0; i < (files -> size); i++) {
        if (receive_file(file_descriptor, &(files -> files[i]), thread_id) == -1) {
            perror("Błąd odbierania plików \n");
            return -1;
        }
        send_confirmation(file_descriptor);
        
    }
    return 0;
}

int send_file(int file_descriptor, File* file) {
    char buffer[BUFFER_SIZE] = {EMPTY_SYMBOL};
    char file_path[BUFFER_SIZE] = {EMPTY_SYMBOL};
    memcpy(file_path, PATH_SERVER, sizeof(PATH_SERVER));
    strcat(file_path, file -> filename);
    
    FILE* source = fopen(file_path, "rb");
    if (!source) {
        perror("Nie mogłem otworzyć pliku \n");
        return -1;
    }
    
    int bytes_counter = file -> size;
    int bytes_send = 0;
    int bytes_read = 0;
    
    while (bytes_counter) {
        if ((bytes_read = (int)fread(buffer, sizeof(char), BUFFER_SIZE, source)) < 0) {
            perror("Błąd wczytywania pliku \n");
            fclose(source);
            return -1;
        }
        
        if (bytes_read == 0) {
            printf("BUFFER: %s \n", buffer);
            perror("Błąd wczytywania pliku - plik jest pusty \n");
            fclose(source);
            return -1;
        }
        
        if ((bytes_send = (int)send(file_descriptor, buffer, bytes_read, 0)) < 0) {
            perror("Błąd wysyłania pliku \n");
            fclose(source);
            return -1;
        }
        
        if (bytes_send != bytes_read) {
            perror("Błąd wysyłania pliku - nie wysłałem wszystkiego \n");
            fclose(source);
            return -1;
        }
        
        bytes_counter -= bytes_send;
        
    }
    fclose(source);
    return 0;
}

int send_files(int file_descriptor, Files* files) {
    for (int i = 0; i < (files -> size); i++) {
        if (send_file(file_descriptor, &(files->files[i])) == -1) {
            perror("Błąd wysyłania plików \n");
            return -1;
        }
        receive_confirmation(file_descriptor);
        
    }
    return 0;
}

int send_confirmation(int file_descriptor) {
    int bytes_counter = 0;
    
    char local_buffer[1] = {CONFIRM};
    
    if ((bytes_counter = (int)send(file_descriptor, local_buffer, 1, 0)) < 0) {
        perror("Błąd wysyłania potwierdzenia - nie wysłano \n");
        return -1;
    }
    
    if (bytes_counter != 1) {
        perror("Błąd wysyłania potwierdzenia - wysłano 0 bajtów \n");
        return -1;
    }
    else {
        return 0;
    }
}

int receive_confirmation(int file_descriptor) {
    int local_buffer_size = 2;
    char* local_buffer = malloc(sizeof(char) * local_buffer_size);
    memset(local_buffer, EMPTY_SYMBOL, local_buffer_size);
    
    int bytes_counter = 0;
    
    if ((bytes_counter = (int)recv(file_descriptor, local_buffer, local_buffer_size - 1, 0)) < 0) {
        perror("Błąd odbierania potwierdzenia - nie odebrano \n");
        return -1;
    }
    
    if (bytes_counter != 1) {
        perror("Błąd odbierania potwierdzenia - odebrano 0 \n");
        return -1;
    }
    else if (local_buffer[0] == CONFIRM) {
        return 0;
    }
    else {
        perror("Błąd odbierania potwierdzenia - niepoprawny znak \n");
        return -1;
    }
}

void free_files(Files* files) {
    for (int i = 0; i < (files -> size); i++) {
        if (files -> files[i].filename != NULL) {
            free(files -> files[i].filename);
            files -> files[i].filename = NULL;
        }
    }
    free(files -> files);
    files -> files = NULL;
}

int remove_file(uint64_t thread_id) {
    char file_path[BUFFER_SIZE] = {EMPTY_SYMBOL};
    char file_name[FILENAME_SIZE] = {EMPTY_SYMBOL};
    sprintf(file_name, "%lu", thread_id);
    
    memcpy(file_path, PATH_SERVER, sizeof(PATH_SERVER));
    strcat(file_path, file_name);
    strcat(file_path, ".zip");
    
    char command[BUFFER_SIZE] = {EMPTY_SYMBOL};
    strcat(command, "rm ");
    strcat(command, file_path);
    
    if (system(command) == 0) {
        return 0;
    }
    perror("Problem z usunięciem archiwum \n");
    return -1;
}

int build_directory(void) {
    struct stat status;
    if (stat(PATH_DOWNLOAD, &status) == -1) {
        if (mkdir(PATH_DOWNLOAD, 0700) == -1) {
            perror("Błąd katalogu do zapisywania, nie ma i nie moge utworzyć \n");
            return -1;
        }
        else if (stat(PATH_DOWNLOAD, &status) != -1) {
            return 0;
        }
    }
    return 0;
}

int move_files(uint64_t thread_id) {
    char command[BUFFER_SIZE] = {EMPTY_SYMBOL};
    char filename_buffer[FILENAME_SIZE] = {EMPTY_SYMBOL};
    sprintf(filename_buffer, "%lu", thread_id);
    strcat(command, "mv ");
    strcat(command, PATH_SERVER);
    strcat(command, filename_buffer);
    strcat(command, ".zip ");
    
    strcat(command, PATH_DOWNLOAD);
    strcat(command, "/");
    if (system(command) == 0) {
        return 0;
    }
    perror("Problem z zapisaniem archiwum \n");
    return -1;
}

int remove_directory(uint64_t thread_id) {
    char directory_name_buffer[BUFFER_SIZE] = {EMPTY_SYMBOL};
    char directory_name[FILENAME_SIZE] = {EMPTY_SYMBOL};
    sprintf(directory_name, "%lu", thread_id);
    
    memcpy(directory_name_buffer, PATH_SERVER, sizeof(PATH_SERVER));
    strcat(directory_name_buffer, directory_name);
    
    char command[BUFFER_SIZE] = {EMPTY_SYMBOL};
    strcat(command, "rm -rf ");
    strcat(command, directory_name_buffer);
    
    if (system(command) == 0) {
        return 0;
    }
    perror("Problem z usunięciem katalogu \n");
    return -1;
}
