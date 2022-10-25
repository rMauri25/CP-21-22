#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "options.h"

#define MAX_AMOUNT 20

struct bank {
    int num_accounts;        // number of accounts
    int *accounts;           // balance array
    pthread_cond_t **cond;    // pointer to the condition to control withdrawing
    pthread_mutex_t **mutex;  // pointer to the mutex to control accounts array access
    int fin;
};

struct args {
    int          thread_num;  // application defined thread #
    int          delay;       // delay between operations
    int	         iterations;  // number of operations
    int          net_total;   // total amount deposited by this thread
	int 	     transf_total;	 // total amount transfered by this thread
    struct       bank *bank;        // pointer to the bank (shared with other threads)
};

struct thread_info {
    pthread_t    id;    // id returned by pthread_create()
    struct args *args;  // pointer to the arguments
};

// Threads run on this function
void *deposit(void *ptr)
{
    struct args *args =  ptr;
    int amount, account, balance;

    while(args->iterations--) {
        amount  = rand() % MAX_AMOUNT;
        account = rand() % args->bank->num_accounts;

        if(pthread_mutex_lock(args->bank->mutex[account]));
          

        printf("Thread %d depositing %d on account %d\n",
            args->thread_num, amount, account);

        balance = args->bank->accounts[account];
        if(args->delay) usleep(args->delay); // Force a context switch

        balance += amount;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[account] = balance;
        if(args->delay) usleep(args->delay);

        args->net_total += amount;

        pthread_mutex_unlock(args->bank->mutex[account]);
    }
    return NULL;
}

// Threads transfer money on this function (exercise 2)
void *transfer(void *ptr){

	struct args *args =  ptr;	
	int amount  = rand() % MAX_AMOUNT;
	int init_account, dest_account; 
	int i,j;

	while(args->iterations--) {

		if(args->bank->num_accounts != 0){
			do{
				init_account = rand() % args->bank->num_accounts;
				dest_account = rand() % args->bank->num_accounts;
			}while(init_account==dest_account);
		}

		while(1){
			pthread_mutex_lock(args->bank->mutex[init_account]);
			if(pthread_mutex_trylock(args->bank->mutex[dest_account])){
				pthread_mutex_unlock(args->bank->mutex[init_account]);
				continue;
			}

		i = args->bank->accounts[init_account];
		if(args->delay) usleep(args->delay);

		amount = 0;
		if (i != 0)
			amount = rand() % (i+1);

		j = args->bank->accounts[dest_account];
		if(args->delay) usleep(args->delay);

		i -= amount;
		if(args->delay) usleep(args->delay);

		j += amount;
		if(args->delay) usleep(args->delay);

		printf("Thread %d transfering %d from account %d to account %d\n",
			args->thread_num, amount, init_account, dest_account);

		args->bank->accounts[init_account] = i;
		if(args->delay) usleep(args->delay);

		args->bank->accounts[dest_account] = j;
		if(args->delay) usleep(args->delay);

		args->transf_total += amount;

        pthread_cond_broadcast(args->bank->cond[dest_account]);
		pthread_mutex_unlock(args->bank->mutex[init_account]);
		pthread_mutex_unlock(args->bank->mutex[dest_account]);

		break;
		}	
	}
	return NULL;
}

// start opt.num_threads threads running on deposit.
struct thread_info *start_threads(struct options opt, struct bank *bank)
{
    int i,num_threadsX;
    struct thread_info *threads;

