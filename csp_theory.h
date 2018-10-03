//Library for CSP environment

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define T 1
#define F 0
#define S 2
#define R 3

typedef struct{ 
  pthread_mutex_t monitor;
  pthread_cond_t *receive;
  pthread_cond_t *send;
  int *check_recv, *check_snd;
  char **channels_buf;
} csp_ctxt;

void csp_init(csp_ctxt *cc, int nofchans, int msg_size){
	int i;
	
	pthread_mutex_init(&cc->monitor, NULL);
	
	cc->receive=(pthread_cond_t *)malloc(sizeof(pthread_cond_t)*nofchans);
    if(cc->receive == NULL){
        perror("Malloc for condition");
        exit(1);
    }
	cc->send=(pthread_cond_t *)malloc(sizeof(pthread_cond_t)*nofchans);
    if(cc->send == NULL){
        perror("Malloc for condition");
        exit(1);
    }
	
	for(i=0; i<nofchans; i++){
		if(pthread_cond_init(&cc->receive[i], NULL) != 0){
            perror("Condition init failed\n");
            exit(1);
        }
		if(pthread_cond_init(&cc->send[i], NULL) != 0){
            perror("Condition init failed\n");
            exit(1);
        }
	}
		
	cc->check_recv=(int *)malloc(sizeof(int)*nofchans);
    if(cc->check_recv == NULL){
        perror("Malloc for check_recv");
        exit(1);
    }
	cc->check_snd=(int *)malloc(sizeof(int)*nofchans);
    if(cc->check_snd == NULL){
        perror("Malloc for check_snd");
        exit(1);
    }
	
	cc->channels_buf=(char **)malloc(sizeof(char*)*nofchans);
    if(cc->channels_buf == NULL){
        perror("Malloc for channels_buf");
        exit(1);
    }
	for(i=0; i<nofchans; i++){
		cc->check_recv[i]=F;
		cc->check_snd[i]=F;
		cc->channels_buf[i]=(char *)malloc(sizeof(char)*msg_size);
        if(cc->channels_buf[i] == NULL){
            perror("Malloc for channels_buf");
            exit(1);
        }
	}
}



int csp_send(csp_ctxt *cc, int chan, char *msg){
	pthread_mutex_lock(&cc->monitor);
	
	//Put message on channel
	strcpy(cc->channels_buf[chan], msg);
	cc->check_snd[chan]=T;
	//signal the channel to receive it
	pthread_cond_signal(&cc->receive[chan]);
	
	//waits for signal from the channel that receives his message
	while(cc->check_recv[chan] == F){
		pthread_cond_wait(&cc->send[chan], &cc->monitor);
	}
	cc->check_recv[chan]=F;
	
	pthread_mutex_unlock(&cc->monitor);
	return 0;
}

int csp_recv(csp_ctxt *cc, int chan, char *msg){
	pthread_mutex_lock(&cc->monitor);

	//waits until there is a message on channel
	while(cc->check_snd[chan] == F){
		pthread_cond_wait(&cc->receive[chan], &cc->monitor);
	}
	cc->check_snd[chan]=F;
	
	//Get message from channel
	strcpy(msg, cc->channels_buf[chan]);
	strcpy(cc->channels_buf[chan], "\0");
	cc->check_recv[chan]=T;
	
	//send signal to the sender that receiving is over
	pthread_cond_signal(&cc->send[chan]);
	
	pthread_mutex_unlock(&cc->monitor);
	return 0;
}

int csp_wait(csp_ctxt *cc, int chans[], int len, int want){
	int channel, i, counter;
	
	pthread_mutex_lock(&cc->monitor);
	
	srand(time(NULL));
	i = rand()%len;
	counter = 0;
	
	//fair choice of channel depending on random check of channels
	while(1){
		counter++;
		//checks for ready channel
		if(((strcmp(cc->channels_buf[chans[i]],"\0") != 0) && want == R) ||
            ((strcmp(cc->channels_buf[chans[i]],"\0") == 0) && want == S)){            
			channel=chans[i];
			break;
		}
		if(i == len-1){
			i=0;
		}
		else{
			i++;
		}
		
		//if there is not a ready channel, the choice is random
		if(counter == len){
			channel=chans[i];
			break;
		}
	}
	
	pthread_mutex_unlock(&cc->monitor);
	return channel;
}
