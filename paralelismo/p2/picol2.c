#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

//MPI_Reduce(&pi, &aux, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
int MPI_FlattreeReduce(void *buf, void *aux, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
     int i;
     int id,procs;
     MPI_Status st;
     MPI_Comm_size(MPI_COMM_WORLD, &procs);
     MPI_Comm_rank(MPI_COMM_WORLD, &id);

     if(id!=root)//Si no eres el proceso raíz envías count
              MPI_Send(buf, count, datatype, root, 0, comm);  
      else{        
		for(i=0;i<procs;i++){
			if(i!=root)//El proc raíz recibe los counts de todos los procs menos de el mismo
				MPI_Recv(buf, count, datatype, i, 0, comm, &st);
				
            *(int *)(aux)=*(int *)(buf)+*(int *)(aux);//El proc suma todos los counts incluído el mismo
		}
	}
	  
}
      
int MPI_BinomialBcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
        int i, destino, origen;
        int id,procs;
        MPI_Status st;
        MPI_Comm_size(MPI_COMM_WORLD, &procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &id);
         
        for(i=1; i<=ceil(log2(procs)); i++){  //numero de pasos a realizar
            if(id<pow(2,i-1)){  //si el proceso tiene que enviar
                destino = id+pow(2,i-1);  //se calcula cuales son los procesos destino que le corresponden a este
                if (destino<procs){  //se comprueba que no nos excedemos del número de procesos
                    MPI_Send(buf, count, datatype, destino, 0, comm);  //se envía el dato
                }
            } else{
                if(id<pow(2,i)){  
                    origen = id-pow(2,i-1);  //se reciben datos de los procesos que le corresponden a este
                    MPI_Recv(buf, count, datatype, origen, 0, comm, &st);
                }
            }
             
        } 
         
    }
    
int main(int argc, char *argv[]){

    int i, done = 0, n, count;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;
    int procs,id,total,aux;
	
    MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &procs);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	
    while (!done){
		aux=0;
		if (id==0){
        	printf("Enter the number of points: (0 quits) \n");
        	scanf("%d",&n);
        }

		MPI_BinomialBcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
			
        if (n == 0) break;

        count = 0;  
		srand(id+time(NULL));

        for (i = 1; i <= n; i+=procs) {
            // Get the random numbers between 0 and 1
	    x = ((double) rand()) / ((double) RAND_MAX);
	    y = ((double) rand()) / ((double) RAND_MAX);

	    // Calculate the square root of the squares
	    z = sqrt((x*x)+(y*y));

	    // Check whether z is within the circle
	    if(z <= 1.0)
                count++;
        }
		MPI_FlattreeReduce(&count, &aux, 1, MPI_INT, 0, MPI_COMM_WORLD);

		if (id==0){
			pi = ((double) aux/(double) n)*4.0;
			printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
		}
	}

    MPI_Finalize();
}
