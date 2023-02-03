#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// HTTP /////////////////////////////////////////////////////////////////////////

typedef struct
{
    char **keys;
    char **values;
    int num_headers;
} Header;

typedef struct
{
    char **keys;
    char **values;
    int num_params;
} QueryParameters;

typedef struct
{
    char *method;
    char *path;
    char *path_without_query;
    char *version;
    QueryParameters *query_parameters;
    Header *headers;
    char *body;
} HttpRequest;

typedef struct
{
    char *version;
    int status_code;
    char *status_message;
    Header *headers;
    char *body;
} HttpResponse;

void parse_request(char *request, HttpRequest *http_request);

void parse_first_line(char *first_line, int first_line_length, HttpRequest *http_request);

char *parse_path(QueryParameters *query_parameters, char *path);

void parse_header(char *header, int header_length, Header *http_header);

HttpResponse *response(int status_code, char *status_message, char *body);

void add_header(HttpResponse *http_response, char *key, char *value);

char *serialize_response(HttpResponse *http_response);

void parse_request(char *request, HttpRequest *http_request)
{
    char *first_line_end = strstr(request, "\r\n");
    int first_line_length = first_line_end - request;
    char *first_line = malloc(first_line_length + 1);
    strncpy(first_line, request, first_line_length);
    first_line[first_line_length] = '\0';
    parse_first_line(first_line, first_line_length, http_request);
    char *header_end = strstr(first_line_end + 2, "\r\n\r\n");
    int header_length = header_end - first_line_end - 2;
    char *header = malloc(header_length + 1);
    strncpy(header, first_line_end + 2, header_length);
    header[header_length] = '\0';
    http_request->headers = malloc(sizeof(Header));
    parse_header(header, header_length, http_request->headers);
    int body_length = strlen(request) - (header_end - request) - 4;
    char *body = malloc(body_length + 1);
    strncpy(body, header_end + 4, body_length);
    body[body_length] = '\0';
    http_request->body = body;
}

void parse_first_line(char *first_line, int first_line_length, HttpRequest *http_request)
{
    char *method = strtok(first_line, " ");
    char *path = strtok(NULL, " ");
    char *version = strtok(NULL, " ");
    http_request->method = method;
    http_request->path = path;
    http_request->version = version;
    http_request->query_parameters = malloc(sizeof(QueryParameters));
    http_request->path_without_query = parse_path(http_request->query_parameters, path);
}

char *parse_path(QueryParameters *query_parameters, char *path)
{
    char *path_copy = malloc(strlen(path) + 1);
    strcpy(path_copy, path);
    char *path_without_query = malloc(strlen(path) + 1);
    strcpy(path_without_query, path);
    char *query_start = strstr(path_copy, "?");
    if (query_start != NULL)
    {
        int path_without_query_length = query_start - path_copy;
        path_without_query[path_without_query_length] = '\0';
        char *query = query_start + 1;
        char *key = strtok(query, "=");
        char *value = strtok(NULL, "&");
        query_parameters->keys = malloc(sizeof(char *));
        query_parameters->values = malloc(sizeof(char *));
        query_parameters->keys[0] = key;
        query_parameters->values[0] = value;
        query_parameters->num_params = 1;
        while (value != NULL)
        {
            key = strtok(NULL, "=");
            value = strtok(NULL, "&");
            if (key != NULL && value != NULL)
            {
                query_parameters->keys = realloc(query_parameters->keys, sizeof(char *) * (query_parameters->num_params + 1));
                query_parameters->values = realloc(query_parameters->values, sizeof(char *) * (query_parameters->num_params + 1));
                query_parameters->keys[query_parameters->num_params] = key;
                query_parameters->values[query_parameters->num_params] = value;
                query_parameters->num_params++;
            }
        }
    }
    return path_without_query;
}

