#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/un.h>
#include "unboundedqueue.h"

#define SOCKET_PATH "/tmp/socketMattiaDaday"
#define BUFFER_SIZE 1024
#define MAX_PATH_LENGTH 1024

//mutex per la mutua esclusione
pthread_mutex_t mutex;

//struct usato da un thread per immagazzinare i dati di un singolo file
typedef struct{
    int num_count;
    double average;
    double deviaziones;
    char path[MAX_PATH_LENGTH];
}dati_file;

//struct usato per contenere i parametri da passare a un singolo thread
typedef struct{
    int worker_socket;
    Queue_t *qpath;
    int tid;
}structworker;


// Calcola la media dei numeri all'interno del file specificato dal percorso.
void printAv(char *filepath, dati_file *dati_file) {

    //accedo al file specificato dal filepath in input
    FILE* file = fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "Errore nell'apertura del file %s\n", filepath);
        return;
    }

    // Conta il numero di numeri all'interno del file
    int num_count = 0;
    double num_sum = 0.0;
    double num;
    double s_dev_pot=0.0;
    double deviaziones=0.0;
    double average=0.0;
    while(fscanf(file, "%lf", &num) == 1){
        num_count++;
        num_sum += num;
    }
    //se il file non contiene numeri al suo interno
    if(num_count == 0){
        average=0.0;
        //deviaziones=0.0;
        //printf("Il file %s è vuoto o non contiene numeri.\n", filepath);
    }else{
        average = num_sum / num_count;
        average = trunc(average*100)/100;
    }

    fclose(file);
    file=fopen(filepath,"r");

    //calcolo deviazione standard
    while(fscanf(file,"%lf", &num) == 1){
        //printf("num1: %lf",num);
        s_dev_pot=s_dev_pot+pow((num-average),2);
    }

    if(num_count == 0) deviaziones=0.0;
    else{
        deviaziones=sqrt(s_dev_pot/num_count);
        deviaziones = trunc(deviaziones*100)/100;
    }

    fclose(file);

    //una volta calcolato i vari dati da questo file si riempiono i campi della struct dati_file che poi sarà inviata via socket al server
    dati_file->num_count=num_count;
    dati_file->average=average;
    dati_file->deviaziones=deviaziones;
    strncpy(dati_file->path, filepath, MAX_PATH_LENGTH - 1);
    dati_file->path[MAX_PATH_LENGTH - 1] = '\0'; // Mi assicuro che la stringa termini correttamente con il carattere null
}


//procedura che esegue un worker generico
/*
casto gli argomenti arg
prelevo dalla coda un dato (un filepath)
invoco la funzione di calcolo sul singolo file modificando la struct dati_file
connessione via socket
invio della struct dati_file via socket
*/
static void* workergen(void *arg){
 
    //casto i parametri in input
    structworker *str=(structworker *)arg;
    int worker_socket=str->worker_socket;

    //istanzio la struct dati_file
    dati_file df; 
    memset(&df, 0, sizeof(dati_file));
    df.num_count=0;
    df.average=0.0;
    df.deviaziones=0.0;
    strcpy(df.path,"");   

    //se la coda non è vuota eseguo pop e calcolo i dati su quel filename prelevato
    while(length(str->qpath)>0){
        char *s1 = (char*)pop(str->qpath);
        printAv(s1,&df);
        
        //invio via socket della struttura dati_file in mutua esclusione
        pthread_mutex_lock(&mutex);
        if (write(worker_socket, &df, sizeof(dati_file)) == -1) {
            perror("Errore nell'invio della struttura dati");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(&mutex);
        
        //printf("worker n°%d %d\t\t%.2f\t\t%.2f\t\t%s\n",tid,df.num_count,df.average,df.deviaziones,df.path);
        free(s1);
    }
    // Chiudi il socket del worker
    //close(worker_socket);
    pthread_exit(0);
}

//visita ricorsiva a partira da un file e i file name vengono messi in coda q
void visitDirectory(const char* dirpath, Queue_t* q) {
    struct dirent *entry;
    DIR *dir = opendir(dirpath);
    if (!dir) {
        perror("opendir");
        return;
    }
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char path[MAX_PATH_LENGTH];
        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
        if (entry->d_type == DT_DIR) {
            visitDirectory(path, q);
        } else if (entry->d_type == DT_REG) {
            if (strstr(entry->d_name, ".dat")) {
                char* path_copy = strdup(path);
                if (path_copy) {
                    push(q, path_copy);
                } else {
                    fprintf(stderr, "Errore: Impossibile allocare memoria per il percorso\n");
                }
            
            }
        }
    
    }
    closedir(dir);
}




