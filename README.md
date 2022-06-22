# Game Of Life Report
Implementation of GameOfLife using MPI library.

| Dati progetto|  |  
|-------------|----------| 
| Studente | SILVIO CRESCI | 
| Progetto di corso| Game Of Life |

## Presentazione della soluzione 

Questo progetto ha come obiettivo quello di eseguire in modo parallelo Game Of Life in modo da avere le migliori prestazioni possibili.  

Al fine di raggiungere tale scopo è stato utilizzato il protocollo MPI e nello specifico la sua implementazione OpenMPI.

Nell'approccio utilizzato la matrice di gioco viene inizialmente rappresentata come un vettore per poi essere distribuita tra i vari processori per righe.

Le dimensioni della matrice (righe e colonne) vengono specificate dall'utente, dopodiché il processore root la inizializzerà automaticamente con delle strutture note (di cui già si conosce il comportamento con il passare delle generazioni). La matrice viene inizializzata automaticamente poiché ciò non impatta sulle prestazioni ma serve solamente a verificare la correttezza dell'algoritmo. A questo punto il processore root suddivide la matrice per righe e la distribuisce tra tutti i processori incluso se stesso. Se si ha a disposizione un solo processore ovviamente non si può parlare di parallelizzazione e il programma verrà eseguito in modo sequenziale. 

La taglia locale (per ogni processore) del problema è data dalla divisione:  
[(numero di righe / numero di processori) * numero di colonne]  
e l'eventuale resto viene suddiviso tra tutti i processori.

Una volta effettuata la suddivisione, il programma inizia la computazione per ogni generazione:  
- Ricevo riga superiore e/o inferiore (non bloccante);
- Invio prima e/o ultima riga (non bloccante);
- Eseguo la computazione di tutte le righe per cui non servono quelle da ricevere;
- Attendo ricezione righe (bloccante);
- Completo computazione con righe interessate dalle righe ricevute.

Una volta completata la computazione per tutte le generazioni, le matrici locali (distribuite su ogni processore) vengono inviate al processore root che le fonderà in un'unica matrice che poi sarà l'output del nostro programma.

In caso la matrice abbia un numero di righe e colonne minore o uguale a 20, questa verrà stampata nel terminale sia nella sua forma di input che nella sua forma di output.

## Dettagli implementatitvi

Dopo aver inzializzato la matrice, il processore root calcola il numero di righe da inviare ad ogni processore e controlla se c'è un eventuale resto. In caso positivo, quest'ultimo sarà distribuito tra i vari processori incrementando la variabile *sendcount*.


```c
// Numero di righe da processare
    local_rows = rows/nproc;
    int resto = rows%nproc;
    
    int dist = 0;
    
    for(int i = 0; i < nproc; i++){
        sendcount[i] = local_rows * cols;
        displ[i] = dist; 

        if(resto != 0){
            sendcount[i] += cols;

            resto-- ;
        }

        dist += sendcount[i];
    }
```
Il vettore *displ* serve per tenere traccia del displacement, che insieme a *sendcount* viene utilizzato per la distribuzione delle righe mediante la **Scatterv**. Per quanto riguarda invece il numero di righe che ogni processore dovrà avere viene utilizzata la semplice **Scatter**.

```c
//Il processore 0 comunica il numero di righe che ogni processore avrà
MPI_Scatter(sendcount, 1, MPI_INT, &local_rows, 1, MPI_INT, 0,MPI_COMM_WORLD);

// Adesso 0 invia a tutti un pezzo della matrice
MPI_Scatterv(
    world, sendcount, displ, MPI_CHAR,
    localWorld, local_rows*cols, MPI_CHAR,
    0, MPI_COMM_WORLD);

```

Una volta che tutti i processori hanno la loro porzione di matrice locale, inizia la computazione per ogni generazione. I processori hanno comportamenti diversi in base alla loro posizione, in particolare abbiamo tre possibili posizioni: primo processore, ultimo processore, i processori nel mezzo.

Siccome vogliamo sfruttare il tempo necessario all'invio e alla ricezione dei messaggi tra i processori, iniziando ad eseguire la computazione per le celle che non necessitano della presenza della riga da ricevere, abbiamo bisogno che ogni processore salvi la seconda e/o la penultima riga della matrice locale in modo da poterle utilizzare successivamente in caso venga cambiata dalla prima fase di computazione.

