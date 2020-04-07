#include <stdio.h>
#include <iostream>
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <unistd.h>          /* read, write, close */
#include <netdb.h>	         /* gethostbyaddr */
#include <stdlib.h>	         /* exit */
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include<fcntl.h>
#include <errno.h>
#define MSGSIZE 256

using namespace std;

//const char *t = "/home/yea/Desktop/tests";
int read_all(int fd,char *buf,size_t size)
{
    size_t rec , n ;
    for ( rec = 0; rec < size ; rec += n )
    {
        if (( n = read(fd, buf+rec, size-rec) ) == -1)
            return -1;
    }
    return rec ;

}

// file "home/a/b/c/data.txt"
void filecopy(char *file) {
    FILE *to;
      int n=0;
    char *yea;
    char str[128];
    char temp[4000];
    char * temp2, temp3[4000];
    size_t k=0;

    if (strlen(file )==0) {
        return;
    }

    temp[0] = '\0';
    temp3[0] = '\0';
    strcpy(str,file);

    yea=strtok(str,"/");

    while(yea != NULL)
    {
        cout << yea << endl;
        n=strlen(yea);
        strcat(temp,yea);
        strcat(temp,"/");
        yea=strtok(NULL,"/");
    }

    strcpy(str,temp);
    cout << str << endl;

    for(k=strlen(str)-n-1; k<strlen(str); k++)
        str[k]='\0';
   cout << str << endl;
   temp2=strtok(str,"/");
   while(temp2 != NULL)
   {
      cout << temp2 << endl;
      strcat(temp3,temp2);
      strcat(temp3,"/");
      mkdir(temp3,0755);
      temp2=strtok(NULL,"/");
   }
   //temp3 = home/a/b/c/

    /*to = open(file, "w+"); // source


    fwrite(buffer, sizeof (char), st.st_size, to);


   chmod(file, st.st_mode);
   fclose(to);
   k=0; */
     cout << "ok" << endl;
}

int main(int argc, char *argv[]) {
    int port, sock, nwrite;
    char msgbuf[MSGSIZE + 1] = {0};
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*) &server;
    char host[200];
    struct hostent *rem;
    struct stat s;
    char *buffer;
    struct stat statbuf;
     long sz = sysconf(_SC_PAGESIZE);


    strcpy(host, argv[2]);

    /* Create socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket");

    if ((rem = gethostbyname(host)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }


    port = atoi(argv[4]); /*Convert port number to integer*/
    server.sin_family = AF_INET; /* Internet domain */
    server.sin_port = htons(port); /* Server port */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);


    /* Initiate connection */
    if (connect(sock, serverptr, sizeof (server)) < 0) {
        perror("connect");
        exit(0);
    }

    printf("Connecting to %s port %d\n", host, port);




    strcpy(msgbuf, argv[6]);
    if ((nwrite = write(sock, msgbuf, MSGSIZE + 1)) == -1) {
        perror("Error in Writing");
        exit(2);
    }

    cout << "sending.." << msgbuf << endl;
    int received=0;
    int n=0;
    while (read(sock, msgbuf, MSGSIZE + 1) > 0) {
        cout << msgbuf;

        read_all(sock,(char*)&s, sizeof (struct stat));

        buffer = (char *) malloc(sizeof(char)*(sz));

       // if (read_all(sock, buffer, s.st_size) > 0) {
       //     cout << " " << (int) s.st_size << endl;

     //   }
          filecopy(msgbuf);


           if (stat(msgbuf,&statbuf) < 0){
            if (errno != ENOENT){ perror("stat"); exit(11); } }
             else // if it already exists
                remove(msgbuf);
         int fd=open(msgbuf,O_CREAT|O_WRONLY|O_APPEND,0755);
          received=0;
          while (received < s.st_size ) {
            if (s.st_size - received >= sz) {

                n = read_all(sock, buffer, sz);

                if(write(fd,buffer,sz)<0)
                  perror("write");

            } else {
                n = read_all(sock, buffer, s.st_size - received);


                write(fd,buffer,s.st_size-received);

            }

            received = received +n;
        }
        //cout << "pire bytes :" << msgbuf << received << endl;

        close(fd);
       // buffer[s.st_size] = '\0';
        //cout << buffer << endl;



        free(buffer);
    }

    close(sock); /* Close socket and exit */

    return 0;
}

