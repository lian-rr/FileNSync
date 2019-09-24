# FileNsync

FileNSync is an applicaction that syncronize two directories

## How to use
The app can work as server or client depending of the arguments.
The instance that will act as the server needs to be running when the client instances gets executed.

#### As server:
To work as the server you only need to pass the directory path to sync.

    ./fileNsync <dir path>

**Example:** `./fileNsync content/` where *content* is the directory to sync.

#### As client:
To work as the client you need to pass the directory path to sync and the address of the server (including the port: **5555**).

    ./fileNsync <dir path> <server address>

**Example:** `./fileNsync content/ localhost:5555` where *content* is the directory to sync and *localhost:5555* is the address of the server.
