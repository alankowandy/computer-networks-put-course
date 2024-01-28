# sieci_komputerowe
# System zdalnej kompresji plików

# Opis protokołu komunikacyjnego
1. Serwer czeka na połączenie
2. Klient podłącza się do serwera i wybiera plik/katalog do skompresowania. Klient
następnie wysyła nagłówek składający się kolejno z: wielkości nagłówka, wybranych
opcji zapisu i odbioru, ilości plików, nazw plików wraz z ich wielkością oraz znaku
kończącego nagłówek, czyli „#”. Wartości oddzielone są znakiem „:”.
3. Klient czeka na potwierdzenie odebrania nagłówka przez serwer.
4. Wysłanie plików do zdalnej kompresji.
5. Serwer tworzy katalog i umieszcza w nim pliki, a następnie je kompresuje.
6. Serwer usuwa folder z plikami i pozostawia skompresowany plik, a następnie
sprawdza jakie opcje zostały wybrane – czy .zip ma zostać odesłany i czy ma zostać
zapisany na serwerze.
7. Jeżeli została wybrana opcja odesłania skompresowanego pliku, to serwer tworzy
nagłówek dla klienta i go wysyła.
8. Serwer odbiera potwierdzenie odebrania nagłówka przez klienta i wysyła plik.
9. Jeżeli została wybrana opcja zapisanie skompresowanego pliku na serwerze, to
serwer tworzy katalog i umieszcza plik w wcześniej zdefiniowanej ścieżce.

# Kompilacja
Dla serwera:
1. Otworzyć konsolę w lokalizacji serwera
2. Wpisujemy kolejno komendy:
gcc -Wall -pthread main.c utils.c -o serwer
Uruchomienie dla standardowego portu 12345:
./serwer
Uruchomienie dla portu X:
./serwer X

Dla klienta:
1. Uruchamiamy plik client.exe lub otwieramy konsolę w lokalizacji klienta i wpisujemy
komendę:
python client.py
