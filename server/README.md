# MySN Server
## Compilation
To compile the server, use the following command:
> make all

The server will be compiled into a binary called "mysn_server.bin". <br>

Therefore, to run the server simply execute the following command:
> ./mysn_server.bin

Optionally, the server can be given two parameters: server IP and port. For instance, to have the server listening on 127.0.0.2:8081, execute:
> ./mysn_server.bin 127.0.0.2 8081

Not giving both parameters will default to 127.0.0.1:8080. <br>

To clean the directory, use the following command:
> make clean

## Usage
There is no interaction with the server application. To stop it, simply use Ctrl + C.