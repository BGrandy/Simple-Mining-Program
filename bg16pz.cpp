#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>

int numOfLeadingZeros;
static const int NUMTHREADS=4;
volatile bool continuing;
volatile int occupied;
pthread_mutex_t lock; //Our mutual exclusion lock

void printThirtyTwo(unsigned int word) {
        for (int i=0;i<32;i++)
            std::cout<<(((0x80000000>>i)&word)?1:0);
    std::cout<<std::endl;
}

void printSixtyFour(unsigned long word) {
    for (int i=0;i<64;i++)
        std::cout<<(((0x8000000000000000>>i)&word)?1:0);
    std::cout<<std::endl;
}

//Breaks nonce up into 16 4-bit tokens, to generate hash value
unsigned int calchash(unsigned long nonce) {
unsigned int hash=0;
    for (int i=15;i>=0;i--) {
        hash=hash*17+((nonce>>(4*i))&0x0F);
    }
return hash;
}

void genULong(unsigned long &nonce) {
    nonce=0;
    for (int i=63;i>=0;i--) {
        nonce<<=1;
        nonce|=random()%2;
    }
}

int leadingZeroes(unsigned int value) {
    for (int i=0;i<32;i++)
        if ((value>>(31-i))&1) return i;

return 32;
}

void *busywork(void *unnecessary) {

    unsigned long nonce;
    unsigned int hash;

    while (continuing) {
        genULong(nonce);
        hash = calchash(nonce);
        if(leadingZeroes(hash) == numOfLeadingZeros){
            pthread_mutex_lock(&lock);
            occupied--;
            std::cout << "Nonce: " << nonce; 
            std::cout << " | Hash: ";
            printThirtyTwo(hash);
            pthread_mutex_unlock(&lock);
        }
    }
    pthread_exit(NULL);
}

void peek(int sig) {
    std::cout<<"Currently processing: "<<(continuing?"Yes":"No")<<std::endl;
}

void interrupted(int sig) {
    std::cout<<"\nComputations complete.\nHalting now..."<<std::endl;
    continuing=false;
}

int main() {

    srand(time(NULL));

    pthread_t ct[NUMTHREADS];//our child threads
    continuing=true;
    //std::cout<<"About to commence; PID: "<<getpid()<<std::endl;

    int choice = -1;
    while (true) {

        if (signal(SIGINT,interrupted)==SIG_ERR) {
            std::cout<<"Unable to change signal handler."<<std::endl;
            return 1;
        }

        if (signal(SIGUSR1,peek)==SIG_ERR) {
            std::cout<<"Unable to change signal handler."<<std::endl;
            return 1;
        }
        
        std::cout << "(0) QUIT | (1) Select a number of leading zeros for nonces: ";
        std::cin >> choice;
        switch(choice){
            case 0:
                return 0;
            case 1:
                continuing = true;
                std::cout << "Number of leading zeros: ";
                std::cin >> numOfLeadingZeros;
                for (int i=0;i<NUMTHREADS;i++) {
                    pthread_mutex_lock(&lock);//reserve lock
                    pthread_create(&ct[i], NULL, &busywork, NULL);
                    occupied++;
                    pthread_mutex_unlock(&lock);//release lock
                }
                while(continuing) sleep(1);
                break;
            default:
                std::cout << "Please Select one of the provided options." << std::endl;
            }
    }

    while (occupied>0)
        sleep(1);
    std::cout<<"Execution complete."<<std::endl;
}
