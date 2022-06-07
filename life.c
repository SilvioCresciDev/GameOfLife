#include <stdio.h>
#include <stdlib.h>
#include <mpi.h> 

void seed_linea(char *world, int row, int cols, int start_rows, int start_cols){

    for ( int i = 0 ; i< row; i++){
        for (int j = 0; j< cols; j++){
            world[i * cols +j] = 'd';
        }
    } 
    
    for(int i=start_cols; i<start_cols+3; i++){
    world[i + start_rows * cols] = 'a';
    }
}

void seed_glider(char *world, int row, int cols, int start_rows, int start_cols){

    for ( int i = 0 ; i< row; i++){
        for (int j = 0; j< cols; j++){
            world[i * cols +j] = 'd';
        }
    } 

    world[start_cols + start_rows * cols] = 'a';
    world[ 1 + start_cols + (start_rows+1) * cols] = 'a';
    seed_linea(world, row, cols, start_rows+2, start_cols-1);

}

void seed_forma(char *world, int row, int cols, int start_rows, int start_cols){

    for ( int i = 0 ; i< row; i++){
        for (int j = 0; j< cols; j++){
            world[i * cols +j] = 'd';
        }
    } 
    
    world[start_cols + start_rows * cols] = 'a';
    seed_linea(world, row, cols, start_rows+1, start_cols-1);
}

void stampa(char *world, int rows, int cols){
    for (int i=0; i<rows; i++){
        
        for (int j=0; j< cols; j++){
            if(world[j + i*cols] == 'd'){
                printf("|   ");
            } else {
                printf("| - ");
            }
            if (j == cols-1)
            {
                printf("|\n");
            }
        }
    }
    printf("\n\n");
}

