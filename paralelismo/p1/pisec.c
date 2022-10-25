#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

int main(int argc, char *argv[]){

    int i, done = 0, n, count;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;
	double aux;
	int procs,id,total;
	
    MPI_Init(&argc, &argv); //Incializar 
	MPI_Comm_size(MPI_COMM_WORLD, &procs); //numero de procesos 
	MPI_Comm_rank(MPI_COMM_WORLD, &id); //identificacion de procesos 


    while (!done)
    {
		if (id==0){
        	printf("Enter the number of points: (0 quits) \n");
        	scanf("%d",&n);
			for(int j=1;j<procs;j++)
				MPI_Send(&n,1,MPI_INT,j,0,MPI_COMM_WORLD);
		}else
			MPI_Recv(&n,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

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


        if(id!=0)
			MPI_Send(&count,1,MPI_DOUBLE,0,0,MPI_COMM_WORLD);
		else{
		for(int j=1;j<procs;j++){
				MPI_Recv(&total,1,MPI_DOUBLE,j,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				count += total;
		}
			pi = ((double) count/(double) n)*4.0;
			total=0;
			printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        }
    }
    MPI_Finalize();//finalizar 
}