    num_threadsX = opt.num_threads * 2;
    printf("creating %d threads\n", opt.num_threads * 2);
    threads = malloc(sizeof(struct thread_info) * num_threadsX);

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running swap()
    for (i = 0; i < opt.num_threads * 2; i++) {
        threads[i].args = malloc(sizeof(struct args));

        threads[i].args -> thread_num = i;
        threads[i].args -> net_total  = 0;
		threads[i].args -> transf_total  = 0;
        threads[i].args -> bank       = bank;
        threads[i].args -> delay      = opt.delay;
        threads[i].args -> iterations = opt.iterations / opt.num_threads ;
        
        if (0 != pthread_create(&threads[i].id, NULL, deposit, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    for(i = opt.num_threads ; i < num_threadsX; i++){
		threads[i].args = malloc(sizeof(struct args));

		threads[i].args -> thread_num = i;
		threads[i].args -> net_total  = 0;
		threads[i].args -> transf_total  = 0;
		threads[i].args -> bank       = bank;
		threads[i].args -> delay      = opt.delay;
		threads[i].args -> iterations = opt.iterations / opt.num_threads;

        
		if (0 != pthread_create(&threads[i].id, NULL, transfer, threads[i].args)) {
			printf("Could not create thread #%d", i);
			exit(1);
		}
	}

    return threads;
}

struct thread_info *start_threads_transfer(struct options opt, struct bank *bank){
    int i,num_threadsX;
    struct thread_info *threads;

    num_threadsX = opt.num_threads * 2;
    threads = malloc(sizeof(struct thread_info) * num_threadsX);
    

    // Create num_thread threads running swap()
    for (i = 0; i < opt.num_threads * 2; i++) {

        if (0 != pthread_create(&threads[i].id, NULL, deposit, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    for(i = opt.num_threads * 2; i < num_threadsX; i++){

		if (0 != pthread_create(&threads[i].id, NULL, transfer, threads[i].args)) {
			printf("Could not create thread #%d", i);
			exit(1);
		}
	}

    return threads;


}

void *total(void *ptr)
{
    struct args *args =  ptr;	
	int i, out = 1, total;

    while(out) {
        total = 0;

        //Bloqueo de mutex de las cuentas
        for (i = 0; i < args->bank->num_accounts; i++) {        
            pthread_mutex_lock(args->bank->mutex[i]);
        }

         //Cuenta el saldo y libera el recurso
        for (i = 0; i < args->bank->num_accounts; i++) {       
            total += args->bank->accounts[i];
            pthread_mutex_unlock(args->bank->mutex[i]);
        }


        args->net_total = total;
        printf("Total amount : %d \n", args->net_total);

        if (args->delay) usleep(args->delay);

       // if(total==0) out=0;
    }
    return NULL;
}

struct thread_info *start_threads_check(struct options opt, struct bank *bank)
{
    struct thread_info *thread;

    printf("creating %d thread\n", 1);
    thread = malloc(sizeof(struct thread_info));

    if (thread == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running swap()
        thread->args = malloc(sizeof(struct args));
        thread->args -> net_total  = 0;
        thread->args -> bank       = bank;
        thread->args -> delay      = opt.delay;
        thread->args -> iterations = opt.iterations;

        if (0 != pthread_create(&thread->id, NULL, total, thread->args)) {
            printf("Could not create thread #%d", 1);
            exit(1);
        }

    return thread;
}



// Print the final balances of accounts and threads
void print_balances(struct bank *bank, struct thread_info *thrs, int num_threads) {
    int total_deposits=0, bank_total=0;
    printf("\nNet deposits by thread\n");

    for(int i=0; i < num_threads; i++) {
        printf("%d: %d\n", i, thrs[i].args->net_total);
        total_deposits += thrs[i].args->net_total;
    }
    printf("Total: %d\n", total_deposits);

    printf("\nAccount balance\n");
    for(int i=0; i < bank->num_accounts; i++) {
        printf("%d: %d\n", i, bank->accounts[i]);
        bank_total += bank->accounts[i];
    }
    printf("Total: %d\n", bank_total);
}

// wait for all threads to finish, print totals, and free memory
void wait(struct options opt, struct bank *bank, struct thread_info *threads, int optss) {
    // Wait for the threads to finish
    int num_threadsX = opt.num_threads * 2;
	
	for (int i = 0; i < num_threadsX; i++)
		pthread_join(threads[i].id, NULL);

	bank->fin = 1;
    
	

	print_balances(bank, threads, num_threadsX);

	for (int i = 0; i < num_threadsX; i++)
		free(threads[i].args);
    if(optss != 0){
        for (int i = 0; i < opt.num_accounts; i++) {
            pthread_mutex_destroy(bank->mutex[i]);
            free(bank->mutex[i]);		//Liberamos los mutex
        }
	free(bank->mutex);
    }
	free(bank->accounts);
	free(threads);
}

// allocate memory, and set all accounts to 0
void init_accounts(struct bank *bank, int num_accounts) {

    pthread_mutex_t ** mutexL;

    bank->num_accounts = num_accounts;
    bank->accounts     = malloc(bank->num_accounts * sizeof(int));
    mutexL = malloc(sizeof(pthread_mutex_t *) * (bank->num_accounts)); //Malloc array
    
    if(mutexL == NULL){
		printf("No hay suficiente memoria.\n");
		exit(1);
	}

    for(int i=0; i < bank->num_accounts; i++){
        bank->accounts[i] = 0;
        mutexL[i] = malloc(sizeof(pthread_mutex_t)); //Hacemos un malloc para cada mutex
		if(mutexL[i]==NULL){
			printf("No hay suficiente memoria.\n");
			exit(1);
		}
        pthread_mutex_init(mutexL[i], NULL);
    }
    
    bank->mutex = mutexL;
    bank->fin = 0;

}

int main (int argc, char **argv)
{
    struct options      opt;
    struct bank         bank;
    struct thread_info *thrs,*thr,*thrcheck;

    srand(time(NULL));

    // Default values for the options
    opt.num_threads  = 5;
    opt.num_accounts = 10;
    opt.iterations   = 100;
    opt.delay        = 10;

    read_options(argc, argv, &opt);

    init_accounts(&bank, opt.num_accounts);

    //Liberamos memoria sin borrar bank
    thrs = start_threads(opt, &bank);
    thrcheck = start_threads_check(opt, &bank);
    wait(opt,&bank,thrs,0);


    free(thrcheck->args);
    free(thrcheck);

    return 0;
}