int main(int argc, char *argv[]){

    //controllo numero parametri
    if(argc!=3){ //errore numero parametri inseriti
        printf("# param sbagliati: Riprovare...\n");
        exit(EXIT_FAILURE);
    }
    
    int nworker=atoi(argv[2]);  //numero worker
    
    //creazione processo collector figlio (server)
    int pid;
    //inizializzazione della mutex 
    pthread_mutex_init(&mutex, NULL);

    
    if((pid=fork())!=0){ //fork
        if(pid==-1){
            perror("Errore fork");
            exit(EXIT_FAILURE);
        }
        //processo padre
        sleep(2);
        //visita con caricamento in coda dei nomi di file
        int l=strlen(argv[1]);               //printf("length %d\n",l);
        if(l>0 && argv[1][l-1]== '/'){argv[1][l-1]='\0';}
       
        //istanza coda per file 
        Queue_t *qpath=initQueue();
        //visito le directory a partire da path in argv[1]
        visitDirectory(argv[1],qpath);

  
        //connessione socket
        int worker_socket;
        if((worker_socket=socket(AF_UNIX,SOCK_STREAM,0))==-1){
            perror("Errore nella creazione del socket");
            exit(EXIT_FAILURE);
        }
        //Imposta i parametri dell'indirizzo del server
        struct sockaddr_un server_address;
        server_address.sun_family=AF_UNIX;
        strncpy(server_address.sun_path,SOCKET_PATH,sizeof(server_address.sun_path)-1);
        

        //connettiti al server
        if(connect(worker_socket,(struct sockaddr*)&server_address,sizeof(server_address))==-1){
            perror("Errore connessione socket");
            exit(EXIT_FAILURE);
        }

        //printf("Worker connesso\n");

       
        //creazione n thread worker
        pthread_t tid[nworker];
        structworker *sw=malloc(sizeof(structworker));
        sw->qpath=qpath;
        sw->worker_socket=worker_socket;
        
   
        for(int i=0;i<nworker;i++){
            sw->tid=i;
            if((pthread_create(&tid[i],NULL,workergen,sw))!=0){
                perror("Errore pthread create");
                exit(EXIT_FAILURE);
            }   
        }

        //join dei thread creati
        for(int i=0;i<nworker;i++){
            if((pthread_join(tid[i],NULL))!=0){
                perror("Errore pthread join");
                exit(EXIT_FAILURE); 
            }
        }

        //invio via socket la terminazione
        dati_file final;
        memset(&final, 0, sizeof(dati_file));
        final.num_count=0;
        final.average=0.0;
        final.deviaziones=0.0;

        //final.path="-";
        strcpy(final.path, "-");
        final.path[MAX_PATH_LENGTH - 1] = '\0'; 

        pthread_mutex_lock(&mutex);
        if (write(worker_socket, &final, sizeof(dati_file)) == -1) {
            perror("Errore nell'invio della struttura dati");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(&mutex);
        close(worker_socket);
        //dealloco path ecc..
        deleteQueue(qpath);
        free(sw);
        return 0;
    
    }else{  //processo figlio (server collector)
       
        
        
        printf("n \t\t avg \t\t dev \t\t path\n");
        printf("******************************************************************************************************\n");
        
        //creazione socket lato server 

        int server_socket,client_socket;
        struct sockaddr_un address;

        //rimuovo socket precedente se esiste
        unlink(SOCKET_PATH);

        //creo il socket del server
        if((server_socket=socket(AF_UNIX,SOCK_STREAM,0))==-1){
            perror("Errore creazione socket");
            exit(EXIT_FAILURE);
        }
        
        //impostiamo i parametri dell'inidirizzo
        address.sun_family=AF_UNIX;
        strncpy(address.sun_path,SOCKET_PATH,sizeof(address.sun_path)-1);

        //collego il socket all'indirizzo
        if(bind(server_socket,(struct sockaddr*)&address,sizeof(address))==-1){
            perror("Bind fallito");
            exit(EXIT_FAILURE);
        }
        //Metti il server in ascolto delle connessioni in entrata
        if(listen(server_socket,nworker)==-1){
            perror("Errore fase di ascolto");
            exit(EXIT_FAILURE);
        }
        
        //ricezione messaggi dai client
        while(1){
            //accetto una nuova connessione in entrata
            if((client_socket = accept(server_socket,NULL,NULL))==-1){
                perror("Errore accettazione connessione");
                exit(EXIT_FAILURE);
            }
            //printf("Nuova connessione accettata\n");

            //ricevo la struttura dati dal worker
            dati_file dati_ricevuti;
            while(1){
                //pthread_mutex_lock(&mutex);
                if(read(client_socket,&dati_ricevuti,sizeof(dati_file))<0){
                    perror("Errore nella lettura della struttura dati");
                    exit(EXIT_FAILURE);
                }
                //terminazione
                if (strcmp(dati_ricevuti.path, "-") == 0) {
                    printf("******************************************************************************************************\n");
                    break;
                }else{
                    
                    //stampa la struttura dati
                    char *s=dati_ricevuti.path;
                    //printf("%d\t\t%.2f\t\t%.2f\t\t%s\n",num_count,average,deviaziones,nomef);
                    printf("%d\t\t%.2f\t\t%.2f\t\t%s\n",dati_ricevuti.num_count,dati_ricevuti.average,dati_ricevuti.deviaziones,s);
                }    
            }
            //chiudi il socket del client
            close(client_socket);
        }
        
        //chiudi il socket del server
        close(server_socket);
        
        // Distruzione del mutex
        pthread_mutex_destroy(&mutex);
        return 0;
    }
    
}
