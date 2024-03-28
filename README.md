# Server-Client Application with Concurrency Management

This server-client model exemplifies advanced socket programming in C, incorporating thread pools for efficient request handling, mutexes for thread synchronization and avoiding race conditions, and condition variables to manage thread wake-up calls efficiently. Designed for educational purposes, it showcases a comprehensive approach to handling HTTP requests and console interactions.

## Core Features

- **Thread Pool Implementation**: Leverages a fixed-size thread pool to manage incoming connections, optimizing resource use and system responsiveness.
- **Concurrency Control**: Utilizes mutexes and condition variables to ensure that access to shared resources is safely managed among multiple threads.
- **HTTP and Console Support**: Capable of serving HTML content to web clients and interacting with console-based clients, demonstrating versatile server functionality.

## Getting Started

To compile and run this application, follow these steps:

1. Compile the server and client:
    ```bash
    gcc -pthread -o server server.c
    gcc -o client client.c
    ```
2. Start the server:
    ```bash
    ./server
    ```
3. In a separate terminal, initiate the client:
    ```bash
    ./client
    ```

To view the HTML content served by the server, navigate to `http://localhost:8088/daniel` in your web browser.

## Dive Into the Code

- `server.c`: The backbone of the server, handling socket connections, thread pool management, and request processing.
- `client.c`: Connects to the server, sending requests and displaying responses.
- `daniel.html`: Sample HTML file served by the server when accessed via a browser.

## Contributing

Your contributions can help enhance this project further. Whether it's improving the concurrency model, adding new features, or fixing bugs, your input is valued. Please fork the repository and submit pull requests with your changes.

## License

This project is licensed under the MIT License, supporting open-source collaboration and sharing of knowledge.
