#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <zmq.h>
#include <assert.h>
#include "arraylist.h"
#include "filehistory.h"
#include "utils.h"

struct ArrayList *history;
struct ArrayList *files;
struct ArrayList *changes;

double time_diff;

void update_local_history(char *dir);
void work_as_server();
void work_as_client(char *address);

char *msg_recv(void *socket, int flag);
int msg_send(void *socket, char *msg);

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("\n[Error] Please include the directory to sync.\n\n");
        return 1;
    }

    printf("\n====================================\n");
    printf(" ==> Updating local history...");
    printf("\n------------------------------------\n\n");

    update_local_history(argv[1]);

    if (argc == 2)
    {
        printf("\n====================================\n");
        printf(" ==> Starting as server...");
        printf("\n------------------------------------\n\n");

        work_as_server();
    }
    else
    {
        printf("\n====================================\n");
        printf(" ==> Starting as client...");
        printf("\n------------------------------------\n\n");

        work_as_client(argv[2]);
    }

    return 0;
}

void update_local_history(char *dir)
{
    open_dir();

    printf("--> Loading directory history\n");

    history = load_data();

    printf("\n--> Reading files in: %s\n\n", dir);

    files = list_dir(dir);

    changes = find_differences(history, files);
    int i;
    for (i = 0; i < arraylist_size(changes); i++)
    {
        struct Change *change = (struct Change *)arraylist_get(changes, i);
        printf("\n -> File named %s was %s\n", change->file->name, change->type == 0 ? "created" : change->type == 1 ? "modified" : "deleted");
    }

    printf("--> Saving directory history updated\n");
    save_data(files);
}

void work_as_server()
{
    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_REP);
    assert(responder);

    int r = zmq_bind(responder, "tcp://*:5555");
    assert(r == 0);

    printf("--> Server ready listening in port :5555\n");

    printf("\n====================================\n");
    printf(" ==> Waiting for client...");
    printf("\n------------------------------------\n\n");

    char *request;
    request = msg_recv(responder, 0);

    time_t c_time = time(NULL);

    time_t client_time;
    sscanf(request, "%lu", &client_time);
    free(request);

    time_diff = difftime(c_time, client_time);

    printf("--> Current time: %s\n\n", ctime(&c_time));
    printf("--> Client's time: %s\n\n", ctime(&client_time));
    printf("--> Time difference: %.f\n\n", time_diff);

    char buffer[20];
    sprintf(buffer, "%lu", (unsigned long)c_time);
    msg_send(responder, buffer);

    zmq_close(responder);
    zmq_ctx_destroy(context);
}

void work_as_client(char *address)
{
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    assert(requester);

    char *full_address = string_concat("tcp://", address);
    printf("--> Connecting to: %s\n\n", full_address);

    int r = zmq_connect(requester, "tcp://localhost:5555");
    assert(r == 0);

    time_t c_time = time(NULL);

    printf("--> Sending current time to server %s\n", ctime(&c_time));

    char buffer[20];
    sprintf(buffer, "%lu", (unsigned long)c_time);
    msg_send(requester, buffer);

    char *request = msg_recv(requester, 0);

    time_t server_time;

    sscanf(request, "%lu", &server_time);
    free(request);
    
    time_diff = difftime(c_time, server_time);

    printf("--> Server's current time %s\n", ctime(&server_time));
    printf("--> Time difference: %.f\n\n", time_diff);

    zmq_close(requester);
    zmq_ctx_destroy(context);
}

char *msg_recv(void *socket, int flag)
{
    char buffer[256];
    int size = zmq_recv(socket, buffer, 255, flag);
    if (size == -1)
        return NULL;
    buffer[size] = '\0';
    return strdup(buffer);
}

int msg_send(void *socket, char *msg)
{
    if (msg == NULL)
        return zmq_send(socket, NULL, 0, 0);
    return zmq_send(socket, msg, strlen(msg), 0);
}