int get_count(char *world, int i_me, int j_me, int row, int cols){

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
            local_count = get_count(world,i,j,rows,cols);
            
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

void next_round(char *world, int rows, int cols){
    
    char *world_tmp = malloc(rows * cols * sizeof(char));
    
    int local_count;

    for (int i=0; i<rows; i++){
        for (int j=0; j< cols; j++){
            local_count = get_count(world,i,j,rows,cols);
            
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

int main(int argc, char **argv) {

int nproc;              // Numero di processi totale
int me;                 // Il mio id
int rows,cols;                  // Dimensione della matrice
int local_rows;            // Dimensione dei dati locali
int round;

// Variabili di lavoro
char *world, *localWorld;

double T_inizio,T_fine,T_max;

MPI_Comm comm2D, commrow;
 MPI_Status Stat[8];

MPI_Request request[8];
for(int i = 0; i< 8; i++){
    request[i] = MPI_REQUEST_NULL;
}

/*Attiva MPI*/
MPI_Init(&argc, &argv);
/*Trova il numero totale dei processi*/
MPI_Comm_size (MPI_COMM_WORLD, &nproc);
/*Da ad ogni processo il proprio numero identificativo*/
MPI_Comm_rank (MPI_COMM_WORLD, &me);

/* la barrier assicura che tutti conoscano le proprie coordinate prima di assegnare aij*/
MPI_Barrier(MPI_COMM_WORLD); 

int *displ = malloc (nproc * sizeof(int));
int *sendcount = malloc(nproc * sizeof(int));

// Se sono la radice
if(me == 0)
{
    printf("inserire numero di righe= \n"); 
    scanf("%d",&rows); 
    
    printf("inserire numero di colonne= \n"); 
    scanf("%d",&cols);

    printf("Inserisci numero di round:\n");
    scanf("%d",&round);

    // Alloco spazio di memoria
    world = malloc(rows * cols * sizeof(char)); 
    
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
    //seed_linea(world, rows, cols, 1, 1);
    //seed_glider(world, rows, cols, 1, 1);
    seed_forma(world, rows, cols, 3, 3);
    if(rows <= 20 && cols <=20){
    stampa(world, rows, cols);
    }
} // fine me==0

//Sequenziale poiché ho un solo processore a disposizione
if(nproc == 1){
    T_inizio=MPI_Wtime(); //inizio del cronometro per il calcolo del tempo di inizio
    for(int i = 0; i < round; i++){
        next_round(world,rows,cols);
        //printf("La matrice al round %d è:\n\n", i+1);
        //stampa(world,rows,cols);
    }
    if(rows <= 20 && cols <=20){
    printf("La matrice risultante al round %d è:\n\n", round);
    stampa(world,rows,cols);
    }
    T_fine=MPI_Wtime()-T_inizio; // calcolo del tempo di fine
    printf("\nTempo calcolo locale: %lf\n", T_fine);

    free(world);
    free(sendcount);
    free(displ);

    MPI_Finalize (); // Disattiva MPI 
    return 0;
}
//Il processore 0 comunica il numero di righe che ogni processore avrà
MPI_Scatter(sendcount, 1, MPI_INT, &local_rows, 1, MPI_INT, 0, MPI_COMM_WORLD);

// Spedisco righe, colonne e numero di round
MPI_Bcast(&rows,1,MPI_INT,0,MPI_COMM_WORLD);  
MPI_Bcast(&cols,1,MPI_INT,0,MPI_COMM_WORLD);            
MPI_Bcast(&round,1,MPI_INT,0,MPI_COMM_WORLD);

//Sequenziale se dividendo la matrice di input non si hanno almeno due righe per processore
if (rows < nproc * 2){
    if(me==0){
        for(int i = 0; i < round; i++){
            next_round(world, rows, cols);
            }

        if(rows <= 20 && cols <=20){    
            printf("La matrice risultante al round %d è:\n\n", round);
            stampa(world,rows,cols);
        }

        free(world);
        free(sendcount);
        free(displ);
    }

    MPI_Finalize (); // Disattiva MPI 
    return 0;
}

local_rows /= cols;

// tutti allocano A locale e il vettore dei risultati
localWorld = malloc(local_rows * cols * sizeof(char));

// Adesso 0 invia a tutti un pezzo della matrice
MPI_Scatterv(
    world, sendcount, displ, MPI_CHAR,
    localWorld, local_rows*cols, MPI_CHAR,
    0, MPI_COMM_WORLD);

MPI_Barrier(MPI_COMM_WORLD);
T_inizio=MPI_Wtime(); //inizio del cronometro per il calcolo del tempo di inizio

//Ogni processore alloca una matrice temporanea dove vengono inviate anche la prima
// e ultima riga delle sottomatrici contenute dai processori precedente e successivo

char *localWorldTmp, *localWorldTmpBot;
char *firstRow, *lastRow, *firstRowRecv, *lastRowRecv;
char *rowTopToSave, *rowBotToSave;

localWorldTmp = malloc(cols * 3 * sizeof(char));

if(me != 0){
    firstRow = malloc (cols * sizeof(char));
    firstRowRecv = malloc (cols * sizeof(char));
    rowTopToSave = malloc (cols * sizeof(char));
    }

if(me != nproc-1) {    
    lastRow = malloc (cols * sizeof(char));
    lastRowRecv = malloc (cols * sizeof(char));
    rowBotToSave = malloc (cols * sizeof(char));
}

if (me != 0 && me != nproc-1){
    localWorldTmpBot = malloc ( cols * 3 * sizeof(char));
}


//Inizio del ciclo per di comunicazione e computazione necessaria ad eseguire ogni round
for(int r = 1; r <= round; r++){
    int count = 0;
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
    MPI_Waitall(count, request, Stat);
    
    //MPI_Barrier(MPI_COMM_WORLD);

    //Salvo le seconde e penultime righe necessarie al calcolo
    
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

    first_next_round(me, nproc, localWorld, local_rows, cols);

    //I processori adesso formano quella che sarà la nuova matrice temporanea su cui eseguire i calcoli
    if(me == 0){

        for(int j = 0; j< cols; j++){
            localWorldTmp[j] = rowBotToSave[j]; 
        }

        for(int j = 0; j< cols; j++){
            localWorldTmp[cols + j] = localWorld[(local_rows -1) * cols + j]; 
        }

        for (int i = 0 ; i< cols; i++){

            localWorldTmp[2*cols + i] = lastRowRecv[i];
        }
    }else if(me == nproc-1){
        
        for (int i = 0 ; i< cols; i++){
            localWorldTmp[i] = firstRowRecv[i];
        }

        for(int j = 0; j< cols; j++){
            localWorldTmp[cols + j] = localWorld[j]; 
        }

         for(int j = 0; j< cols; j++){
            localWorldTmp[2 * cols + j] = rowTopToSave[j]; 
        }
        
    }else{

        for (int i = 0 ; i< cols; i++){
            localWorldTmp[i] = firstRowRecv[i];
        }
        
        for(int j = 0; j< cols; j++){
            localWorldTmp[cols + j] = localWorld[j]; 
        }

        for(int j = 0; j< cols; j++){
            localWorldTmp[2 * cols + j] = rowTopToSave[j]; 
        }

        for(int j = 0; j< cols; j++){
            localWorldTmpBot[j] = rowBotToSave[j]; 
        }
        for(int j = 0; j< cols; j++){
            localWorldTmpBot[cols + j] = localWorld[(local_rows -1) * cols + j]; 
        }

        for (int i = 0 ; i< cols; i++){

            localWorldTmpBot[2*cols + i] = lastRowRecv[i];
        }

    }

    //I processori eseguono i calcoli

    if(me == 0 || me == nproc-1){
        next_round(localWorldTmp, 3, cols);
    }else{
        next_round(localWorldTmp, 3, cols);
        next_round(localWorldTmpBot, 3, cols);
    }

    //I processori ritornano alla matrice grande come quella di partenza assegnatagli, ma con i valori aggiornati
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
    //Se si vuole vedere il risultato anno per anno, attivare la seguente parte commentata.
    /*
    MPI_Gatherv(localWorld, local_rows * cols, MPI_CHAR, world, sendcount, displ, MPI_CHAR,0,MPI_COMM_WORLD);
    
    if(me == 0){
        printf("ANNO %d: \n\n",r);
        stampa (world, rows, cols);
    }
   */

}

// 0 raccoglie i risultati parziali
MPI_Gatherv(localWorld, local_rows * cols, MPI_CHAR, world, sendcount, displ, MPI_CHAR,0,MPI_COMM_WORLD);

    

if(me == 0){
        if(rows <= 20 && cols <=20){
        printf("La matrice risultante al round %d è: \n\n",round);
        stampa (world, rows, cols);
        }
    }

//Libero la memoria
free(localWorldTmp);
free(localWorld);
if(me != 0){
    free(firstRow);
    free(firstRowRecv);
    free(rowTopToSave);
    
}
if(me != nproc-1) {    
    free(lastRow);
    free(lastRowRecv);
    free(rowBotToSave);
    
}
if (me != 0 && me != nproc-1){
    free(localWorldTmpBot);
}
if(me==0){
    free(world);
    free(displ);
    free(sendcount);
}

T_fine=MPI_Wtime()-T_inizio; // calcolo del tempo di fine
if(me==0){

    printf("\nTempo calcolo locale: %lf\n", T_fine);

}

MPI_Finalize (); /* Disattiva MPI */
return 0;  
}