```c
if( me == 0 ){
    
    for ( int i = 0; i < cols; i++ ){
        rowBotToSave[i] = localWorld[(local_rows-2)*cols + i];
    }

}else if(me == nproc-1){

    for(int i = 0; i < cols; i++){
        rowTopToSave[i] = localWorld[cols + i];
    }

}else{

    for ( int i = 0; i < cols; i++ ){
        rowBotToSave[i] = localWorld[(local_rows-2)*cols + i];
    }

    for(int i = 0; i < cols; i++){
        rowTopToSave[i] = localWorld[cols + i];
    }
}
```

Salvate le informazioni necessarie, possiamo inziare con l'invio delle righe tra i processori. In particolare:

- il processore con rank 0: essendo il primo processore, deve soltanto comunicare con il processore successivo, a cui deve inviare l'ultima riga e da cui deve riceverne la prima.
- 0 < rank < numero processori -1: essendo processori centrali, dovranno comunicare sia con il processore precedente che con quello successivo, e scambiarsi le prime e ultime righe.
- il processore con rank nproc-1: essendo l'ultimo processore, deve soltanto comunicare con il processore precedente, a cui deve inviare la prima riga e da cui deve riceverne l'ultima. 

```c
//Invio della prima e ultima riga ai processori vicini
if( me == 0 ){
    for(int i = 0 ; i<cols; i++){
        lastRow[i] = localWorld[i + cols * (local_rows-1)];
    }

    MPI_Isend(lastRow, cols, MPI_CHAR, me+1, 0, MPI_COMM_WORLD, &request[count++]);

    MPI_Irecv(lastRowRecv,cols, MPI_CHAR, me+1, 0, MPI_COMM_WORLD, &request[count++]);

}else if( me == nproc-1){

    for(int i = 0 ; i<cols; i++){
        firstRow[i] = localWorld[i];
    }

    MPI_Isend(firstRow, cols, MPI_CHAR, me-1, 0, MPI_COMM_WORLD, &request[count++]);

    MPI_Irecv(firstRowRecv,cols, MPI_CHAR, me-1, 0, MPI_COMM_WORLD,&request[count++]);

}else{

    for(int i = 0 ; i<cols; i++){
        lastRow[i] = localWorld[cols * (local_rows-1)+i];
    }

    for(int i = 0 ; i<cols; i++){
        firstRow[i] = localWorld[i];
    }

    MPI_Isend(firstRow, cols, MPI_CHAR, me-1, 0, MPI_COMM_WORLD, &request[count++]);
    
    MPI_Irecv(firstRowRecv,cols, MPI_CHAR, me-1, 0, MPI_COMM_WORLD, &request[count++]);
    

    MPI_Isend(lastRow, cols, MPI_CHAR, me+1, 0, MPI_COMM_WORLD, &request[count++]);
    
    MPI_Irecv(lastRowRecv,cols, MPI_CHAR, me+1, 0, MPI_COMM_WORLD, &request[count++]);
    
}
```
La sincronizzazione tra le generazioni avviene attraverso l'utilizzo di **MPI_Waitall**.

Per procedere con l'illustrazione dell'algoritmo implementato è necessario dare una breve spiegazione delle funzioni utilizzate.

### Funzioni 

**get_count_neighbour**

Questa funzione serve a calcolare il numero di vicini *alive* per una detereminata cella target passatagli come parametro:

```c
int get_count_neighbour(char *world, int i_me, int j_me, int row, int cols){

    int count=0;
    
    for(int i = i_me-1; i<=i_me+1; i++){
        for(int j = j_me-1; j<=j_me+1; j++){
            if(i<0 || j<0);
            else if(i >= row || j >= cols);
            else if(i==i_me && j == j_me);
            else if( world[j + i*cols] == 'a') count ++;
        }
    }
    
    return count;
}
```

**first_next_round**

È la funzione che permette di eseguire la prima fase di computazione sulla matrice, ovvero la computazione sulle celle che non necessitano della presenza della riga da ricevere da altri processori.

```c
void first_next_round(int me, int nproc, char *world, int rows, int cols){
    char *world_tmp = malloc (rows * cols * sizeof(char));

    int local_count;
    int a, b;
    if( me == 0 ){
        a = 0;
        b = rows-1;

    } else if( me == nproc -1){
        a = 1;
        b = rows;
    } else {
        a = 1;
        b = rows-1;
    }
    for (int i=a; i<b; i++){
        for (int j=0; j< cols; j++){
            local_count = get_count_neighbour(world,i,j,rows,cols);
            
            //se la cella analizzata è alive
            if(world[j + i*cols] == 'a'){
                if(local_count < 2) world_tmp[j + i*cols] = 'd';
                else if(local_count == 2 || local_count == 3)world_tmp[j + i*cols] = 'a';
                else if(local_count > 3) world_tmp[j + i*cols] = 'd';
            }//se invece la cella è dead
            else if (world[j + i*cols] == 'd'){
                if(local_count == 3) world_tmp[j + i*cols] = 'a';
                else world_tmp[j + i*cols] = 'd';
            }
        }
    }

    for (int i=a; i<b; i++){
        for (int j=0; j< cols; j++){
            world[j + i*cols] = world_tmp[j + i*cols];
        }
    }

    free(world_tmp); 
}
```

