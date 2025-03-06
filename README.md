# UDP Chatroom

## Description
A simple command line chat application that uses the User Datagram Protocol (UDP). The app consists of clients that send messages and a server that receives and processes these messages.

## Functionality
- **Multi-client Support**: Connect up to 10 clients simultaneously.
- **Real-time Messaging**: Send and receive messages in real-time.
- **User Registration**: Clients can register with a name and receive a unique ID.
- **Command Support**: Clients can execute commands to leave the chatroom or view all participants.
- **Cross-Platform**: Compatible with both Windows and Unix-like systems.

## Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/udp-chatroom.git
   cd udp-chatroom
   ```

2. Optionally, compile the application using the provided Makefile:
   ```bash
   make -C chatroom
   ```

## Usage
1. Build the project and start the server and two clients on `127.0.0.1:3000` with ```./launch.bat``` for windows or ```./launch.sh``` for UNIX.

2. OR build and start them separately:
   ```bash
   cd chatroom
   make
   ./client <serverrt>
   ./server <port>
   ```

3. Follow the prompts to register and type your messages directly in the terminal.

4. As client, type `!h` for help.

5. Optionally, send messages as server.

## Testing
To test project, run `cd chatroom && make check`
