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

char *wd;
double time_diff;
int first_sync;
time_t c_time; //Used as current time during execution

void update_local_history(char *dir);
void work_as_server();
void work_as_client(char *address);

void send_file_list(void *socket);
struct ArrayList *receive_file_list(void *socket);
void update_local_dir(void *socket, struct ArrayList *changes);
void serve_files(void *socket);
void request_and_save_file(void *socket, char *filename);
void send_requested_file(void *socket, char *filename);

void print_diff_list(struct ArrayList *diffs);

char *msg_recv(void *socket, int flag);
int msg_send(void *socket, char *msg);
int msg_sendmore(void *socket, char *msg);

enum Command
{
    start,
    send,
    stop
};

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("\n[Error] Please include the directory to sync.\n\n");
        return 1;
    }

    //initialize time difference in 0
    time_diff = 0;

    //set local time
    c_time = time(NULL);

    //clean working path
    wd = clean_path(argv[1]);

    printf("\n====================================\n");
    printf(" ==> Updating local history...");
    printf("\n------------------------------------\n\n");

    update_local_history(wd);

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

    free(wd);

    return 0;
}

void update_local_history(char *dir)
{
    //first syncronization
    first_sync = make_dir();

    printf("--> Loading directory history\n");

    history = load_data();

    printf("\n--> Reading files in: %s\n\n", dir);

    files = list_dir(dir);

    changes = find_differences(history, files, time_diff);
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

    //client first sync
    int c_first_sync;

    time_t client_time;

    char *request;
    request = msg_recv(responder, 0);
    sscanf(request, "%d %lu", &c_first_sync, &client_time);
    free(request);

    printf("--> Client first sync: %s\n\n", c_first_sync ? "yes" : "no");

    time_diff = difftime(c_time, client_time);

    printf("--> Current time: %s\n\n", ctime(&c_time));
    printf("--> Client's time: %s\n\n", ctime(&client_time));
    printf("--> Time difference: %.f seconds\n\n", time_diff);

    char *buffer = malloc(sizeof(char) * 20);
    sprintf(buffer, "%d %lu", first_sync, (unsigned long)c_time);
    msg_send(responder, buffer);

    free(buffer);

    printf("\n====================================\n");
    printf(" ==> Receiving list of files from client...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *client_list = receive_file_list(responder);

    struct ArrayList *diffs = NULL;

    if (arraylist_is_empty(client_list))
    {
        if (c_first_sync)
            diffs = create_arraylist();
        else
            diffs = fill_with_changes_by_type(deleted, files);
    }
    else if (arraylist_is_empty(files))
        diffs = fill_with_changes_by_type(created, client_list);
    else
        diffs = find_differences(files, client_list, time_diff);

    print_diff_list(diffs);

    printf("\n====================================\n");
    printf(" ==> Requesting files to client...");
    printf("\n------------------------------------\n\n");

    update_local_dir(responder, diffs);

    printf("\n====================================\n");
    printf(" ==> Updating local history...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *newfiles = list_dir(wd);

    save_data(newfiles);

    printf("--> Local history updated\n");

    printf("\n====================================\n");
    printf(" ==> Sending list of files to the client...");
    printf("\n------------------------------------\n\n");

    send_file_list(responder);

    printf("--> List of files sent\n");

    printf("\n====================================\n");
    printf(" ==> Waiting for client to request files...");
    printf("\n------------------------------------\n\n");

    serve_files(responder);

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

    printf("--> Notifing server of first sync\n\n");
    printf("--> Sending current time to server %s\n", ctime(&c_time));

    char *buffer = malloc(sizeof(char) * 20);

    sprintf(buffer, "%d %lu", first_sync, (unsigned long)c_time);
    msg_send(requester, buffer);

    free(buffer);

    char *request = msg_recv(requester, 0);

    //client first sync
    int s_first_sync;

    time_t server_time;

    sscanf(request, "%d %lu", &s_first_sync, &server_time);
    free(request);

    time_diff = difftime(c_time, server_time);

    printf("--> Server's current time %s\n", ctime(&server_time));
    printf("--> Time difference: %.f seconds\n\n", time_diff);

    printf("\n====================================\n");
    printf(" ==> Sending list of files to the server...");
    printf("\n------------------------------------\n\n");

    send_file_list(requester);

    printf("--> List of files sent\n");

    printf("\n====================================\n");
    printf(" ==> Waiting for server to request files...");
    printf("\n------------------------------------\n\n");

    serve_files(requester);

    printf("\n====================================\n");
    printf(" ==> Receiving list of files from server...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *server_list = receive_file_list(requester);

    struct ArrayList *diffs = NULL;

    if (arraylist_is_empty(server_list))
    {
        if (s_first_sync)
            diffs = create_arraylist();
        else
            diffs = fill_with_changes_by_type(deleted, files);
    }
    else if (arraylist_is_empty(files))
        diffs = fill_with_changes_by_type(created, server_list);
    else
        diffs = find_differences(files, server_list, time_diff);

    print_diff_list(diffs);

    printf("\n====================================\n");
    printf(" ==> Requesting files to client...");
    printf("\n------------------------------------\n\n");

    update_local_dir(requester, diffs);

    printf("\n====================================\n");
    printf(" ==> Updating local history...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *newfiles = list_dir(wd);

    save_data(newfiles);

    printf("--> Local history updated\n");

    zmq_close(requester);
    zmq_ctx_destroy(context);
}

void send_file_list(void *socket)
{
    size_t len = arraylist_size(files);

    if (len == 0)
        msg_send(socket, "0");
    else
    {
        char *lenbuf = malloc(sizeof(char) * sizeof(len));
        sprintf(lenbuf, "%zu", len);
        msg_sendmore(socket, lenbuf);
    }

    int i;
    for (i = 0; i < arraylist_size(files); i++)
    {
        struct File *f = (struct File *)arraylist_get(files, i);

        char *buffer = malloc(strlen(f->name) + sizeof(f->m_time) + sizeof(f->size));
        sprintf(buffer, "%s %zu %lu", f->name, f->size, (unsigned long)(f->m_time));

        if (i < arraylist_size(files) - 1)
            msg_sendmore(socket, buffer);
        else
            msg_send(socket, buffer);
    }
}

struct ArrayList *receive_file_list(void *socket)
{
    int more;
    size_t more_size = sizeof(more);
    char *data;

    struct ArrayList *fl = create_arraylist();

    printf("Receiving lisf of files\n");
    data = msg_recv(socket, 0);

    int lenght;
    sscanf(data, "%d", &lenght);

    zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);

    if (lenght > 0)
    {
        while (more)
        {
            data = msg_recv(socket, 0);

            struct File *f = malloc(sizeof(struct File));

            char *buf = malloc(sizeof(char) * 50);
            time_t fm_date;

            sscanf(data, "%s %zu %lu", buf, &(f->size), &fm_date);

            f->m_time = fm_date + time_diff;
            f->name = strdup(buf);
            free(buf);

            arraylist_add(&fl, f);

            zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
        }
    }
    free(data);

    return fl;
}

void serve_files(void *socket)
{
    enum Command cmd;

    char *buf = msg_recv(socket, 0);
    sscanf(buf, "%d", (int *)&cmd);

    if (cmd == start)
    {
        do
        {
            char filename[50];

            buf = msg_recv(socket, 0);
            sscanf(buf, "%d %s", (int *)&cmd, filename);

            if (cmd == stop)
                break;

            printf(" -> Attempting to send %s --> ", filename);

            send_requested_file(socket, filename);

            printf("Sent\n\n");

        } while (cmd == send);
    }
    msg_send(socket, NULL);
}

void update_local_dir(void *socket, struct ArrayList *changes)
{
    if (!arraylist_is_empty(changes))
    {
        char *command = malloc(sizeof(char));

        sprintf(command, "%d", start);
        msg_sendmore(socket, command);
        free(command);

        int i;
        for (i = 0; i < arraylist_size(changes); i++)
        {
            struct Change *ch = (struct Change *)arraylist_get(changes, i);

            printf(" -> Attempting to sync %s --> ", ch->file->name);

            if (ch->type == created || ch->type == modified)
                request_and_save_file(socket, ch->file->name);
            //delete file if ch->type == deleted

            printf(" Syncronized\n\n");
        }

        command = malloc(sizeof(char));
        sprintf(command, "%d", stop);
        msg_send(socket, command);
        free(command);

        //closing exchange
        msg_recv(socket, 0);
    }
}

void request_and_save_file(void *socket, char *filename)
{
    FILE *df;

    char *full_path = string_concat(wd, filename);

    df = fopen(full_path, "w");

    if (df == NULL)
    {
        printf("[Error]: Error opening %s for writing...\n", full_path);
        return;
    }

    char *command = malloc(sizeof(char) * strlen(filename) + 2);
    sprintf(command, "%d %s", send, filename);
    msg_send(socket, command);

    int more;
    size_t more_size = sizeof(more);
    char *data;

    do
    {
        data = msg_recv(socket, 0);

        fprintf(df, "%s", data);

        zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
    } while (more);

    fclose(df);
}

void send_requested_file(void *socket, char *filename)
{
    int buffer_size = 555; //move to constant
    char *buffer = malloc(sizeof(char) * buffer_size);

    FILE *fp;

    char *full_path = string_concat(wd, filename);

    fp = fopen(full_path, "r");
    if (fp == NULL)
    {
        printf("[Error]: Error opening %s for reading...\n", full_path);
        return;
    }

    while (fgets(buffer, buffer_size, fp) != NULL)
    {
        msg_sendmore(socket, buffer);
    }

    msg_send(socket, NULL);
    fclose(fp);
}

void print_diff_list(struct ArrayList *diffs)
{
    if (!arraylist_is_empty(diffs))
    {
        int i;
        for (i = 0; i < arraylist_size(diffs); i++)
        {
            struct Change *change = (struct Change *)arraylist_get(diffs, i);
            printf(" -> File named %s was %s\n", change->file->name, change->type == 0 ? "created" : change->type == 1 ? "modified" : "deleted");
        }
    }
}

char *msg_recv(void *socket, int flag)
{
    char buffer[555];
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

int msg_sendmore(void *socket, char *msg)
{
    return zmq_send(socket, msg, strlen(msg), ZMQ_SNDMORE);
}