**next_round**

Una volta completata la comunicazione tra i processori per lo scambio delle righe, viene chiamata questa funzione che serve semplicemente ad eseguire la computazione necessaria per il passaggio di generazione.

```c
char *world_tmp = malloc(rows * cols * sizeof(char));
    
    int local_count;

    for (int i=0; i<rows; i++){
        for (int j=0; j< cols; j++){
            local_count = get_count_neighbour(world,i,j,rows,cols);
            
            //se la cella analizzata è alive
            if(world[j + i*cols] == 'a'){
                if(local_count < 2) world_tmp[j + i*cols] = 'd';
                else if(local_count == 2 || local_count == 3)world_tmp[j + i*cols] = 'a';
                else if(local_count > 3) world_tmp[j + i*cols] = 'd';
            }//se invece la cella è dead
            else if (world[j + i*cols] == 'd'){
                if(local_count == 3) world_tmp[j + i*cols] = 'a';
                else world_tmp[j + i*cols] = 'd';
            }
        }
    }

    for (int i=0; i<rows; i++){
        for (int j=0; j< cols; j++){
            world[j + i*cols] = world_tmp[j + i*cols];
        }
    }
    free(world_tmp);
}
```
Ritorniamo ora all'illustrazione dell'algoritmo implementato. 

Nel frattempo che avviene lo scambio di messaggi, si inizia con la prima fase di computazione mediante la funzione *first_next_roud* presentata pocanzi. Una volta che le comunicazioni sono finite 
- il primo processore andrà a formare una sola matrice temporanea di tre righe contenente la penultima riga salvata precedentemente, la sua ultima riga e la riga ricevuta dal processore successivo.
- l' ultimo processore andrà a formare una sola matrice temporanea di tre righe contenente la seconda riga salvata precedentemente, la sua prima riga e la riga ricevuta dal processore precedente.
- i processori centrali faranno la stessa cosa, solo con due matrici poiché comunicano con il processore precedente e il processore successivo.

Su queste matrici temporanee che vanno a formare, ogni processore chiama la funzione *next_round* in modo da eseguire la computazione per il passaggio di generazione.

Al termine di tali operazioni, ogni processore riformerà una matrice grande quanto quella assegnatagli all'inizio, ma con i valori aggiornati.

```c
if(me == 0){
    
    for(int j = 0; j< cols; j++){
        localWorld[(local_rows-1)*cols +j] = localWorldTmp[cols +j]; 
    }

}else if(me == nproc-1){
    
    for(int j = 0; j< cols; j++){
        localWorld[j] = localWorldTmp[cols +j]; 
    }
    
}else{
    
    for(int j = 0; j< cols; j++){
        localWorld[j] = localWorldTmp[cols +j]; 
    }

    for(int j = 0; j< cols; j++){
        localWorld[(local_rows-1)*cols +j] = localWorldTmpBot[cols +j]; 
    }
    
}
```

Soltanto quando ogni processore avrà finito di eseguire i calcoli per ogni generazione sulla sua porzione di matrice (*localWorld*), viene formata la matrice risultato (*world*) mediante la **MPI_Gatherv**

```c
// 0 raccoglie i risultati parziali
MPI_Gatherv(localWorld, local_rows * cols, MPI_CHAR, world, sendcount, displ, MPI_CHAR,0,MPI_COMM_WORLD);
```

## Analisi Prestazioni

I tempi d'esecuzione sono stati raccolti partendo immediatamente dopo la distribuzione delle righe tra i processori, e finendo dopo l'esecuzione delle varie *free* per liberare la memoria.

```c
MPI_Scatterv(
    world, sendcount, displ, MPI_CHAR,
    localWorld, local_rows*cols, MPI_CHAR,
    0, MPI_COMM_WORLD);

MPI_Barrier(MPI_COMM_WORLD);

//inizio del cronometro per il calcolo del tempo di inizio
T_inizio=MPI_Wtime();

for(int r = 1; r <= round; r++){

    //Computazione per ogni generazione

}

//Libero la memoria
if(me != 0){
    free...
}
if(me != nproc-1) {    
    free...
}
if (me != 0 && me != nproc-1){
    free...
}
if(me==0){
    free...
}

// calcolo del tempo di fine
T_fine=MPI_Wtime()-T_inizio; 
```

