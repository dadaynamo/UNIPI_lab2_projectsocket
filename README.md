# UNIPI_lab2_projectsocket
Progetto per esame di Laboratorio2 presso UNIPI Pisa. Implementazione in di programma che usa un processo padre che calcola tramite thread da lui creati un serie di dati prelevati da file all'interno di cartelle. Il processo figlio invece fa da server e riceve tramite socket i messaggi inviati dai vari thread per poi stampare i risultati.

La creazione dei due processi viene eseguita tramite fork() e la connessione tramite socket viene effettuata da i due processi. Ogni singolo thread condivide la stessa connessione socket e scriverà su di esso in mutua esclusione.
La gestione dei file name e la visita delle varie path è gestita tramite coda unbounded che gestisce con var di condizione e mutex la sincronizzazione per thread in concorrenza.

# Traccia compito 
[Guarda la traccia in formato PDF per info!](https://drive.google.com/file/d/1DTi2gcoUpxUY6yPcsguJJHVjni0CWNqm/view?usp=share_link)