void parse_header(char *header, int header_length, Header *http_header)
{
    char *header_copy = malloc(header_length + 1);
    strncpy(header_copy, header, header_length);
    header_copy[header_length] = '\0';
    char *key = strtok(header_copy, ":");
    char *value = strtok(NULL, "\r");
    http_header->keys = malloc(sizeof(char *));
    http_header->values = malloc(sizeof(char *));
    http_header->keys[0] = key;
    http_header->values[0] = value + 1;
    http_header->num_headers = 1;
    while (value != NULL)
    {
        key = strtok(NULL, ":");
        value = strtok(NULL, "\r");
        if (key != NULL && value != NULL)
        {
            key = key + 1;
            value = value + 1;
            http_header->keys = realloc(http_header->keys, sizeof(char *) * (http_header->num_headers + 1));
            http_header->values = realloc(http_header->values, sizeof(char *) * (http_header->num_headers + 1));
            http_header->keys[http_header->num_headers] = key;
            http_header->values[http_header->num_headers] = value;
            http_header->num_headers++;
        }
    }
}

HttpResponse *response(int status_code, char *status_message, char *body)
{
    HttpResponse *http_response = malloc(sizeof(HttpResponse));
    http_response->status_code = status_code;
    http_response->status_message = status_message;
    http_response->body = body;
    http_response->headers = malloc(sizeof(Header));
    http_response->headers->num_headers = 0;
    return http_response;
}

void add_header(HttpResponse *http_response, char *key, char *value)
{
    http_response->headers->keys = realloc(http_response->headers->keys, sizeof(char *) * (http_response->headers->num_headers + 1));
    http_response->headers->values = realloc(http_response->headers->values, sizeof(char *) * (http_response->headers->num_headers + 1));
    http_response->headers->keys[http_response->headers->num_headers] = key;
    http_response->headers->values[http_response->headers->num_headers] = value;
    http_response->headers->num_headers++;
}

char *serialize_response(HttpResponse *http_response)
{
    char *status_line = malloc(100);
    sprintf(status_line, "HTTP/1.1 %d %s\r\n", http_response->status_code, http_response->status_message);
    char *headers = malloc(1000);
    strcpy(headers, "");
    for (int i = 0; i < http_response->headers->num_headers; i++)
    {
        char *header = malloc(100);
        sprintf(header, "%s: %s\r\n", http_response->headers->keys[i], http_response->headers->values[i]);
        strcat(headers, header);
    }
    char *body = malloc(1000);
    strcpy(body, "");
    if (http_response->body != NULL)
    {
        sprintf(body, "\r\n%s", http_response->body);
    }
    char *response = malloc(strlen(status_line) + strlen(headers) + strlen(body) + 1);
    strcpy(response, status_line);
    strcat(response, headers);
    strcat(response, body);
    return response;
}

// ThreadPool ///////////////////////////////////////////////////////////////////

typedef struct
{
    void *(*worker)(void *);
    void *args;
} Task;

typedef struct
{
    Task **tasks;
    int size;
    int start;
    int end;
    pthread_mutex_t lock;
} TaskQueue;

typedef struct
{
    TaskQueue *taskQueue;
    int numThreads;
    int open;
    sem_t semaphore;
    pthread_t *workerThreads;
} ThreadPool;

void createTaskQueue(TaskQueue *taskQueue, int size);

void enqueueTaskQueue(TaskQueue *taskQueue, Task *task);

Task *dequeueTaskQueue(TaskQueue *taskQueue);

int isTaskQueueEmpty(TaskQueue *taskQueue);

void createThreadPool(ThreadPool *threadPool, TaskQueue *taskQueue, int (*priority)(void *), int numThreads, int queueSize);

void addTaskThreadPool(ThreadPool *threadPool, void *(*function_worker)(void *), void *args);

void *worker(void *args);

void waitThreadPool(ThreadPool *threadPool);

void createTaskQueue(TaskQueue *taskQueue, int size)
{
    taskQueue->tasks = (Task **)malloc(sizeof(Task *) * size);
    for (size_t i = 0; i < size; i++)
        taskQueue->tasks[i] = NULL;
    taskQueue->start = 0;
    taskQueue->end = 0;
    taskQueue->size = size;
    pthread_mutex_init(&taskQueue->lock, NULL);
}

