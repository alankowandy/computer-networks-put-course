#importowanie bibliotek
#moduly tkinter - odpowiadaja za budowe interfejsu graficznego
import tkinter as tk
from tkinter import ttk
from tkinter import filedialog
from socket import socket, AF_INET, SOCK_STREAM
from os import listdir, path, mkdir
from time import sleep

FONT = ("Arial", 12)
BUFFER_SIZE = 512

#funkcja przygotowuje nagłówek do wysłania na serwer
#nagłówek zawiera informacje o pliklach, odbieraniai zapisywania
def prepare_header(files: list[(str, int)], send: bool, save: bool) -> str:
    #results - bedzie zawierać nazwy plików i ich rozmiary
    results = []

    #pętla przechodząca przez listę plików i dodająca nazwę i rozmiar do listy "results"
    for name, size in files:
        results.append(name)
        results.append(str(size))
    
    #tworzenie nagłówka - zawiera informacje o trybie odbierania/zapisywania, liczbie plikow ich nazwach oraz rozmiarach
    #nazwy i rozmairy bierzemy z tablicy results
    header = f":{'1' if save else '0'}:{'1' if send else '0'}:{len(results) // 2}:{':'.join(results)}:#"

    #długość nagłówka
    size = len(header)
    #obliczanie długości liczby reprezentującej długość nagłówka
    size_of_size = len(str(size))
    size += size_of_size
    # Sprawdzenie, czy długość liczby reprezentującej długość nagłówka zwiększyła się.
    # Jeśli tak, dodaj jeszcze jeden bajt do długości nagłówka = gdy z liczby 2 cyfrowej zmiana na 3 cyfrowa
    if len(str(size)) > size_of_size:
        size+= 1
    #dodanie na poczatku naglowka jego rozmiar-dlugosc
    header = str(size) + header
    return header


