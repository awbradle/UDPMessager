Andrew Bradley
COSC 522
Winter 2016
Project #1

Compiling: Use the following console command to compile:

To compile the Server: gcc newServer.c DieWithError.c -l sqlite3 -o newServer
To compile the Client: gcc newClient.c DieWithError.c -l sqlite3 -o newClient

To Run the Server: ./newServer <PORT>
Note: The Port must be between 20000 and 30000

To Run the Client: ./newClient
Note: The program will prompt the user for their ID, Server IP and Server Port upon
execution. Client ID's must be 9 or fewer characters

The message.db database is included with these files. Tables have already been created
but should you need to recreate the database, the schema is saved in createMessagDB.sql

Code was borrowed from the echo server and echo client demo programs and sending and
receiving datagrams was built on that. Code for querying the database using the sqlite3
interface for C was modified from demo segments provided by Tutorials Point.
https://www.tutorialspoint.com/sqlite/sqlite_c_cpp.htm

Contents:

newServer.c         - Server source code
newClient.c         - Client source code
DieWithError.c      - Handles creating error messages for socket errors
message.db          - The database for storing signins, followers, and messages
createMessageDB.sql - Database schema
readme.txt          - This readme
