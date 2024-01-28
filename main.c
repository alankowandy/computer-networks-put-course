#include "utils.h"

// Funkcja konfigurująca serwer na podstawie argumentów wejściowych
int configure_server(sockaddr_in* address, int argc, const char* argv[]) {
    int port = 12345;
    
    // Sprawdzanie liczby argumentów - jeżeli więcej niż 2 argumenty to zwracamy -1
    if (argc > 2){
        return -1;
    }
    
    // Jeżeli podano port jako argument to ustaw podany
    if (argc == 2) {
        port = atoi(argv[1]);
    }
    
    // Wyświetlanie informacji o konfiguracji serwera
    printf("[main_thread] Konfiguruje serwer na porcie: %d \n", port);
    
    // Inicjalizacja struktury sockaddr_in
    memset(address, 0, sizeof(sockaddr_in));
    address -> sin_family = AF_INET;
    address -> sin_addr.s_addr = INADDR_ANY;
    address -> sin_port = htons(port);
    
    return 0;
}

// Funkcja główna programu
int main(int argc, const char* argv[]) {
    int server_socket_file_descriptor;
    int options = 1;
    sockaddr_in address;
    
    // Konfiguracja serwera na podstawie argumentów wejściowych
    if (configure_server(&address, argc, argv) == -1) {
        perror("Niepoprawne argumenty wywołania programu \n");
        exit(1);
    }

    sockaddr* server_address = (sockaddr*)&address;
    
    // Tworzenie gniazda
    if ((server_socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Problem z tworzeniem gniazda \n");
        exit(1);
    }
    
    // Ustawianie opcji gniazda
    if (setsockopt(server_socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &options, sizeof(options))) {
        perror("Problem z ustawieniem opcji gniazda \n");
        exit(1);
    }

    // Powiązywanie gniazda z adresem IP
    if ((bind(server_socket_file_descriptor, server_address, sizeof(address))) == -1 ) {
        perror("Problem powiązania gniazda z adresem IP \n");
        exit(1);
    }
    
    // Oczekiwanie na połączenia
    if (listen(server_socket_file_descriptor, MAX_CLIENTS) == -1) {
        perror("Problem z przygotowaniem gniazda \n");
        exit(1);
    }
    
    // Przygotowanie katalogu do zapisywania
    printf("[main_thread] Przygotowuje katalog do zapisywania \n");
    if (build_directory() < 0) {
        perror("Problem z budowaniem katalogu \n");
        exit(1);
    }
    
    printf("[main_thread] Jestem gotowy na przyjmowanie połączeń! \n");
    
    while(1) {
        // Inicjalizacja struktury klienta
        Client* client = malloc(sizeof(client));
        socklen_t* client_address_size = malloc(sizeof(socklen_t));
        *client_address_size = sizeof(client -> address);
        
        printf("[main_thread] Oczekuje na połączenie \n");
        
        // Akceptowanie połączenia
        if ((client -> file_descriptor = accept(server_socket_file_descriptor, (struct sockaddr*)&(client->address), client_address_size)) == - 1) {
            perror("Problem z przyjęciem połączenia \n");
            free(client);
            free(client_address_size);
            continue;
        }
        
        // Tworzenie nowego wątku do obsługi połączenia
        pthread_t thread_id;
        
        printf("[main_thread] Nowe połączenie przychodzące z adresu: %s \n", inet_ntoa(client->address.sin_addr));
        if (pthread_create(&thread_id, NULL, handle_incoming_connection, client) == -1) {
            perror("Problem z tworzeniem nowego wątku \n");
            free(client);
            free(client_address_size);
            continue;
        }
        
        // Odczepianie wątku i zwolnienie jego zasobów
        pthread_detach(thread_id);
    }

    return 0;
}