void enqueueTaskQueue(TaskQueue *taskQueue, Task *task)
{
    if (taskQueue->start == (taskQueue->end + 1) % taskQueue->size)
        return;
    pthread_mutex_lock(&taskQueue->lock);
    taskQueue->tasks[taskQueue->end] = task;
    taskQueue->end = (taskQueue->end + 1) % taskQueue->size;
    pthread_mutex_unlock(&taskQueue->lock);
}

Task *dequeueTaskQueue(TaskQueue *taskQueue)
{
    pthread_mutex_lock(&taskQueue->lock);
    if (taskQueue->start == taskQueue->end)
    {
        pthread_mutex_unlock(&taskQueue->lock);
        return NULL;
    }
    Task *task = taskQueue->tasks[taskQueue->start];
    taskQueue->start = (taskQueue->start + 1) % taskQueue->size;
    pthread_mutex_unlock(&taskQueue->lock);
    return task;
}

int isTaskQueueEmpty(TaskQueue *taskQueue)
{
    return taskQueue->start == taskQueue->end;
}

void createThreadPool(ThreadPool *threadPool, TaskQueue *taskQueue, int (*priority)(void *), int numThreads, int queueSize)
{
    threadPool->taskQueue = taskQueue;
    threadPool->numThreads = numThreads;
    threadPool->open = 1;
    sem_init(&threadPool->semaphore, 0, 0);
    threadPool->workerThreads = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);
    for (size_t i = 0; i < numThreads; i++)
        pthread_create(&threadPool->workerThreads[i], NULL, worker, threadPool);
}

void addTaskThreadPool(ThreadPool *threadPool, void *(*function_worker)(void *), void *args)
{
    Task *task = (Task *)malloc(sizeof(Task));
    task->worker = function_worker;
    task->args = args;
    enqueueTaskQueue(threadPool->taskQueue, task);
    sem_post(&threadPool->semaphore);
}

void *worker(void *args)
{
    ThreadPool *threadPool = (ThreadPool *)args;
    while (threadPool->open)
    {
        sem_wait(&threadPool->semaphore);
        Task *task = dequeueTaskQueue(threadPool->taskQueue);
        if (task != NULL)
            task->worker(task->args);
    }
}

void waitThreadPool(ThreadPool *threadPool)
{
    while (!isTaskQueueEmpty(threadPool->taskQueue))
        ;
    for (size_t i = 0; i < threadPool->numThreads; i++)
        sem_post(&threadPool->semaphore);
    threadPool->open = 0;
    for (size_t i = 0; i < threadPool->numThreads; i++)
        pthread_join(threadPool->workerThreads[i], NULL);
}

// WebServer ////////////////////////////////////////////////////////////////////

typedef struct
{
    char *path;
    void *(*handler_function)(void *);
} Handler;

typedef struct
{
    int port;
    int num_threads;
    int queue_size;
    Handler *handlers;
    int num_handlers;
    int open;
} WebServer;

void create_web_server(WebServer *webServer, int port, int num_threads, int queue_size, int max_num_handlers);

void add_new_handler(WebServer *webServer, char *path, void *(*handler_function)(void *));

void create_web_server(WebServer *webServer, int port, int num_threads, int queue_size, int max_num_handlers)
{
    webServer->port = port;
    webServer->num_threads = num_threads;
    webServer->queue_size = queue_size;
    webServer->handlers = (Handler *)malloc(sizeof(Handler) * max_num_handlers);
    webServer->num_handlers = 0;
    webServer->open = 1;
}

void add_new_handler(WebServer *webServer, char *path, void *(*handler_function)(void *))
{
    Handler *handler = &webServer->handlers[webServer->num_handlers++];
    handler->path = path;
    handler->handler_function = handler_function;
}

