# UNIPI_lab2_projectsocket
Progetto per esame di Laboratorio2 presso UNIPI Pisa. Implementazione in di programma che usa un processo padre che calcola tramite thread da lui creati un serie di dati prelevati da file all'interno di cartelle. Il processo figlio invece fa da server e riceve tramite socket i messaggi inviati dai vari thread per poi stampare i risultati.

La creazione dei due processi viene eseguita tramite fork() e la connessione tramite socket viene effettuata da i due processi. Ogni singolo thread condivide la stessa connessione socket e scriverà su di esso in mutua esclusione.
La gestione dei file name e la visita delle varie path è gestita tramite coda unbounded che gestisce con var di condizione e mutex la sincronizzazione per thread in concorrenza.



# Traccia compito 
[Guarda la traccia in formato PDF per info!](https://drive.google.com/file/d/1DTi2gcoUpxUY6yPcsguJJHVjni0CWNqm/view?usp=share_link)

# Compilazione e esecuzione di test su shell
Per quanto riguarda la compilazione e l'esecuzione automatica vi è un file makefile specifico.
Per compilare digitare sulla shell:
```
>$ make
```

Per eseguire il programma usando un thread e selezionando la visita a partire dalla path corrente:
```
>$ make test1
```

Per eseguire il programma usando 5 thread e selezionando la visita a partire dalla path corrente:
```
>$ make test2
```


Per eseguire il programma usando valgrind e controllare eventuali memory leak:
```
>$ make test3
```


Per eseguire il comando di cancellazione dei vari file .o e dell'eseguibile usare:
```
>$ make clean
```
