# LoVCA
Local Video Comunication App

A decentralized, autonomous app for local video communication.


## Functionality so far:
Simple server-client UDP connection, where client sends a message to local server.

## How to run:
You'll need two terminals. 
First, choose the LoVCA folder, so that bin is within that folder.
1. Then in the first terminal write `./bin/test.out` (I assume **.out** is binary file for Linux only?). 
It runs the compiled binary program file that sets up the **server**.

2. Then in the first terminal write `./bin/client_test.out`.
It runs the compiled binary program file that sets up the **client**;
It instantly sends the message, and both programs terminate.<br/>
Message sent by client is displayed in the **server terminal**

## P.S. running not on Linux:
I think you can compile the **main.c** (server), and **client.c** elsewhere, using the compilator of your choice.<br/>
Then simply run it, and it presumably should work the same way.