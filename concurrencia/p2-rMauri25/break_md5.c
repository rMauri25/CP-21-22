#include <sys/types.h>
#include <openssl/md5.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>


#define PASS_LEN 6                   
#define NUM_THREADS 5                


struct shared_info {
    pthread_mutex_t mutex;
    long   globl_iters;              
    bool   is_password_broken;       
    long   max_iters;                
    char * keys;                     
    int    num_keys;                 
    int    broken_pass;              
    long comprobados;
    long comprobadosSeg;
    long aux;
};

struct args {
    int                  thread_num; 
    unsigned char      * pass;      
    struct shared_info * shared;     
};

struct thread_info {
    pthread_t     id;                
    struct args * args;            
};

long ipow(long base, int exp) {
    long res = 1;
    for(;;) {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }
    return res;
}

long pass_to_long(char *str) {
    long res = 0;

    for(int i=0; i < PASS_LEN; i++)
        res = res * 26 + str[i]-'a';

    return res;
}

void long_to_pass(long n, unsigned char *str) {  
    for(int i=PASS_LEN-1; i >= 0; i--) {
        str[i] = n % 26 + 'a';
        n /= 26;
    }
    str[PASS_LEN] = '\0';
}

void to_hex(unsigned char *res, char *hex_res) {
    for(int i=0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(&hex_res[i*2], 3, "%.2hhx", res[i]);
    }
    hex_res[MD5_DIGEST_LENGTH * 2] = '\0';
}

void *progress_bar(void *ptr) {
    
    struct args *args = ptr;
    char s[51] = "##################################################";

    int start = 0;
    int sharps = 50;
    
    while(args->shared->globl_iters < args->shared->max_iters && args->shared->is_password_broken == false) {
        if(args->shared->is_password_broken) break;
        printf("%d / %d ---> %d", args->shared->num_keys, args->shared->broken_pass,args->shared->is_password_broken);
        printf("\r%.*s[%0.2f%%] || %ld b/s" , (int)(((float)args->shared->globl_iters/args->shared->max_iters)*100)/2, s,
                                  ((float)args->shared->globl_iters)/(args->shared->max_iters)*100,args->shared->comprobadosSeg);
        fflush(stdout);
    }
    return NULL;
}

void * ComprobadosSeg(void *ptr){
    struct args* args= ptr;
    clock_t start;

    while(1){
        start=clock();

        while(clock()-start==1){
            pthread_mutex_lock(&args->shared->mutex);
            args->shared->aux=args->shared->comprobadosSeg;
            args->shared->comprobadosSeg=0;
            args->shared->aux=0;
            pthread_mutex_unlock(&args->shared->mutex);

        } 
        if(args->shared->is_password_broken) break; 

        }
        return NULL;
}


void *break_pass(void *ptr) {
    struct args *args = ptr;
    unsigned char res[MD5_DIGEST_LENGTH];
    char hex_res[MD5_DIGEST_LENGTH * 2 + 1];
    int max=100;
    long bound = ipow(26, PASS_LEN);
    long parte=bound/max;


    for(long i = args->thread_num; i < args->shared->max_iters; i+=NUM_THREADS-1) {
        if(args->shared->is_password_broken){            
             break; // If is_password_broken is true, exit each thread
        }
        long_to_pass(i, args->pass);
        MD5(args->pass, PASS_LEN, res);
        to_hex(res, hex_res);

      
        if(args->shared->globl_iters < i) args->shared->globl_iters = i;

     
        for(int i = 0; i < args->shared->num_keys*33; i+=33) {
            if(strcmp(hex_res, args->shared->keys+i) == 0) {
                printf("\nPassword found by thread #%d -> %s = %s\n",
                       args->thread_num+1, args->shared->keys+i, args->pass);                
                args->shared->broken_pass++;
                if(args->shared->broken_pass >= args->shared->num_keys){
                    args->shared->is_password_broken = true;                    
                    break;
                }
            }
            pthread_mutex_lock(&args->shared->mutex);
            args->shared->comprobadosSeg++;
            args->shared->comprobados+=(parte);
            pthread_mutex_unlock(&args->shared->mutex);
        }
    }
    return NULL;
}


void init_shared(int argc, char * argv[], struct shared_info * shared) {
    shared->is_password_broken = false;
    shared->max_iters = ipow(26, PASS_LEN);
    shared->globl_iters = 0;
    shared->num_keys = argc-1;
    shared->broken_pass = 0;
    shared->keys = malloc(shared->num_keys * (sizeof(char)*33));
    shared->aux=0;
    shared->comprobadosSeg=0;
    shared->comprobados=0;
    pthread_mutex_init(&shared->mutex, NULL);

    int j = 1;
    for(int i = 0; i < shared->num_keys*33; i+=33) {
        strcpy(&shared->keys[i], argv[j]);
        strcat(&shared->keys[i], "\0");
        j++;
    }
}
struct thread_info *start_threads(struct shared_info * shared) {
    struct thread_info *threads;

    printf("Creating %d threads to break %d password(s)\n", NUM_THREADS, shared->num_keys);
    threads = malloc(sizeof(struct thread_info)*(NUM_THREADS));
    
    for(int i = 0; i < NUM_THREADS; i++) {
        threads[i].args               = malloc(sizeof(struct args));
        threads[i].args -> pass       = malloc(sizeof(char) * PASS_LEN);
        threads[i].args -> thread_num = i;
        threads[i].args -> shared     = shared;

        if( 0 < i || i > 3) {            
            if (0 != pthread_create(&threads[i].id, NULL,break_pass , threads[i].args)) {
                printf("Could not create thread #%d", i);
                exit(1);
            }
        } if(i == 3){   
                 
            if(0 != pthread_create(&threads[i].id, NULL,progress_bar , threads[i].args)){
                printf("Could not create thread #%d", i);	//Mensaje de error
                exit(1);									//Salida
            }              
    
        } else {                    
            if (0 != pthread_create(&threads[i].id, NULL,ComprobadosSeg , threads[i].args)) {
                printf("Could not create thread #%d", i);
                exit(1);
            }            
            
        } 
    }
    return threads;
}
void wait(struct thread_info *threads, struct shared_info *shared) {
    // Wait for the threads to finish
    for(int i = 0; i < NUM_THREADS; i++)
        if(0 != pthread_join(threads[i].id, NULL)) {
            printf("Cannot join thread #%d\n", threads[i].args->thread_num+1);
        }
    
    for(int i = 0; i < NUM_THREADS; i++) {
        free(threads[i].args->pass);
        free(threads[i].args);
    }
    
    free(shared->keys);
    pthread_mutex_destroy(&shared->mutex);
    free(threads);
}

int main(int argc, char *argv[]) {
    struct thread_info *threads;
    struct shared_info shared;
    time_t start, end;
    
    time(&start);

    if(argc < 2) {
        printf("Use: %s string\n", argv[0]);
        exit(0);
    }

    init_shared(argc, argv, &shared);
    threads = start_threads(&shared);
    wait(threads, &shared);
    time(&end);
    printf("\nPassword(s) broken in %ld seconds\n", end-start);
    return 0;
}