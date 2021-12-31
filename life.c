#include <stdio.h>
#include <stdlib.h>
#include <mpi.h> 

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

int id2D, colID, ndim=2;
int coords1D[1], coords2D[2], dims[2], aij, alocal[3];  
int belongs[2], periods[2], reorder;
MPI_Comm comm2D, commrow;
 MPI_Status Stat;

MPI_Request request = MPI_REQUEST_NULL;


/*Attiva MPI*/
MPI_Init(&argc, &argv);
/*Trova il numero totale dei processi*/
MPI_Comm_size (MPI_COMM_WORLD, &nproc);
/*Da ad ogni processo il proprio numero identificativo*/
MPI_Comm_rank (MPI_COMM_WORLD, &me);

/* crea la topologia cartesiana 2D */
dims[0] = nproc;		/* numero di righe */  
dims[1] = 1;	/* numero di colonne */
MPI_Cart_create(MPI_COMM_WORLD, ndim, dims, periods, reorder,  &comm2D);
MPI_Comm_rank(comm2D, &id2D); 
MPI_Cart_coords(comm2D, id2D, ndim, coords2D);

/* Crea le sottogriglie di righe 1D */  
belongs[0] = 0;
belongs[1] = 1;
MPI_Cart_sub(comm2D, belongs, &commrow);  
MPI_Comm_rank(commrow, &colID);  
MPI_Cart_coords(commrow, colID, 1, coords1D);

/* la barrier assicura che tutti conoscano le proprie coordinate prima di assegnare aij*/
MPI_Barrier(MPI_COMM_WORLD); 

// Se sono a radice
if(me == 0)
{
    printf("inserire numero di righe = \n"); 
    scanf("%d",&rows); 
    
    printf("inserire numero di colonne = \n"); 
    scanf("%d",&cols);

    printf("Inserisci numero di round:\n");
    scanf("%d",&round);

    // Numero di righe da processare
    local_rows = rows/nproc;  
    
    // Alloco spazio di memoria
    world = malloc(rows * cols * sizeof(char));

    for ( int i = 0 ; i< rows; i++){
        for (int j = 0; j< cols; j++){
            world[i * cols +j] = 'd';
        }
    } 
    /*
    world[27] = 'a';
    world[34] = 'a';
    world[35] = 'a';
    world[36] = 'a';
    */

    world[1] = 'a';
    world[9] = 'a';
    world[17] = 'a';

    printf("\nLa matrice di partenza è: \n\n");
    stampa(world, rows, cols);

} // fine me==0

// Spedisco m, n, local_m e v
MPI_Bcast(&rows,1,MPI_INT,0,MPI_COMM_WORLD);  
MPI_Bcast(&cols,1,MPI_INT,0,MPI_COMM_WORLD);            
MPI_Bcast(&local_rows,1,MPI_INT,0,MPI_COMM_WORLD);
MPI_Bcast(&round,1,MPI_INT,0,MPI_COMM_WORLD);


// tutti allocano A locale e il vettore dei risultati
localWorld = malloc(local_rows * cols * sizeof(char));

// Adesso 0 invia a tutti un pezzo della matrice
MPI_Scatter(
    &world[0], local_rows*cols, MPI_CHAR,
    &localWorld[0], local_rows*cols, MPI_CHAR,
    0, MPI_COMM_WORLD);

MPI_Barrier(MPI_COMM_WORLD);
T_inizio=MPI_Wtime(); //inizio del cronometro per il calcolo del tempo di inizio

//Ogni processore alloca una matrice temporanea dove vengono inviate anche la prima
// e ultima riga delle sottomatrici contenute dai processori precedente e successivo

char *localWorldTmp, *localWorldTmpBot;
char *firstRow, *lastRow, *firstRowRecv, *lastRowRecv;
firstRow = malloc (cols * sizeof(char));
lastRow = malloc (cols * sizeof(char));