Sono state valutate le performance sia in termini di weak scalability sia in termini di strong scalability.
Tutti i test sono stati valutati per 100 generazioni, il cluster è composto in totale da 8 macchine ognuna quad core ( C2 - standard-4 ), le macchine sono state distribuite:
- 2 in us-west1 (Oregon);
- 2 in us-west2 (Los Angeles);
- 2 in us-west3 (Salt Lake City);
- 2 in us-west4 (Las Vegas).

### Strong Scalability 

La strong scalability è dominata dalla legge di Amdhal, essa pone un limite superiore allo speedup.

### Speedup = 1/ ( s + p/n ) = T(1,size)/ T(n ,size)

Nella seguente tabella sono riportati i risultati ottenuti variando il numero dei processori.

|           | Sequenziale | P2        |      |      | P4        |      |      | P6        |      |      | P8        |      |      | P10       |      |      | P12       |      |      | P14       |      |      | P16       |      |      | P18       |      |      | P20       |       |      | P22       |       |      | P24       |       |      | P26       |       |      | P28       |       |      | P30       |       |      | P32       |       |      |
|-----------|-------------|-----------|------|------|-----------|------|------|-----------|------|------|-----------|------|------|-----------|------|------|-----------|------|------|-----------|------|------|-----------|------|------|-----------|------|------|-----------|-------|------|-----------|-------|------|-----------|-------|------|-----------|-------|------|-----------|-------|------|-----------|-------|------|-----------|-------|------|
| N         | tempo (s)   | tempo (s) | Sp   | Ep   | tempo (s) | Sp   | Ep   | tempo (s) | Sp   | Ep   | tempo (s) | Sp   | Ep   | tempo (s) | Sp   | Ep   | tempo (s) | Sp   | Ep   | tempo (s) | Sp   | Ep   | tempo (s) | Sp   | Ep   | tempo (s) | Sp   | Ep   | tempo (s) | Sp    | Ep   | tempo (s) | Sp    | Ep   | tempo (s) | Sp    | Ep   | tempo (s) | Sp    | Ep   | tempo (s) | Sp    | Ep   | tempo (s) | Sp    | Ep   | tempo (s) | Sp    | Ep   |
| 1000*1000 |        3,41 |      1,71 | 1,99 | 1,00 |      1,53 | 2,23 | 0,56 |      1,05 | 3,25 | 0,54 |      0,79 | 4,32 | 0,54 |      1,18 | 2,89 | 0,29 |      1,23 | 2,77 | 0,23 |      1,38 | 2,47 | 0,18 |      1,07 | 3,19 | 0,20 |      1,02 | 3,34 | 0,19 |      1,09 |  3,13 | 0,16 |      1,03 |  3,31 | 0,15 |      1,03 |  3,31 | 0,14 |      1,26 |  2,71 | 0,10 |      1,27 |  2,69 | 0,10 |      1,33 |  2,56 | 0,09 |      1,37 | 2,49  | 0,08 |
| 2000*2000 |       13,65 |      6,86 | 1,99 | 0,99 |      6,07 | 2,23 | 0,56 |      4,13 | 3,31 | 0,55 |      3,11 | 4,39 | 0,55 |      2,70 | 5,06 | 0,51 |      2,63 | 5,19 | 0,43 |      2,38 | 5,74 | 0,41 |      2,37 | 5,76 | 0,36 |      2,22 | 6,15 | 0,34 |      2,03 |  6,72 | 0,34 |      1,99 |  6,86 | 0,31 |      2,01 |  6,79 | 0,28 |      2,18 |  6,26 | 0,24 |      2,33 |  5,86 | 0,21 |      2,53 |  5,40 | 0,18 |      2,78 | 4,91  | 0,15 |
| 4000*4000 |       54,58 |     27,41 | 1,99 | 1,00 |     24,24 | 2,25 | 0,56 |     16,40 | 3,33 | 0,55 |     12,37 | 4,41 | 0,55 |     10,15 | 5,38 | 0,54 |      8,81 | 6,20 | 0,52 |      7,91 | 6,90 | 0,49 |      7,26 | 7,52 | 0,47 |      6,71 | 8,13 | 0,45 |      6,21 |  8,79 | 0,44 |      5,75 |  9,49 | 0,43 |      5,44 | 10,03 | 0,42 |      5,40 | 10,11 | 0,42 |      5,49 |  9,94 | 0,36 |      5,61 |  9,73 | 0,32 |      5,49 | 9,94  | 0,31 |
| 8000*8000 |      221,08 |    109,77 | 2,00 | 1,00 |     97,96 | 2,26 | 0,56 |     66,68 | 3,32 | 0,55 |     49,04 | 4,51 | 0,56 |     39,61 | 5,58 | 0,56 |     33,83 | 6,54 | 0,54 |     29,14 | 7,59 | 0,54 |     25,96 | 8,52 | 0,53 |     23,35 | 9,47 | 0,53 |     21,77 | 10,16 | 0,51 |     20,65 | 10,71 | 0,49 |     18,33 | 12,06 | 0,50 |     18,00 | 12,28 | 0,47 |     17,95 | 12,32 | 0,44 |     16,83 | 13,14 | 0,44 |     16,38 | 13,50 | 0,42 |

