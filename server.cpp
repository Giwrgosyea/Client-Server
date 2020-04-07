#include <stdio.h>
#include<iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include<pthread.h>
#include<stdlib.h>
#include <string.h>
#include<dirent.h> //opendir ktl ktl
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
//--------------------------------------------
#define POOL_SIZE 6
#define MSGSIZE 256
#define work_it 10

//--------------------------------------------
using namespace std;
//long sz = sysconf (_SC_PAGESIZE);


void exit_handler(int signum)
{
   cout << "Time to exit server...." << endl;
   /* for (int i = 0; i < 2; i++) {
    pthread_join(&workers[i], NULL);
   } */

   exit(0);

}


typedef struct str_thdata
{
    char message[MSGSIZE + 1];
    int socket_fd;
    pthread_mutex_t* mutex;
    int * file_counter;
} thdata;

typedef struct savefile
{
    //char array1[POOL_SIZE][100];
    char array1[MSGSIZE + 1];
    int socket_fd;
    pthread_mutex_t* mutex;
    int * file_counter;
} savefile;

typedef struct
{
    // savefile file;
    savefile *file;
    int startpool;
    int endpool;
    int counter;
} pool_t;

//------------------------------------------
pthread_mutex_t mtx; // mutex gia ton producer & consumer
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
pool_t pool;
//------------------------------------------
pthread_mutex_t mtxback;

int write_all ( int fd, char* buff , size_t size )
{
    size_t sent , n ;
    for ( sent = 0; sent < size ; sent += n )
    {
        if (( n = write ( fd , buff + sent , size - sent ) ) == -1)
            return -1;
        if (n==0) {
            puts("======= zerp    ======");
            break;
        }
    }
    return sent ;
}


void initialize(pool_t *pool)
{
    for (int i = 0; i < POOL_SIZE; i++)
        for (int j = 0; j < 100; j++)
            pool->file[i].array1[j] = '\0';

    pool->startpool = 0;
    pool -> endpool = -1;
    pool -> counter = 0;
    cout << "------->pool initialized " << endl;
}

void place(pool_t * pool, char *data, int fd, int* fc, pthread_mutex_t* g)
{

    pthread_mutex_lock(&mtx);

    while (pool->counter >= POOL_SIZE)
    {
        cout << " Thread " << pthread_self() << "---->Found Buffer Full.." << endl;

        pthread_cond_wait(&cond_nonfull, &mtx); // when you unblock you are gonna check again
    }

    pool->endpool = (pool->endpool + 1) % POOL_SIZE;
    //strcpy(pool->file.array1[pool->endpool],data);
    strcpy(pool->file[pool->endpool].array1, data);
    pool->file[pool->endpool].socket_fd = fd;
    pool->file[pool->endpool].mutex = g;
    pool->file[pool->endpool].file_counter = fc;
    pool->counter++;


    pthread_mutex_unlock(&mtx);
}

savefile obtain(pool_t * pool)
{
    savefile data2;

    pthread_mutex_lock(&mtx);
    while (pool->counter <= 0)
    {
        cout << " Thread " << pthread_self() << "---->Found Buffer Empty.." << endl;
        pthread_cond_wait(&cond_nonempty, &mtx);
    }

    strcpy(data2.array1, pool->file[pool->startpool].array1);
    data2.socket_fd = pool->file[pool->startpool].socket_fd;
    data2.mutex = pool->file[pool->startpool].mutex;
    data2.file_counter = pool->file[pool->startpool].file_counter;
    pool->startpool = (pool->startpool + 1) % POOL_SIZE;
    pool->counter--;


    pthread_mutex_unlock(&mtx);

    return data2;
}

// worker threads ...


int count_file(char *path, int depth, int fd) // needs fix
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char tempname[2000];
    int count = 0;

    if ((dp = opendir(path)) == NULL)
    {
        cout << "cant open directory " << path << endl;
        return 0;
    }

    while ((entry = readdir(dp)) != NULL) // mexri na min vreis allo arxeop
    {
        strcpy(tempname, path);
        strcat(tempname, "/");
        strcat(tempname, entry->d_name);

        lstat(tempname, &statbuf);

        if (S_ISDIR(statbuf.st_mode))
        {
            /* Found a directory, but ignore . and .. */
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            printf("%*s%s/\n", depth, "", entry->d_name);
            /* Recurse at a new indent level */

            count = count  +count_file(tempname, depth + 4, fd);
        }
        else
        {
            count ++;
            //printf("%*s%s\n",depth,"",entry->d_name);
        }

    }

    closedir(dp);

    return count;
}

void search_file(char *path, int depth, int fd, pthread_mutex_t* m, int* fc, int n) // needs fix
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char tempname[2000];
    char localname[2000];


    if ((dp = opendir(path)) == NULL)
    {
        cout << "cant open directory " << path << endl;
        return;
    }

    while ((entry = readdir(dp)) != NULL) // mexri na min vreis allo arxeop
    {
        strcpy(tempname, path);
        strcat(tempname, "/");
        strcat(tempname, entry->d_name);

        lstat(tempname, &statbuf);

        if (S_ISDIR(statbuf.st_mode))
        {
            /* Found a directory, but ignore . and .. */
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            printf("%*s%s/\n", depth, "", entry->d_name);
            /* Recurse at a new indent level */

            search_file(tempname, depth + 4, fd, m, fc, n);
        }
        else
        {
            //printf("%*s%s\n",depth,"",entry->d_name);
            strcpy(localname, path);
            strcat(localname, "/");
            strcat(localname, entry->d_name);


           // cout << "kgsdjgjsdlg" << (*fc) << endl;

            place(&pool, localname, fd, fc, m);
            cout << "producer" << pthread_self() << " placed: " << localname << endl;
            pthread_cond_signal(&cond_nonempty);
        }

    }

    closedir(dp);
}
// search arxeion

