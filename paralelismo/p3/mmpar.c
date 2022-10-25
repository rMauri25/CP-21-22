#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <math.h>

#define DEBUG 0 //0 tiempos , 1 matriz 

#define N 1024

int main(int argc, char *argv[] ) {

  int i, j, id,aux, aux2, numprocs, reparto; 
  float matrix[N][N];
  float matrix2[N/2][N/2]; 
  float result2[N]; 
  float vector[N];
  float result[N];
  struct timeval  tv0,tv1, tv2,tv3;

  MPI_Init(&argc, &argv); //Inicializamos MPI (sincronizacion procesos)
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs); //Numero de procesos existentes
  MPI_Comm_rank(MPI_COMM_WORLD, &id); //Identificador de proceso


	if (id == 0){ //Inicializacion de vector y matriz
	  	for(i=0;i<N;i++) {
	   	 vector[i] = i;
	   	 for(j=0;j<N;j++) {
	   	   matrix[i][j] = i+j;
	   	 }
	 	}
	} 
	
	reparto = (int)floor((double) N/numprocs); //Numero de filas por proceso
	
	gettimeofday(&tv0, NULL);
	
	MPI_Bcast(vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD); //Envio de VECTOR a todos los procesos

	MPI_Scatter(matrix, reparto*N, MPI_FLOAT, matrix2, reparto*N, MPI_FLOAT, 0, MPI_COMM_WORLD); //Reparto de (reparto*N)FILAS de Matrix a Matrix2 desde el proc.0

	gettimeofday(&tv1, NULL);

	//Para el num de filas de cada proceso
	for(i=0;i<reparto;i++) {
	  result2[i]=0; //Inicializacion de vector resultado
	  for(j=0;j<N;j++) { //Multiplicacion filaXvector
	    result2[i] += matrix2[i][j]*vector[j];
	  }
	}

	
	//Filas restantes
	if(id==0 && (N % numprocs)){ //El proceso 0 se encarga de las filas restantes
		for(i=N-(N % numprocs);i<N;i++) {
		  	result[i]=0;
	 	 	for(j=0;j<N;j++) //Multiplicacion filaXvector
	   			result[i] += matrix[i][j]*vector[j];
		}
	}

	gettimeofday(&tv2, NULL);		    
	
	MPI_Gather(result2, reparto, MPI_FLOAT, result, reparto, MPI_FLOAT, 0, MPI_COMM_WORLD); 
	//Envío result2(tamaño filas proceso) a result a manos del proc.0
	
	gettimeofday(&tv3, NULL);
	
	//Cáculo de tiempos (microseconds - microseconds + 1000000*(seconds - seconds))
	
	int comp = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);  //tiempo de computacion 

	int comm1 = (tv3.tv_usec - tv2.tv_usec)+ 1000000 * (tv3.tv_sec - tv2.tv_sec); //comunicacion de recepcion
	
	int comm2 = (tv1.tv_usec - tv0.tv_usec)+ 1000000 * (tv1.tv_sec - tv0.tv_sec); //comunicacion de envio
	
	int comm=comm1+comm2;
	
	//Cálculo de tiempos computación y comunicación totales

	MPI_Reduce(&comp,&aux,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD); //Suma de COMP en aux : computation
	
	MPI_Reduce(&comm,&aux2,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD); //Suma de COMM en aux2 : comunication
	
	if (id == 0){		
		//El proceso 0 imprime los resultados y tiempos
		if (DEBUG){
		  printf("\n");
		  for(i=0;i<N;i++) 
		    printf(" %10.0f \t ",result[i]);
		  printf("\n");
		} else {
			
		  printf ("Time computation(seconds) = %lf\n", (double) aux/1E6);
		  
		  printf ("Time comunication(seconds) = %lf\n", (double) aux2/1E6);
		}
	}    

	MPI_Finalize(); //Liberar recursos reservados por MPI
	return 0;
}

