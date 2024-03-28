#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>

#define THREAD_POOL_SIZE 20



typedef struct Args
{
    int new_socket;
    char *ip_address;
    char *ip_client;
    uint16_t port;
}Args;

typedef struct Node {
    Args *args;
    struct Node *next;
}Node;

typedef struct MyQueue
{
    Node *head;
    Node *tail;
}MyQueue;

int serverfd;
bool running = true;
// we create THREAD_POOL_SIZE threads to handle the connections
// the size is determined by testing, too many threads it's slow,
// since switching between threads takes time; if too few threads
// and if multiple connections request let's say 2gb, then the server
// is unable to accept() other connections until one of them is handled
pthread_t thread_pool[THREAD_POOL_SIZE];
// a mutex is needed when a critical region is entered - that is when
// a shared variable changes state - and multiple threads may change
// its state simultanously, make sure that only the one that grabs the
// lock() first can do stuff with that variable until we unlock()
// then, the next thread can grab the lock() and so on.
// thus, achieving mutual exclusion
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// when we pthread_cond_signal() a condition variable, one of
// the threads will wake up and handle stuff. otherwise, no
// busy waiting in a while(1) loop
// pthread_cond_wait() waits for a signal and wakes up the
// thread, otherwise everything is blocked - no busy waiting
// can be seen that "else" clause is never executed - that is,
// only when we have sth to handle, we do it, when it's NULL,
// the condition isn't met and it's waiting
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

// queue for connection requests
MyQueue *myQueue;

void enqueue(Args *args)
{
    Node *node = (Node*) malloc(sizeof(Node));
    node->args = args;
    node->next = NULL;
    if(myQueue->tail == NULL)
    {
        myQueue->head = node;
    }
    else
    {
        myQueue->tail->next = node;
    }
    myQueue->tail = node;
}

Args* dequeue()
{
    if(myQueue->head == NULL)
    {
        return NULL;
    }
    Args *args = myQueue->head->args;
    Node *tmp =  myQueue->head;
    myQueue->head =  myQueue->head->next;
    if(myQueue->head == NULL)
    {
        myQueue->tail = NULL;
    }
    free(tmp);
    return args;
}


void* handle_connection(void *p_args)
{
    char response[2048];
    char request[1024];
    Args *args = (Args*) p_args;
    int request_length;
    read(args->new_socket, &request_length, sizeof(int));
    read(args->new_socket, request, request_length);
    printf("Client sent %s\n", request);
    
    if(strstr(request, "HTTP"))
    {
        char *response_format = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n%s";
        char *requested_page = strtok(request, " ");
        printf("Client requests webpage:%s\n", requested_page);
        if(strcmp(requested_page, "/daniel") == 0)
        {
            FILE *fp = fopen("daniel.html", "r");
            char file_buf[512]; // not filesize safe!
            memset(file_buf, 0, 512);
            fseek(fp, 0, SEEK_END);
            int file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            fread(file_buf, sizeof(char), file_size, fp);
            char response[1024];
            sprintf(response, response_format, file_buf);
            fclose(fp);
        }
        else
        {
            printf("Client requested a random page that we don't have\n");
            printf("We send a default page 'hello world'\n");
            char *html_page_format = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>Hello world page %s IP %s:%d</h1></body></html>";
            char html_page[512];
            sprintf(html_page, html_page_format, requested_page, args->ip_address, args->port);
        }
    }
    // client starts the app from the terminal
    else
    {
        sprintf(response, "Hello client %s who said %s\n", args->ip_client, request);
        // if the request contains the word "trimite", then split after space
        // if whatever is left ends in .txt, try to open that file
        // if file exists - display contents, else, error msg
    }
    int response_size;
    response_size = strlen(response) + 1;
    write(args->new_socket, &response_size, sizeof(int));
    write(args->new_socket, response, response_size);
    close(args->new_socket);
    free(args->ip_address);
    free(args->ip_client);
    free(args);
    return NULL;
}

void* thread_function(void *p_args)
{
    while(running)
    {
        Args *args;
        pthread_mutex_lock(&mutex);
        // wait for the condition to be met - that is when pthread_cond_signal() is called
        pthread_cond_wait(&cond_var, &mutex);
        // we can dequeue() a node, because it was enqueued, because after enqueue(), pthread_cond_signal() gets called
        args = dequeue();
        pthread_mutex_unlock(&mutex);
        if(args != NULL)
        {
            printf("\n\nA thread will handle the connection\n\n");
            handle_connection((void*) args);
        }
        else
        {
            printf("wasting cycles - never happening - condition not met! :-) (unless we exit)\n");
        }
    }
    return NULL;
}

void handler(int signum)
{
    running = false;
    printf("Exiting server gracefully - clearing up all memory\n");
    pthread_cond_broadcast(&cond_var);
    close(serverfd);
    free(myQueue);
    for(int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_join(thread_pool[i], NULL);
    exit(EXIT_SUCCESS);
}
int main(int argc, char **argv)
{
    char *ip_address = "127.0.0.1";
    char *port = "8088";
    myQueue = (MyQueue*) malloc(sizeof(MyQueue));
    myQueue->head = NULL;
    myQueue->tail = NULL;
    //- the server should create a listening socket bound to port 8080 (for example)
    if((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addresss;
    server_addresss.sin_family = AF_INET;
    server_addresss.sin_addr.s_addr = INADDR_ANY;
    server_addresss.sin_port = htons(atoi(port)); // host to network short
    // binding the socket
    if(bind(serverfd, (struct sockaddr*) &server_addresss, sizeof(server_addresss)) == -1)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    int num_connections = 3;
    if(listen(serverfd, num_connections) == -1)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on %s:%s\n", ip_address, port);

    // create a thread pool that dequeues() connections and passes them to the handle_connection function
    for(int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);

    signal(SIGINT, handler);
    while(true)
    {
        // accept connections until server shutdown
        int new_socket;
        struct sockaddr_in client_address;
        socklen_t addr_len = sizeof(client_address);
        if((new_socket = accept(serverfd, (struct sockaddr*)&client_address, &addr_len)) == -1)
        {
            perror("accept()");
            exit(EXIT_FAILURE);
        }
        char ip_client[32];
        sprintf(ip_client, "%s", inet_ntoa(client_address.sin_addr));
        printf("Client %s connected successfully\n", ip_client);
        Args *args = (Args*) malloc(sizeof(Args));
        args->new_socket = new_socket;
        args->port = htons(client_address.sin_port);
        args->ip_address = (char*) malloc((strlen(ip_address)+1)*sizeof(char));
        strcpy(args->ip_address, ip_address);
        args->ip_client = (char*) malloc((strlen(ip_client)+1)*sizeof(char));
        strcpy(args->ip_client, ip_client);

        // enter the critical region when a node is enqueued
        pthread_mutex_lock(&mutex);
        // critical section of the code
        enqueue(args);
        // end of critical section
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond_var); // wake up a thread when the condition is met (we enqueued a connection)
        printf("Client %s disconnected succesfully\n", ip_client);
    }
    // it never reaches this statement
    return 0;
}