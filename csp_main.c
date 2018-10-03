//Environment for Communicating Sequential Processes (CSP), solving the problem of producers and consumers

#include "csp_theory.h"
#include <pthread.h>


typedef struct{
	csp_ctxt *cc;
	int chan;
	int msg_size;
	int nofprod;
} cons_prod_arg;

typedef struct{
	csp_ctxt *cc;
	int nofcons, nofprod;
	int msg_size, capacity;
} buf_arg;

void *buf_func(void *args){
	buf_arg *arg=(buf_arg *)args;
	int capacity=arg->capacity;
	int nofcons=arg->nofcons;
	int nofprod=arg->nofprod;
	int msg_size=arg->msg_size;
	csp_ctxt *cc=arg->cc;
	int n=0, in=0, out=0, channel, i, want;
	int chanP[nofprod], chanC[nofcons];
	char buffer[capacity][msg_size];
	char msg[msg_size];
	
	//separating the channels
	for(i=0; i<nofprod; i++){
		chanP[i]=i;
	}
	for(i=0; i<nofcons; i++){
		chanC[i]=nofprod+i;
	}
	
	while(1){
		//buffer filling
		if(n<capacity){
			want=R;
			channel=csp_wait(cc, chanP, nofprod, want);
			csp_recv(cc, channel, msg);
			printf("Buffer   \treceived\t'%s'\t(from producer %d)\n", msg, channel+1);
			strcpy(buffer[in], msg);
			in=(in+1)%capacity;
			n++;
		}
		
		//buffer emptying
		if(n>0){
			want=S;
			channel=csp_wait(cc, chanC, nofcons, want);
			printf("Buffer    \tsending \t'%s'\t(to consumer %d)\n", msg, channel-nofprod+1);
			strcpy(msg, buffer[out]);
			csp_send(cc, channel, msg);
			out=(out+1)%capacity;
			n--;
		}
	}
}


void *prod_func(void *args){
	cons_prod_arg *arg=(cons_prod_arg *)args;
	int channel=arg->chan;
	int msg_size=arg->msg_size;
	csp_ctxt *cc=arg->cc;
	char msg[msg_size];
	int i;
	
	srand(time(NULL));
	
	while(1){
		//producing random message
		for(i=0; i<msg_size-1; i++){
			msg[i]= 'A'+rand()%26;
		}
		msg[msg_size-1]='\0';
		printf("Producer %d\tsending \t'%s'\n", channel+1, msg);
		csp_send(cc, channel, msg);
		sleep(1);
	}
}

void *cons_func(void *args){
	cons_prod_arg *arg=(cons_prod_arg *)args;
	int channel=arg->chan;
	int msg_size=arg->msg_size;
	int nofprod=arg->nofprod;
	csp_ctxt *cc=arg->cc;
	char msg[msg_size];
	
	while(1){
		//receiving message
		csp_recv(cc, channel, msg);
		printf("Consumer %d\treceived\t'%s'\n", channel-nofprod+1, msg);
		sleep(1);
	}
}

int main(int argc, char *argv[]){
	int nofcons, nofprod, nofchans, i, capacity, msg_size, *arrayC, *arrayP;
	pthread_t *consumers, *producers, buffer;
	csp_ctxt *cc;
	buf_arg buf;
	cons_prod_arg *argument;
	
	printf("Give number of producers: ");
	scanf("%d", &nofprod);
	printf("Give number of consumers: ");
	scanf("%d", &nofcons);
	printf("Give buffer capacity: ");
	scanf("%d", &capacity);
	printf("Give size of message: ");
	scanf("%d", &msg_size);
	msg_size++; //to make space for '\0'
	printf("\n");
		
	//initializations
	nofchans=nofprod+nofcons;
	csp_init(cc, nofchans, msg_size);
	
	argument=(cons_prod_arg *)malloc(sizeof(cons_prod_arg)*nofchans);
    if(argument == NULL){
        perror("Malloc for argument");
        exit(1);
    }
	arrayC=(int *)malloc(sizeof(int)*nofcons);
    if(arrayC == NULL){
        perror("Malloc for arrayC");
        exit(1);
    }
	arrayP=(int *)malloc(sizeof(int)*nofprod);
    if(arrayP == NULL){
        perror("Malloc for arrayP");
        exit(1);
    }
	
	consumers=(pthread_t *)malloc(sizeof(pthread_t)*nofcons);
    if(consumers == NULL){
        perror("Malloc for consumers");
        exit(1);
    }
	producers=(pthread_t *)malloc(sizeof(pthread_t)*nofprod);
    if(producers == NULL){
        perror("Malloc for producers");
        exit(1);
    }
	
	buf.cc=cc;
	buf.capacity=capacity;
	buf.msg_size=msg_size;
	buf.nofcons=nofcons;
	buf.nofprod=nofprod;
	
	//creates buffer
	if(pthread_create(&buffer, NULL, buf_func, &buf)){
		perror("\nCreating thread for buffer");
		return 1;
	}
	
	//creating consumers and producers
	for(i=0; i<nofprod; i++){
		argument[i].cc=cc;
		argument[i].chan=i;
		argument[i].msg_size=msg_size;
		if(pthread_create(&producers[i], NULL, prod_func, &argument[i])){
            perror("\nCreating thread for producer");
            return 1;
        }
	}
	for(i=0; i<nofcons; i++){
		argument[i+nofprod].cc=cc;
		argument[i+nofprod].chan=i+nofprod;
		argument[i+nofprod].msg_size=msg_size;
		argument[i+nofprod].nofprod=nofprod;
		if(pthread_create(&consumers[i], NULL, cons_func, &argument[i+nofprod])){
            perror("\nCreating thread for consumer");
            return 1;
        }
	}
	
	
	//end of the program
	pthread_join(buffer, NULL);
	for(i=0; i<nofprod; i++){
		pthread_join(producers[i], NULL);
	}
	for(i=0; i<nofcons; i++){
		pthread_join(consumers[i], NULL);
	}
	pthread_mutex_destroy(&cc->monitor);
	for(i=0; i<nofchans; i++){
		pthread_cond_destroy(&cc->receive[i]);
		pthread_cond_destroy(&cc->send[i]);
	}
	
	free(cc->receive);
	free(cc->send);
	free(cc->check_recv);
	free(cc->check_snd);
	for(i=0; i<nofchans; i++){
		free(cc->channels_buf[i]);
	}
	free(cc->channels_buf);
	free(argument);
	free(arrayC);
	free(arrayP);
	free(consumers);
	free(producers);
	
	return 0;
}
