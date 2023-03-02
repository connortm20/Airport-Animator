/*
Connor Marshall
Computer Systems Project 2
AIRPORT TOUR SIMULATION
10-6-22
*/

#include "AirportAnimator.hpp"

#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <sys/sem.h>

using namespace std;

#define SEMSET_SIZE 3
#define NUM_OPS 1
#define SEM_KEY 12345


int avalPassengers, totalTours, sem_id, toursStarted=0, toursComplete=0;
struct sembuf semWaitCommand[1];
struct sembuf semSigCommand[1];
struct sembuf passWaitCommand[1];
struct sembuf passSigCommand[1];
struct sembuf tourWaitCommand[1];
struct sembuf tourSigCommand[1];


pthread_t threadData[8];
void* planeThread(void *args)
{
    int *planeID = (int *)args; //stores plane ID to own integer pointer

while(toursStarted < totalTours)
    {   
        AirportAnimator::updateStatus(*planeID, "BOARD");
        //deadlock avoidence on passenger count
        semop(sem_id,passWaitCommand,NUM_OPS);
        if(avalPassengers>=12)
        {
            avalPassengers -= 12;

            //semaphore to prevent overstarting tours
            semop(sem_id,tourWaitCommand,NUM_OPS);
            if(toursStarted < totalTours)
            {
                toursStarted++;
                semop(sem_id, tourSigCommand, NUM_OPS);
            }
            else
            {
                avalPassengers+=12;
                semop(sem_id, tourSigCommand, NUM_OPS);
                semop(sem_id, passSigCommand, NUM_OPS);
                continue;
            }  

            semop(sem_id, passSigCommand, NUM_OPS);
        }
        else
        {
            semop(sem_id, passSigCommand,NUM_OPS);
            continue;
        }

        //boarding process
        for(int pass=1; pass<=12; pass++)
        {
            AirportAnimator::updatePassengers(*planeID, pass);
            sleep(rand()%3);
        }
        //taxi to runway
        AirportAnimator::updateStatus(*planeID, "TAXI");
        AirportAnimator::taxiOut(*planeID);

        //attempt to takeoff
        semop(sem_id, semWaitCommand, NUM_OPS);

            AirportAnimator::updateStatus(*planeID, "TKOFF");
            AirportAnimator::takeoff(*planeID);
            sleep(2);

        semop(sem_id, semSigCommand, NUM_OPS);

        //start tour time
        AirportAnimator::updateStatus(*planeID,"TOUR");
        sleep(15+rand()%31);

        //request land access
        AirportAnimator::updateStatus(*planeID,"LNDRQ");
        semop(sem_id, semWaitCommand, NUM_OPS);

            AirportAnimator::updateStatus(*planeID,"LAND");
            AirportAnimator::land(*planeID);
            sleep(2);

        semop(sem_id, semSigCommand, NUM_OPS);

        //taxi back to gate
        AirportAnimator::updateStatus(*planeID, "TAXI");
        AirportAnimator::taxiIn(*planeID);

        //unload passengers
        AirportAnimator::updateStatus(*planeID, "DEPLN");
        for(int passenger=11; passenger>=0; passenger--)
            {
            AirportAnimator::updatePassengers(*planeID, passenger);
            avalPassengers++;//only ever increases passegengers so no need for semaphore
            sleep(1);
            }

        //update tours
        toursComplete++;
        AirportAnimator::updateTours(toursComplete);
    }

    AirportAnimator::updateStatus(*planeID, "CLOSED");
    return NULL;
}

int main(int argc, char *argv[])
{
    //request parameters from users if not provided
    if(argc==3)
    {
        avalPassengers = atoi(argv[1]);
        totalTours = atoi(argv[2]);
    }
    else
    {
        cout << "Enter Total Passengers : ";
        cin >> avalPassengers;
        
        cout << "Enter Total Number of Tours : ";
        cin >> totalTours;
    }

    //checks passenger count is >= 12 so at least 1 plane can complete tours
    if(avalPassengers<12)
    {
        cerr << "PASSENGER VALUE MUST BE GREATER THAN 11 FOR 1 PLANE TO TAKEOFF" << endl << "PROGRAM CANCELED" << endl;
        return 1;
    }

    //create runway semaphore
    sem_id = semget(SEM_KEY, SEMSET_SIZE, 0);
    if (sem_id != -1) // likely semaphore set does not exist ...
    {
    semctl(sem_id,0,IPC_RMID, 1);
    }
    
    
    sem_id = semget(SEM_KEY, SEMSET_SIZE, IPC_CREAT|IPC_EXCL|0600);
    
    // initialize the semaphore ...
    semctl(sem_id,0,SETVAL, 1);
    semctl(sem_id,1,SETVAL, 1);
    semctl(sem_id,2,SETVAL, 1);
    
    //define semaphore operations
    //runway semwait 
    semWaitCommand[0].sem_num = 0;
    semWaitCommand[0].sem_op = -1;
    semWaitCommand[0].sem_flg = 0;
    //runway semsignal  
    semSigCommand[0].sem_num = 0;
    semSigCommand[0].sem_op = +1;
    semSigCommand[0].sem_flg = 0;  
    //passgenr semwait  
    passWaitCommand[0].sem_num = 1;
    passWaitCommand[0].sem_op = -1;
    passWaitCommand[0].sem_flg = 0;
    //passenger semsignal  
    passSigCommand[0].sem_num = 1;
    passSigCommand[0].sem_op = +1;
    passSigCommand[0].sem_flg = 0;
    //tour semwait
    tourWaitCommand[0].sem_num = 2;
    tourWaitCommand[0].sem_op = -1;
    tourWaitCommand[0].sem_flg = 0;
    //tour semsignal
    tourSigCommand[0].sem_num = 2;
    tourSigCommand[0].sem_op = +1;
    tourSigCommand[0].sem_flg = 0;


    AirportAnimator::init();

    //creates 8 threads with distinct ids to represent each plane
    for(int pid=0; pid<8;pid++)
    {
        int *data = new int(pid);
        pthread_create(&threadData[pid], NULL, planeThread, (void *)data);
    }



    //wait for all threads to complete
    for(int t=0; t < 8; t++)
        pthread_join(threadData[t], NULL);

    sleep(2);//pause the screen to show completion
    AirportAnimator::end(); //close and clean Animator
    semctl(sem_id,0,IPC_RMID, 1); //remove semaphore
    cout << "Airport Simulation Completed" << endl;
    return 0;
}