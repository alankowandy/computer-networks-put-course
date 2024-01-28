from os import listdir, path, getpid
from socket import socket, AF_INET, SOCK_STREAM
from multiprocessing import Process

BUFFER_SIZE = 512

def get_files_names() -> list:
    filenames = []
    for filename in listdir('.'):
        fullname = path.join('.', filename)
        if path.isfile(fullname) and not filename.startswith('.') and not filename.endswith('.py') and not filename.endswith('.zip'):
            filenames.append((filename, path.getsize(fullname)))
    return filenames

def prepare_header() -> str:
    results = []
    for name, size in get_files_names():
        results.append(name)
        results.append(str(size))
    
    header = f":1:1:{len(results) // 2}:{':'.join(results)}:#"

    size = len(header)

    size_of_size = len(str(size))
    size += size_of_size
    if len(str(size)) > size_of_size:
        size+= 1
    header = str(size) + header
    return header

def worker(host, port) -> None:    
    try: 
        with socket(AF_INET, SOCK_STREAM) as connection:
            connection.connect((host, port))
            connection.sendall(prepare_header().encode())   # wyślij header
            response = connection.recv(BUFFER_SIZE)         # odbierz ack
            for element in get_files_names():
                with open(element[0], 'rb') as source:
                    data = source.read()
                    connection.sendall(data)                # wyslij plik
                    response = connection.recv(BUFFER_SIZE) # odbierz ack
            response = connection.recv(BUFFER_SIZE)         # odbierz naglowek
            header = response.decode().split(':')
            filename = header[4]
            file_size = int(header[5])
            connection.sendall("%".encode())                # wyslij ack
            
            with open('downloads/' + filename, 'wb') as source:
                counter = 0
                bytes_counter = 0
                while True:                             
                    counter += 1
                    response = connection.recv(512)         # odbieraj plik
                    source.write(response)
                    bytes_counter += len(response)
                    if bytes_counter == file_size:
                        break
                print(f"PID:{getpid()} Zrobione")
                connection.sendall("%".encode()) 

    except ConnectionError as e:
        print(f"Błąd połączenia: {e}")

 
if __name__ == '__main__':
    host = '192.168.168.128'
    port = 12345
    clients = 15

    processes = [Process(target=worker, args=(host, port)) for i in range(clients)]

    for p in processes:
        p.start()

    for p in processes:
        p.join()
    
