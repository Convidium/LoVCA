# LoVCA
Local Video Comunication App

A decentralized, autonomous app for local video communication.


## Functionality so far:
Simple server-client UDP/TCP connection, where client sends a message to local server.

# How to run:

## UDP Server / CLient
You'll need two terminals. 
First, choose the LoVCA folder, so that bin is within that folder.
1. Then in the first terminal write `./bin/server_udp.out` (I assume **.out** is binary file for Linux only?). 
It runs the compiled binary program file that sets up the **server**.

2. Then in the second terminal write `./bin/client_udp.out`.
It runs the compiled binary program file that sets up the **client**;
It instantly sends the message, and both programs terminate.<br/>
Message sent by client is displayed in the **server terminal**

## TCP Server / Client
You'll need at least two terminals. 
Make sure you're in the LoVCA folder (like with UDP)
1. Then in the first terminal write `./bin/server_tcp.out` (**.out** is a binary file for Linux only). 
It runs the compiled binary program file that sets up the **server**.

2. Then in the second terminal write `./bin/client_tcp.out localhost nickname`.
`localhost` (or alternatively `127.0.0.1`) is the local network of your computer, a network which is
used for communication between **server** and **client** processes. We write it, so that our **client** knows 
where to look for the **server**.
Instead of `nickname` you can actually write anything you'd like, its your nickname that the server will see,
when you'll try to connect to it.

3. You can also add as many clients as you like (max limit is 20). They all would work identically.

### How TCP server works:
The entire point of my TCP **server** is to send messages to an arbitrary amount of clients (max: 20),
and the connected **client**'s simply recieve it. If a server decides to terminate the communication
it does so via pressing `Ctrl + D` or writing `exit` in the **server terminal**. **Client**'s then get disconnected,
and the **server** terminates.
**Server** simply distributes the same message to all connected **clients**, so they're the same across every **client**.

## P.S. running not on Linux:
I think you can compile the **server_xxx.c** (server), and **client_xxx.c** elsewhere, using the compiler of your choice.<br/>
Then simply run it, and it presumably should work the same way. (not guaranteed)
**Target OS**: Linux / macOS / POSIX-compliant systems.
**Windows Note**: For Windows, use WSL (Windows Subsystem for Linux)