Di seguito sono riportati i grafici di Speedup ed Efficienza calcolati sulla scalabilità forte:

![Speedup](/Assets/Scalabilità__forte_Speedup.png)

![Efficienza](/Assets/Efficienza_scalabilità_forte.png)

### Valutazioni scalabilità forte

### Weak Scalability 

La weak scalability è dominata dalla legge di Gustafson, essa mette in relazione la dimensione del problema con il numero di processori, infatti lo speedup ottenuto è detto anche scaled-speedup.

### Speedup scalato = s + p x N = N( T(1,size) ) / T( N , N * size )

Di seguito i risultati ottenuti variando la dimensione del problema (in termini di righe) in funzione del numero di processori.

|            | 2000*1000 | 2000*2000 | 4000*2000 | 4000*4000 | 8000*4000 |
|------------|-----------|-----------|-----------|-----------|-----------|
| nproc      |     2     |     4     |     8     |     16    |     32    |
| tempo      | 3,46      | 6,07      | 6,17      | 7,26      | 9,59      |
| speedup    | 1,97      | 2,24      | 4,42      | 7,51      | 11,37     |
| efficienza | 0,985     | 0,56      | 0,5525    | 0,469375  | 0,3553125 |

Di seguito i risultati ottenuti variando la dimensione del problema (in termini di colonne) in funzione del numero di processori.

|            | 1000*2000 | 2000*2000 | 2000*4000 | 4000*4000 | 4000*8000 |
|------------|-----------|-----------|-----------|-----------|-----------|
| nproc      |     2     |     4     |     8     |     16    |     32    |
| tempo      | 3,47      | 6,07      | 6,23      | 7,26      | 10,03     |
| speedup    | 1,96      | 2,24      | 4,37      | 7,51      | 10,88     |
| efficienza | 0,98      | 0,56      | 0,54625   | 0,469375  | 0,34      |

I risultati hanno evidenziato che non c'è una differenza sostanziale tra la variazione dando priorità alle righe che la variazione dando priorità alle colonne.


Di seguito sono riportati i grafici di speedup ed efficienza.

![Speedup](/Assets/Scalabilità_debole_speedup.png)

![Efficienza](/Assets/Scalabilità_debole_Efficienza.png)

### Valutazione scalabilità debole

## Esecuzione

Per il testing si è utilizzato il docker disponibile [qui](https://hub.docker.com/r/spagnuolocarmine/docker-mpi)

Per la build del Docker:

```
docker run -it --name mpi --cap-add=SYS_PTRACE --mount type=bind,source="$(pwd)"/[percorso da montare],target=/mpi  spagnuolocarmine/docker-mpi
```
Per la compilazione eseguire il comando:

```
mpicc life.c -o life
```

Per l'esecuzione:

```
mpirun --allow-run-as-root -np [nProc] life
```
Per l'esecuzione c'è bisogno di impostare solo nProc, ovvero il numero di processori su cui vogliamo eseguire il programma. Esempio:

```
mpirun --allow-run-as-root -np 4 life
```

Nota: ti ricordo che il programma darà in output la matrice iniziale e finale soltanto se la dimensione non supera le 20 righe o le 20 colonne, altrimenti stamperà solo il tempo.

## Conclusioni 

Come dimostrano i grafici per lo speedup e l'efficienza in termini di scalabilità debole e forte evidenziano che il programma scala in modo abbastanza costante soprattutto se si parla delle matrici con dimensione maggiore.

Il bilanciamento dei dati viene eseguito correttamente, ogni processore esegue la computazione su una sottomatrice della stessa dimenisone delle altre, eccezzion fatta per la presenza dell'eventuale resto fatta nella divisione iniziale che implica la presenza di al massimo una riga in più per alcuni processori.