int main() {
    char buffer[BUFFER_SIZE];
    char resp[] = "HTTP/1.0 200 OK\r\n"
                  "Server: webserver-c\r\n"
                  "Content-type: text/html\r\n\r\n"
                  "<html>hello, world</html>\r\n";

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("webserver (socket)");
        return 1;
    }
    printf("socket created successfully\n");

    // Create the address to bind the socket to
    struct sockaddr_in host_addr;
    int host_addrlen = sizeof(host_addr);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(PORT);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
        perror("webserver (bind)");
        return 1;
    }
    printf("socket successfully bound to address\n");

    // Listen for incoming connections
    if (listen(sockfd, SOMAXCONN) != 0) {
        perror("webserver (listen)");
        return 1;
    }
    printf("server listening for connections\n");

    for (;;) {
        // Accept incoming connections
        int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr,
                               (socklen_t *)&host_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");

        // Read from the socket
        int valread = read(newsockfd, buffer, BUFFER_SIZE);
        if (valread < 0) {
            perror("webserver (read)");
            continue;
        }

        // Write to the socket
        int valwrite = write(newsockfd, resp, strlen(resp));
        if (valwrite < 0) {
            perror("webserver (write)");
            continue;
        }

        close(newsockfd);
    }

    return 0;
}
// typedef struct
// {
//     int m;
//     int n;
// } AckermannArg;

// int ackermann(int m, int n)
// {
//     if (m == 0)
//         return n + 1;
//     else if (n == 0)
//         return ackermann(m - 1, 1);
//     else
//         return ackermann(m - 1, ackermann(m, n - 1));
// }

// int visited[4][10] = {0};

// void *ackermannWorker(void *args)
// {
//     AckermannArg *ackermannArgs = (AckermannArg *)args;
//     int result = ackermann(ackermannArgs->m, ackermannArgs->n);
//     printf("A(%d, %d) = %d\n", ackermannArgs->m, ackermannArgs->n, result);
//     visited[ackermannArgs->m][ackermannArgs->n] = 1;
//     return NULL;
// }

// int main() {
//     ThreadPool *threadPool;
//     threadPool = (ThreadPool *)malloc(sizeof(ThreadPool));
//     TaskQueue *taskQueue;
//     taskQueue = (TaskQueue *)malloc(sizeof(TaskQueue));
//     createTaskQueue(taskQueue, 100);
//     createThreadPool(threadPool, taskQueue, NULL, 10, 100);
//     for (int i = 0; i < 4; i++)
//         for (int j = 0; j < 10; j++)
//         {
//             AckermannArg *ackermannArgs = (AckermannArg *)malloc(sizeof(AckermannArg));
//             ackermannArgs->m = i;
//             ackermannArgs->n = j;
//             addTaskThreadPool(threadPool, ackermannWorker, ackermannArgs);
//         }
//     waitThreadPool(threadPool);
//     return 0;
// }

// int main()
// {
//     char *example_request = "GET /hello?name=aryan&last=ahadinia HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\nHello World";
//     HttpRequest *http_request = malloc(sizeof(HttpRequest));
//     parse_request(example_request, http_request);
//     printf("method: %s\n", http_request->method);
//     printf("path: %s\n", http_request->path);
//     printf("version: %s\n", http_request->version);
//     printf("path_without_query: %s\n", http_request->path_without_query);
//     printf("num_params: %d\n", http_request->query_parameters->num_params);
//     for (int i = 0; i < http_request->query_parameters->num_params; i++)
//     {
//         printf("key: %s, value: %s\n", http_request->query_parameters->keys[i], http_request->query_parameters->values[i]);
//     }
//     printf("num_headers: %d\n", http_request->headers->num_headers);
//     for (int i = 0; i < http_request->headers->num_headers; i++)
//     {
//         printf("key: %s, value: %s\n", http_request->headers->keys[i], http_request->headers->values[i]);
//     }
//     printf("body: %s\n", http_request->body);

//     printf("\n\n");

//     HttpResponse *http_response = response(200, "OK", "Hello World");
//     add_header(http_response, "Content-Type", "text/plain");
//     char *response_string = serialize_response(http_response);
//     printf("%s\n", response_string);

//     return 0;
// }
