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

struct ArrayList *remote_history;

char *wd;
int first_sync;
time_t c_time; //Used as current time during execution

void update_local_history(char *dir);
void work_as_server();
void work_as_client(char *address);

void send_file_list(void *socket);
struct ArrayList *receive_file_list(void *socket);
void update_local_dir(void *socket, struct ArrayList *changes);
void serve_files(void *socket);
void request_and_save_file(void *socket, char *filename, char *alt_name);
void send_requested_file(void *socket, char *filename);
int delete_file(char *filename);

struct ArrayList *calc_delta(struct ArrayList *local, struct ArrayList *remote);
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

    //clean working path
    wd = clean_path(argv[1]);

    printf("\n====================================\n");
    printf(" ==> Updating local history...");
    printf("\n------------------------------------\n\n");

    update_local_history(wd);

    printf("\n====================================\n");
    printf(" ==> Loading remote history...");
    printf("\n------------------------------------\n\n");

    remote_history = load_data(REMOTE_INDEX_NAME);

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

    history = load_data(LOCAL_INDEX_NAME);

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
    save_data(LOCAL_INDEX_NAME, files);
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

    char *request;
    request = msg_recv(responder, 0);
    sscanf(request, "%d", &c_first_sync);

    char *buffer = malloc(sizeof(char) * 20);
    sprintf(buffer, "%d", first_sync);
    msg_send(responder, buffer);
    free(buffer);

    printf("--> Client first sync: %s\n\n", c_first_sync ? "yes" : "no");

    //set local time
    c_time = time(NULL);

    printf("\n====================================\n");
    printf(" ==> Receiving list of files from client...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *client_list = receive_file_list(responder);

    printf("\n====================================\n");
    printf(" ==> Saving remote index...");
    printf("\n------------------------------------\n\n");

    save_data(REMOTE_INDEX_NAME, client_list);

    printf("\n====================================\n");
    printf(" ==> Comparing against previous remote index...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *remoteChanges = find_differences(remote_history, client_list);
    int i;
    for (i = 0; i < arraylist_size(remoteChanges); i++)
    {
        struct Change *change = (struct Change *)arraylist_get(remoteChanges, i);
        printf("\n -> File named %s was %s\n", change->file->name, change->type == 0 ? "created" : change->type == 1 ? "modified" : "deleted");
    }

    printf("\n====================================\n");
    printf(" ==> Calculating delta with local index...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *delta = calc_delta(changes, remoteChanges);

    if (!arraylist_is_empty(delta))
    {
        print_diff_list(delta);

        printf("\n====================================\n");
        printf(" ==> Requesting files to client...");
        printf("\n------------------------------------\n\n");

        update_local_dir(responder, delta);

        printf("\n====================================\n");
        printf(" ==> Updating local history...");
        printf("\n------------------------------------\n\n");

        struct ArrayList *newfiles = list_dir(wd);

        save_data(LOCAL_INDEX_NAME, newfiles);

        printf("--> Local history updated\n");
    }
    else
    {
        char *command = malloc(sizeof(char));
        sprintf(command, "%d", stop);
        msg_send(responder, command);
        free(command);

        //closing exchange
        msg_recv(responder, 0);

        printf("--> No remote changes to sync\n");
    }

    printf("\n====================================\n");
    printf(" ==> Sending list of files to the client...");
    printf("\n------------------------------------\n\n");

    send_file_list(responder);

    printf("--> List of files sent\n");

    printf("\n====================================\n");
    printf(" ==> Waiting for client to request files...");
    printf("\n------------------------------------\n\n");

    serve_files(responder);

    printf("--> All files sent\n");

    printf("\n====================================\n");
    printf(" ==> Saving local index...");
    printf("\n------------------------------------\n\n");

    printf("--> Reading files in: %s\n\n", wd);

    files = list_dir(wd);

    printf("--> Saving directory history updated\n");
    save_data(LOCAL_INDEX_NAME, files);

    printf("\n====================================\n");
    printf(" ==> Getting final state of client...");
    printf("\n------------------------------------\n\n");

    arraylist_destroy(client_list);
    client_list = receive_file_list(responder);

    printf("--> Received final state of client\n");

    printf("\n====================================\n");
    printf(" ==> Saving remote index...");
    printf("\n------------------------------------\n\n");

    save_data(REMOTE_INDEX_NAME, client_list);

    printf("--> Saved the remote index\n");

    printf("\n====================================\n");
    printf(" ==> Sending final state...");
    printf("\n------------------------------------\n\n");

    send_file_list(responder);

    printf("--> Final status of dir %s sent.\n", wd);

    printf("\n====================================\n");
    printf(" ==> Closing fileNsync...");
    printf("\n------------------------------------\n\n");

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

    char *buffer = malloc(sizeof(char) * 20);

    sprintf(buffer, "%d", first_sync);
    msg_send(requester, buffer);
    free(buffer);

    char *request = msg_recv(requester, 0);

    //server first sync
    int s_first_sync;

    sscanf(request, "%d", &s_first_sync);

    printf("--> Server first sync: %s\n\n", s_first_sync ? "yes" : "no");

    //set local time
    c_time = time(NULL);

    printf("\n====================================\n");
    printf(" ==> Sending list of files to the server...");
    printf("\n------------------------------------\n\n");

    send_file_list(requester);

    printf("--> List of files sent\n");

    printf("\n====================================\n");
    printf(" ==> Waiting for server to request files...");
    printf("\n------------------------------------\n\n");

    serve_files(requester);

    printf("--> Files served\n");

    printf("\n====================================\n");
    printf(" ==> Receiving list of files from server...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *server_list = receive_file_list(requester);

    printf("\n====================================\n");
    printf(" ==> Saving remote index...");
    printf("\n------------------------------------\n\n");

    save_data(REMOTE_INDEX_NAME, server_list);

    printf("\n====================================\n");
    printf(" ==> Comparing against previous remote index...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *remoteChanges = find_differences(remote_history, server_list);
    int i;
    for (i = 0; i < arraylist_size(remoteChanges); i++)
    {
        struct Change *change = (struct Change *)arraylist_get(remoteChanges, i);
        printf("\n -> File named %s was %s\n", change->file->name, change->type == 0 ? "created" : change->type == 1 ? "modified" : "deleted");
    }

    printf("\n====================================\n");
    printf(" ==> Calculating delta with local index...");
    printf("\n------------------------------------\n\n");

    struct ArrayList *delta = calc_delta(changes, remoteChanges);

    if (!arraylist_is_empty(delta))
    {
        print_diff_list(delta);

        printf("\n====================================\n");
        printf(" ==> Requesting files to server...");
        printf("\n------------------------------------\n\n");

        update_local_dir(requester, delta);

        printf("\n====================================\n");
        printf(" ==> Updating local history...");
        printf("\n------------------------------------\n\n");

        struct ArrayList *newfiles = list_dir(wd);

        save_data(LOCAL_INDEX_NAME, newfiles);

        printf("--> Local history updated\n");
    }
    else
    {
        char *command = malloc(sizeof(char));
        sprintf(command, "%d", stop);
        msg_send(requester, command);
        free(command);

        //closing exchange
        msg_recv(requester, 0);

        printf("--> No remote changes to sync\n");
    }

    printf("\n====================================\n");
    printf(" ==> Saving local index...");
    printf("\n------------------------------------\n\n");

    printf("--> Reading files in: %s\n\n", wd);

    files = list_dir(wd);

    printf("--> Saving directory history updated\n");
    save_data(LOCAL_INDEX_NAME, files);

    printf("\n====================================\n");
    printf(" ==> Sending final state...");
    printf("\n------------------------------------\n\n");

    send_file_list(requester);

    printf("--> Final status of dir %s sent.\n", wd);

    printf("\n====================================\n");
    printf(" ==> Getting final state of server...");
    printf("\n------------------------------------\n\n");

    arraylist_destroy(server_list);
    server_list = receive_file_list(requester);

    printf("--> Received final state of client\n");

    printf("\n====================================\n");
    printf(" ==> Saving remote index...");
    printf("\n------------------------------------\n\n");

    save_data(REMOTE_INDEX_NAME, server_list);

    printf("--> Saved the remote index\n");

    printf("\n====================================\n");
    printf(" ==> Closing fileNsync...");
    printf("\n------------------------------------\n\n");

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
        free(lenbuf);
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
    data = msg_recv(socket, 0);

    size_t lenght;
    sscanf(data, "%zu", &lenght);

    zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);

    if (lenght > 0)
    {
        while (more)
        {
            data = msg_recv(socket, 0);

            struct File *f = malloc(sizeof(struct File));

            char *buf = malloc(sizeof(char) * 50);

            sscanf(data, "%s %zu %lu", buf, &(f->size), &(f->m_time));
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

void update_local_dir(void *socket, struct ArrayList *lchanges)
{
    if (!arraylist_is_empty(lchanges))
    {
        char *command = malloc(sizeof(char));

        sprintf(command, "%d", start);
        msg_sendmore(socket, command);
        free(command);

        int i;
        for (i = 0; i < arraylist_size(lchanges); i++)
        {
            struct Change *ch = (struct Change *)arraylist_get(lchanges, i);

            if (ch->type == created || ch->type == modified)
            {
                printf(" -> Attempting to sync %s --> ", ch->file->name);

                if (ch->conflict == 1)
                {
                    char *alt_name = malloc(sizeof(char) * strlen(ch->file->name) + sizeof(c_time));
                    sprintf(alt_name, "%s-%lu", ch->file->name, (unsigned long)ctime);

                    request_and_save_file(socket, ch->file->name, alt_name);
                    printf(" Syncronized with conflict\n\n");
                }
                else
                {
                    request_and_save_file(socket, ch->file->name, NULL);
                    printf(" Syncronized\n\n");
                }
            }
            else if (ch->type == deleted)
            {
                printf(" -> Attempting to delete %s --> ", ch->file->name);

                delete_file(ch->file->name);

                printf(" Deleted\n\n");
            }
        }

        command = malloc(sizeof(char));
        sprintf(command, "%d", stop);
        msg_send(socket, command);
        free(command);

        //closing exchange
        msg_recv(socket, 0);
    }
}

void request_and_save_file(void *socket, char *filename, char *alt_name)
{
    FILE *df;

    char *full_path = string_concat(wd, alt_name == NULL ? filename : alt_name);

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

int delete_file(char *filename)
{
    char *full_path = string_concat(wd, filename);

    return remove(full_path);
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

struct ArrayList *calc_delta(struct ArrayList *local, struct ArrayList *remote)
{
    if ((arraylist_is_empty(local) && arraylist_is_empty(remote)) || (!arraylist_is_empty(local) && arraylist_is_empty(remote)))
        return NULL;

    struct ArrayList *delta = create_arraylist();

    int i;
    for (i = 0; i < arraylist_size(remote); i++)
    {
        struct Change *remoteChange = (struct Change *)arraylist_get(remote, i);

        if (remoteChange->type == created)
            arraylist_add(&delta, remoteChange);
        else if (remoteChange->type == deleted)
        {
            struct Change *localChange = (struct Change *)find_change_by_file_name(local, remoteChange->file->name);
            if (localChange == NULL)
                arraylist_add(&delta, remoteChange);
        }
        else
        {
            struct Change *localChange = (struct Change *)find_change_by_file_name(local, remoteChange->file->name);
            if (localChange != NULL && (localChange->type == modified))
            {
                struct Change *change_cpy = malloc(sizeof(struct Change));
                change_cpy->type = modified;
                change_cpy->conflict = 1;
                change_cpy->file = localChange->file;

                arraylist_add(&delta, change_cpy);
            }
        }
    }

    return delta;
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