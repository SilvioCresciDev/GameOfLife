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

Le dimensioni della matrice (righe e colonne) vengono specificate dall'utente, dopodiché il processore root la inizializzerà automaticamente con delle strutture note, cioè di cui già si conosce il comportamento con il passare delle generazioni. La matrice viene inizializzata automaticamente poiché ciò non impatta sulle prestazioni ma è servito solamente a verificare la correttezza dell'esecuzione. A questo punto il processore root suddivide la matrice per righe e la distribuisce tra tutti i processori, incluso se stesso. Se si ha a disposizione un solo processore ovviamente non si può parlare di parallelizzazione e il programma verrà eseguito in modo sequenziale. 

La taglia locale (per ogni processore) del problema è data dalla divisione:  
[(numero di righe / numero di processori) * numero di colonne]  
e l'eventuale resto viene suddiviso tra tutti i processori.

Una volta suddivise le righe tra i vari processori, il programma inizia la computazione per ogni generazione:  
- Ricevo riga superiore e/o inferiore (non bloccante);
- Invio prima e/o ultima riga (non bloccante);
- Eseguo la computazione di tutte le righe per cui non servono quelle da ricevere;
- Attendo ricezione righe (bloccante);
- Completo computazione con righe interessate dalle righe ricevute;
- Sincronizzo con altri processori.

Una volta completata la computazione per tutte le generazioni, le matrici locali (distribuite su ogni processore) vengono inviate al processore root che le fonderà in un'unica matrice che poi sarà l'output del nostro programma.

In caso la matrice abbia un numero di righe e colonne minore o uguale a 20, questa verrà stampata nel terminale sia nella sua forma di input che nella sua forma di output.

## Dettagli implementatitvi