class Client():
    #inicjalizacja obiektu klienta
    def __init__(self) -> None:
        self.file_path = None #sciezka do pliku
        self.directory_path = None #sciezka do katalogu

        self.host = None #adres serwera
        self.port = None #adres portu
        self.socket = None #gniazdo do komunikacji z serwerem

        self.window = tk.Tk() #glowne okno interfejsu graficznego
        self.build_app() #wywołanie funkcji która buduje interfejs

        self.window.mainloop()#uruchomienie petli glowej Tkinter

    #metoda do dynamicznego aktualizowania etykiety w interfejsie
    #reprezentuje postep procesu w procentach
    def update_label(self):
        if self.counter:
            self.label_status_down.config(text=f"Odbieram plik {self.counter}%")
            self.window.after(1000, self.update_label) #automatyczne wywołanie self.update_label po 1000 milisekundach - co sekunde aktualizacja etykiety - dynamiczne odświeżanie informacji o postępie procesu.



    #metoda do aktualizacji etykiety statusu
    def set_status_up(self, message):
        self.label_status_up.config(text=message)
    #metoda do aktualizacji etykiety statusu
    def set_status_down(self, message):
        self.label_status_down.config(text=message)

    #wybór pliku lub katalogu za pomoca okna dialogowego
    def choose_file(self):
        self.file_path = filedialog.askopenfilename() #wywołanie okna dialogowe do wyboru pliku
        self.directory_path = None #wybralismy plik a nie katalog wiec none
        filename = self.file_path.split('/')[-1] #ze sciezki wybieramy tylko nazwę pliku
        if self.file_path: #jesli uzytkownik wybrał plik 
            self.button_direcory.config(state='disabled', cursor='arrow')
            self.button_file.config(state='disabled', cursor='arrow') 
            self.set_status_up(f"Wybrano plik: {filename}")
            self.button_send.config(state='normal', cursor='hand2')


    def choose_directory(self):
        self.directory_path = filedialog.askdirectory()
        self.file_path = None
        directory = self.directory_path.split('/')[-1]
        if self.directory_path:
            self.button_direcory.config(state='disabled', cursor='arrow')
            self.button_file.config(state='disabled', cursor='arrow')
            self.set_status_up(f"Wybrano katalog: {directory}")
            self.button_send.config(state='normal', cursor='hand2')

    #metoda nawiązuje połączenie z serwerem, używając adresu i portu podanych przez użytkownika.
    def connect(self):
        address = self.input_address.get() #pobranie adresu który został podany w oknie
        port = self.input_port.get() #pobranie numeru portu, który został podany w oknie

        self.socket = socket(AF_INET, SOCK_STREAM) #utworzenie gniazda TCP
        try:
            self.socket.connect((address, int(port))) #próba połączenia z serwerem

            self.file_path = None #zerowanie sciezki do pliku
            self.directory_path = None #zerwanie sciezki do katalogu 
            self.label_status_up.config(text=f"Połączono z {address}:{port}") # Aktualizacja etykiety statusu górnej informującej o połączeniu
            # Konfiguracja przycisków i pól wyboru po nawiązaniu połączenia
            self.button_connect.config(state="disabled", cursor='arrow')
            self.button_disconnect.config(state='normal', cursor='hand2')
            self.checkbox_recv.config(state='normal', cursor='hand2')
            self.checkbox_save.config(state='normal', cursor='hand2')
            self.button_direcory.config(state='normal', cursor='hand2')
            self.button_file.config(state='normal', cursor='hand2')

        # W przypadku błędu połączenia wywołuje metodę disconnect() i wypisuje komunikat o błędzie
        except ConnectionError as e:
            self.disconnect() #przywrocenie stanu poczatkowego interfejsu
            print(f"Błąd połączenia: {e}")

    
    def disconnect(self):
        #wyczyszczenie etykiet statusu
        self.set_status_down("")
        self.set_status_up("Rozłączono")
        #przywrócenie stanu początkowego przycisków i pól wyboru
        self.button_connect.config(state="normal", cursor='hand2')
        self.button_disconnect.config(state='disabled', cursor='arrow')
        self.checkbox_recv.config(state='disabled', cursor='arrow')
        self.checkbox_save.config(state='disabled', cursor='arrow')
        self.button_direcory.config(state='disabled', cursor='arrow')
        self.button_file.config(state='disabled', cursor='hand2')
        self.button_send.config(state='disabled', cursor='arrow')
        self.file_path = None
        self.directory_path = None
        if self.socket: #zamknięcie gniazda, jeśli jest otwarte
            self.socket.close()
            self.socket = None

    def send(self):
        self.counter = 0
        files = []
        if self.file_path: #sprawdzenie czy wybrano plik
            file_size = path.getsize(self.file_path) #pobranie rozmiaru pliku
            file_name = self.file_path.split('/')[-1] #pobranie nazwy pliku
            self.set_status_down(f"Wysyłam plik")
            files.append((file_name, file_size)) #do listy "files" dodajemy nazwe pliku i jego romziar
            header = prepare_header(files, self.have_to_recv.get(), self.have_to_save.get())
            files = [] #dlaczego tu czyscimy
            files.append(self.file_path)
        else:
            self.set_status_down(f"Wysyłam katalog")
            files_names = listdir(self.directory_path)
            for file_name in files_names:
                files.append((file_name, path.getsize(f"{self.directory_path}/{file_name}")))
            header = prepare_header(files, self.have_to_recv.get(), self.have_to_save.get())
            files = []
            for file_name in files_names:
                files.append(f"{self.directory_path}/{file_name}")

        # print(header)
        # print(files)

        self.socket.sendall(header.encode()) #wysłąnie nagłówka na serwe
        response = self.socket.recv(BUFFER_SIZE) #odbiór potwierdzenia od serwera

        for element in files: #dla kazdego pliku z tablicy files
            with open(element, 'rb') as source: #otwieramy plik
                data = source.read() #wczytujemy jego zawartosc
                self.socket.sendall(data) #wysylamy zawartosc pliku na serwer
                response = self.socket.recv(BUFFER_SIZE) # odbierz potwierdzenie ack od serwera

        
        if self.have_to_save.get(): #jesli opcja zapisu na serwerze 
            self.set_status_down(f"Plik zapisany na serwerze")
        self.set_status_up(f"Wysłałem")

        if self.have_to_recv.get(): #odbiór pliku od serwera
            response = self.socket.recv(BUFFER_SIZE) #odbior naglowak pliku od serwera
            header = response.decode().split(':') #dekodujemy naglowek - dzielenie na czesci z separatorem :
            filename = header[4]
            self.set_status_up(f"Wysłałem")
            self.set_status_down("")
            file_size = int(header[5])
            self.socket.sendall("%".encode()) #wysylamy potwierdzenie do serwera

            # Utworzenie katalogu do zapisu pliku
            if not path.isdir('./downloads'):
                mkdir('downloads')
                
            # Otwarcie pliku do zapisu
            with open('downloads/' + filename, 'wb') as source:
                bytes_counter = 0
                while True:                  
                    response = self.socket.recv(512)  #otwarcei pliku w kawalkach po 512 bajtow
                    source.write(response) #zapisanie otwartego kawalka
                    bytes_counter += len(response)
                    if bytes_counter == file_size: #jesli liczba odebranych bajtow jest rowna rozmiarowi pliku
                        self.set_status_up(f"Odebrałem {filename}")
                        self.socket.sendall("%".encode()) #wysylamy powtierdzenia ack do serwera
                        break
        self.button_send.config(state='disabled', cursor='arrow')
        self.socket.close()

    def build_app(self):
        self.window.geometry('400x300')
        self.window.resizable(False, False)
        self.window.title('Klient zdalnej kompresji')

        self.mainfame = tk.Frame(self.window)
        self.mainfame.pack(fill='both', expand=True)

        self.frame_connection = tk.Frame(self.mainfame)

        self.label_address = tk.Label(self.frame_connection, text="Host", font=FONT)
        self.label_port = tk.Label(self.frame_connection, text="Port", font=FONT)
        self.input_address = tk.Entry(self.frame_connection, font=FONT)
        self.input_address.insert(0, "localhost")
        self.input_port = tk.Entry(self.frame_connection, font=FONT)
        self.input_port.insert(0, "12345")

        self.label_address.grid(row=0, column=0, padx=5, pady=5)
        self.label_port.grid(row=0, column=1, padx=5, pady=5)
        self.input_address.grid(row=1, column=0, padx=5, pady=5)
        self.input_port.grid(row=1, column=1, padx=5, pady=5)

        
        self.frame_connection.pack()

        self.button_connect = tk.Button(self.mainfame, text='Połącz', cursor="hand2", width=47, font=("Arial", 10), command=self.connect)
        self.button_connect.pack(pady=5)

        self.button_disconnect = tk.Button(self.mainfame, text='Rozłącz', state='disabled', width=47, font=("Arial", 10), command=self.disconnect)
        self.button_disconnect.pack(pady=5)

        self.frame_actions = tk.Frame(self.mainfame)
        self.button_file = tk.Button(self.frame_actions, width=12, state='disabled', height=2, text='Wybierz plik', font=("Arial", 10), command=self.choose_file)
        self.button_direcory = tk.Button(self.frame_actions, width=12, state='disabled', height=2, text='Wybierz katalog', font=("Arial", 10), command=self.choose_directory)

        self.frame_options = tk.Frame(self.frame_actions)

        self.have_to_recv = tk.BooleanVar()
        self.have_to_save = tk.BooleanVar()

        self.checkbox_recv = tk.Checkbutton(self.frame_options, state='disabled', text="Odbieranie ", variable=self.have_to_recv)
        self.checkbox_save = tk.Checkbutton(self.frame_options, state='disabled', text="Zapisywanie", variable=self.have_to_save)
        self.checkbox_recv.grid(row=0, column=0, sticky="W")
        self.checkbox_save.grid(row=1, column=0, sticky="W")

        
        self.button_send = tk.Button(self.frame_actions, width=8, height=2, text='Wyślij', state='disabled', font=("Arial", 10), command=self.send)

        self.button_file.grid(row=0, column=0)
        self.button_direcory.grid(row=0, column=1, padx=4)
        self.frame_options.grid(row=0, column=2)
        self.button_send.grid(row=0,column=3)

        self.frame_actions.pack()

        self.label_status_up = tk.Label(self.mainfame, font=FONT, width=42, justify=tk.LEFT)
        self.label_status_down = tk.Label(self.mainfame, font=FONT, width=42, justify=tk.LEFT)
        self.label_status_up.pack(pady=5)
        self.label_status_down.pack()


if __name__ == '__main__':
    Client()