for(int r = 1; r <= round; r++){
    
    //printf("Processore %d \n", me);
    //stampa(localWorld, local_rows, cols);
    
    //Invio della prima e ultima riga ai processori vicini
    if( me == 0 ){
        lastRow = malloc (cols * sizeof(char));
        for(int i = 0 ; i<cols; i++){
            lastRow[i] = localWorld[i + cols * (local_rows-1)];
        
        }

        MPI_Isend(lastRow, cols, MPI_CHAR, me+1, 0, MPI_COMM_WORLD, &request);

        lastRowRecv = malloc (cols * sizeof(char));
        MPI_Irecv(lastRowRecv,cols, MPI_CHAR, me+1, 0, MPI_COMM_WORLD, &request);

    }
    else if( me == nproc-1){
        firstRow = malloc (cols * sizeof(char));
        for(int i = 0 ; i<cols; i++){
            firstRow[i] = localWorld[i];
        }

        MPI_Isend(firstRow, cols, MPI_CHAR, me-1, 0, MPI_COMM_WORLD, &request);

        firstRowRecv = malloc (cols * sizeof(char));
        MPI_Irecv(firstRowRecv,cols, MPI_CHAR, me-1, 0, MPI_COMM_WORLD,&request);

    }
    else{

        firstRow = malloc (cols * sizeof(char));
        lastRow = malloc (cols * sizeof(char));
            
        //printf("Processore %d firstRow = ", me);

        for(int i = 0 ; i<cols; i++){
            lastRow[i] = localWorld[cols * (local_rows-1)+i];
        }

        for(int i = 0 ; i<cols; i++){
            firstRow[i] = localWorld[i];
            //printf("%c",firstRow[i]);
        }
            //printf("\n");

        
        MPI_Isend(firstRow, cols, MPI_CHAR, me-1, 0, MPI_COMM_WORLD, &request);
        firstRowRecv = malloc (cols * sizeof(char));
        MPI_Irecv(firstRowRecv,cols, MPI_CHAR, me-1, 0, MPI_COMM_WORLD, &request);

        MPI_Isend(lastRow, cols, MPI_CHAR, me+1, 0, MPI_COMM_WORLD, &request);
        lastRowRecv = malloc (cols * sizeof(char));
        MPI_Irecv(lastRowRecv,cols, MPI_CHAR, me+1, 0, MPI_COMM_WORLD, &request);
    }
    MPI_Wait(&request, &Stat);
    MPI_Barrier(MPI_COMM_WORLD);

    //Salvo le seconde e penultime righe necessarie al calcolo
    char *rowTopToSave = malloc (cols * sizeof(char));
    char *rowBotToSave = malloc (cols * sizeof(char));
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

    //printf("Processore %d\n", me);
    //stampa(localWorld,local_rows, cols);

    localWorldTmp = malloc(cols * 3 * sizeof(char));

    //I processori adesso formano quella che sarà la nuova matrice temporanea su cui eseguire i calcoli
    if(me == 0){
        //stampa (localWorld, local_rows, cols);
        
        for(int j = 0; j< cols; j++){
            localWorldTmp[j] = rowBotToSave[j]; 
        }
        for(int j = 0; j< cols; j++){
            localWorldTmp[cols + j] = localWorld[(local_rows -1) * cols + j]; 
        }

        for (int i = 0 ; i< cols; i++){

            localWorldTmp[2*cols + i] = lastRowRecv[i];
        }
        //printf("Processore %d, la localWorldTmp è: \n",me);
        //stampa(localWorldTmp, 3, cols);
    
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
        
        //printf("Processore %d, la localWorldTmp è: \n",me);
        //stampa(localWorldTmp, 3, cols);
    }else{
        localWorldTmpBot = malloc ( cols * 3 * sizeof(char));

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
        //printf("Processore %d, la localWorldTmp è: \n",me);
        //stampa(localWorldTmp, 3, cols);
    }

    //I processori eseguono i calcoli

    if(me == 0 || me == nproc-1){
        next_round(localWorldTmp, 3, cols);
    }else{
        next_round(localWorldTmp, 3, cols);
        next_round(localWorldTmpBot, 3, cols);
    }

    //printf("Processore %d dopo next round ho :\n", me);
    //stampa(localWorldTmp,3,cols);

    //I processori ritornano alla matrice grande come quella di partenza assegnatagli, ma con i valori aggiornati
    if(me == 0){
        //printf("Processore %d, la localWorldTmp è: \n",me);
        //stampa(localWorldTmp, local_rows +1, cols);
    
        for(int j = 0; j< cols; j++){
            localWorld[(local_rows-1)*cols +j] = localWorldTmp[cols +j]; 
        }

    }else if(me == nproc-1){
        //printf("Processore %d, la localWorldTmp è: \n",me);
        //stampa(localWorldTmp, local_rows +1, cols);

        for(int j = 0; j< cols; j++){
            localWorld[j] = localWorldTmp[cols +j]; 
        }
        
        
    }else{
        //printf("Processore %d, la localWorldTmp è: \n",me);
        //stampa(localWorldTmp, local_rows +2, cols);
        
        for(int j = 0; j< cols; j++){
            localWorld[j] = localWorldTmp[cols +j]; 
        }
        for(int j = 0; j< cols; j++){
            localWorld[(local_rows-1)*cols +j] = localWorldTmpBot[cols +j]; 
        }
        
    }

    // 0 raccoglie i risultati parziali
    MPI_Gather(localWorld, local_rows * cols, MPI_CHAR,&world[0],local_rows * cols,MPI_CHAR,0,MPI_COMM_WORLD);

   
    if(me == 0){
        printf("ANNO %d: \n\n",r);
        stampa (world, rows, cols);
    }
}

T_fine=MPI_Wtime()-T_inizio; // calcolo del tempo di fine

if(me==0){

    printf("\nTempo calcolo locale: %lf\n", T_fine);

}

MPI_Finalize (); /* Disattiva MPI */
return 0;  
}