void* child_server_search(void* a)
{
    //anadromi gia na psa3ei ston folder
    thdata *data1;
    int n;
    char original_path[2000];
    char msgbuf[MSGSIZE + 1];


    data1 = (thdata *) a;
    data1->mutex = (pthread_mutex_t*) malloc(sizeof (pthread_mutex_t));
    data1->file_counter = (int*) malloc(sizeof(int));
    *(data1->file_counter) = 0;
    pthread_mutex_init(data1->mutex, 0);

    if (read(data1->socket_fd, msgbuf, MSGSIZE + 1) < 0)
    {
        perror("problem in reading msgbuf");
        exit(5);
    }
    strcpy(data1->message, msgbuf);
    cout << data1->message << endl;


    //*(data1->file_counter) = 9;

    cout << " Thread " << pthread_self() << "  executing request for file  " << data1->message << endl;
    // psa3e anadromika to dentro !
    strcpy(original_path, get_current_dir_name());
    n = strlen(original_path) + 1;

     *(data1->file_counter) = count_file(data1->message, 0, data1->socket_fd);

    search_file(data1->message, 0, data1->socket_fd, data1->mutex, data1->file_counter, n);
    chdir(original_path);
    // pthread_mutex_destroy(data1->mutex);

    free(data1);

    pthread_exit(0);
}

void * child_server_worker(void * ptr)
{
    savefile sf;
    FILE* dp;
    struct stat s;
    char *buf;
    int shutdown = 0;
     long sz = sysconf(_SC_PAGESIZE);
     int sent;
     int n;

    cout << "Thread " << pthread_self() << " is a worker with sz =  " << sz << endl;
    while (1)
    {
        sf = obtain(&pool);
        cout << "consumer" << pthread_self() << " obtained : " << sf.array1 << " and fd:" << sf.socket_fd << endl;

        pthread_cond_signal(&cond_nonfull);

        pthread_mutex_lock(sf.mutex); //lock

         shutdown = 0;
        // open to file: sf.array1              (fd = open() ....)

        if ((dp = fopen(sf.array1, "rb")) == NULL)
        {
            cout << "cant open directory " << sf.array1 << endl;
            exit(5);
        }
        cout << "Thread : " << pthread_self() << "about to send..." << endl;


        write_all(sf.socket_fd, sf.array1, MSGSIZE + 1);

        stat(sf.array1, &s);
       // cout << "=====" << s.st_size << endl;

        write_all(sf.socket_fd, (char*)&s, sizeof (struct stat));

        buf = (char *) malloc(sizeof (char)*sz);
        /*fread(buf, 1, s.st_size, dp);
        fclose(dp); */

       // cout << "----->" << buf << endl;
       int fd=open(sf.array1,O_RDONLY);
        sent = 0;
        while (sent < s.st_size ) {
            if (s.st_size - sent >= sz) {

                read(fd,buf,sz);


                n = write_all(sf.socket_fd, buf, sz);



            } else {
                read(fd,buf,s.st_size-sent);

                n = write_all(sf.socket_fd, buf, s.st_size - sent);

            }
            sent = sent +n;
        }
         close(fd);
        // write_all(sf.socket_fd, buf,  s.st_size );

        (*(sf.file_counter))--;


       //cout << "---->" << (*(sf.file_counter)) << endl;

        if (*(sf.file_counter) == 0)
        {
            shutdown = 1;
        }

        pthread_mutex_unlock(sf.mutex); // unlock
        //close(sf.socket_fd); ################## must close socket
        usleep(500000);

        if (shutdown == 1)
        {
            close(sf.socket_fd);
            pthread_mutex_destroy(sf.mutex);
            free(sf.mutex);
            cout << "client is terminated..." << endl;
        }
    }

    pthread_exit(0);
}


int main(int argc, char *argv[])
{
    int port, sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen = sizeof (client);
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct sockaddr *clientptr = (struct sockaddr *) &client;
    //char buf[1];
    pool.file=(savefile*)malloc(atoi(argv[6])* sizeof(savefile));
    pthread_t search_thr;
    thdata* data; //domi gia to message
    pthread_t *workers;
    int y = 1;
    //pthread_t workers;
    initialize(&pool);
    pthread_mutex_init(& mtx, 0);
    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&cond_nonfull, 0);
   signal(SIGINT,exit_handler);
    workers = new pthread_t [atoi(argv[4])];
    for (int i = 0; i < atoi(argv[4]); i++)
        // pthread create ... workers
        pthread_create(workers + i, NULL, child_server_worker, (void *) 0);
    //pthread_create (&workers ,NULL , child_server_worker ,(void * ) 0 );

    port = atoi(argv[2]);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof (int)) == -1)
    {
        perror("setsockopt");
    }

    if (bind(sock, serverptr, sizeof (server)) < 0)
    {
        perror("bind");
        exit(1);
    }

    if (listen(sock, 20) < 0)
        perror("listen");


    while (1)
    {
        printf("Listening for connections to port %d\n", port);
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror("accept");

        data = (thdata*) malloc(sizeof (thdata));
        data->socket_fd = newsock;

        pthread_create(&search_thr, NULL, child_server_search, (void *) data);
        cout << "I am original thread " << pthread_self() << " and I created thread  " << search_thr << endl;

    }

    pthread_join(search_thr, NULL);

    pthread_cond_destroy(& cond_nonempty);
    pthread_cond_destroy(& cond_nonfull);
    pthread_mutex_destroy(& mtx);

    close(sock);




    return 0;
}

