/*
	created by Haowei Zhu on 2019.2.5
	

*/
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#ifdef __unix__
# include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * (x))
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include "fun.h"
#include <string.h>
#pragma comment(lib,"x86/pthreadVC2.lib")
#pragma comment(lib, "User32.lib")
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)

/*Global variables*/
int mode=1;	//mode 1: operating, mode 0: pausing, mode 2: stopping and waiting to exit
int cycle=1;
int x=0;
int active=0;
int rate;	//rate of monitoring
int rate2;	//gap between each cycle
int current=0;	//current device that was being checked
int pause=0;
int next=0;	//
char msg[10][100];
int msgp=0;
char msg2 [100];
int point=0;
int config=1;

/* temperature contrains */
int high=999;
int low=-999;



// add Mutex for data modifying
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

/* data structures*/
struct Device{
	char name[33];	//limit the name to be 32 digits
	int tempup;	//upper temperature threshold 
	int templow;//lower temperature 
	/*
	more features can be added here
	*/
};

/* Data that storing in the system*/
struct para{
	int device;
	int live;	//live result (in second)
	int fault;	//temperature status: -1 unknown, 0 temperature fault, 1 temperature working fine
	
	
	/* 
	
	
	can implement more variables here for other features such as internet speed
	
	
	*/
};

struct Device list[10];	//create an array for the device

/* headers*/
int tempscan(int x);
void clrscr();	//clear screen function
void printstatus(int a,int b, int c);
int check(char c);
void printprogress(int now);
void welcome();
void logo();
void buttommsg();
void errormsg(int a);
void writemsg(int device, int good,int type);
void msgshiftup();
void storemsg();
void lock();
void unlock();



void* UI(void* data) {
	
	struct para *result=(struct para*)data;	//load input data	
	
	//while loop to display UI during the operation
	while (mode!=2){
		
		/* Normal UI for the programme */
		clrscr();
		logo();
		
		//check out which device will be checked next
		if (current == (active-1)){
			printprogress(result[0].device);
//			printf("next is %d",result[0].device);
		}else{
			printprogress(result[current+1].device);
//			printf("next is %d",result[current+1].device);
		}
		
		printf("Devices current status:\n");
		for (int j=0;j<active;j++){
	
			printstatus(result[j].device,result[j].fault,result[j].live);	//print the status out		

		}
		buttommsg();
		
		
		/*Print out the latest 10 alert messages here */
		
		printf("\nImportant messages(latest 10 if more than 10, from latest to oldest. All messages were saved to log.txt):\n");
		for (int j=(point-1);j>=0;j--){
			printf("%s\n",msg[j]);
			
		}		
		
		/* UI refreshing rate is 1 second*/
		sleep(1);	
	}
	
	/*Display for stopping the programme*/
	clrscr();
	logo();
	printprogress(2);
	
	pthread_exit(NULL); 	//exit the thread
}


void* count(void * data){
	struct para *result=(struct para*)data;	//input data
	while(mode!=2){
		for (int j=0;j<active;j++){
			lock();
			result[j].live++;	//update the time for 1 second
			unlock();
		}
		sleep(1);
	}
	pthread_exit(NULL);
}

void* counter(void * data){
	sleep(rate);
	pthread_exit(NULL); 	//exit the thread
}


void* read(void* data){		//the thread that use for comparing the temperature of the device and the limits
	struct para *result=(struct para*)data;	//input data
	int temp2=next;
	while (mode==1){
		
		for (int i=temp2;i<active;i++){
			
			
			pthread_t k; 	//use thread to time
			pthread_create(&k, NULL, counter, NULL);
			
			/* read temperature from device*/
			int temp=tempscan(result[i].device);	//read the temperature data from device

			
			//check if the temp is in the range
			
			lock();
			current=i;
			
			/*case switching to check temperature, check the original status first*/
			
			switch (result[i].fault){
				
				//if the status is unknown
				case -1:
					if (temp>=list[result[i].device-1].templow && temp<=list[result[i].device-1].tempup){	//check if the temp is inside the limits
						
						//if inside the limits
						result[i].fault=1;
					}else{
						
						//if outside
						result[i].fault=0;

						/* print error message to log file*/
						writemsg(result[i].device,0,1);
						
						/*store error message to print later*/
						storemsg();
	
					}
					break;
					
				//if its originally outside the limits
				case 0:
					if (temp>=list[result[i].device-1].templow && temp<=list[result[i].device-1].tempup){	//check if the temp is inside the limits
						
						//if inside the limits
						result[i].fault=1;
						
						/* print error message to log file*/
						writemsg(result[i].device,1,1);
						
						/*store error message to print later*/
						storemsg();
						
					}else{
						
						//if still outside
						
						//do nothing
						
					}				
				
				
					break;
				

				//if originally at working temperature
				case 1:
					if (temp>=list[result[i].device-1].templow && temp<=list[result[i].device-1].tempup){	//check if the temp is inside the limits
						
						//if still inside the limits
						
						//do nothing
					}else{
						
						//if changed to outside
						result[i].fault=0;
						
						/* print error message to log file*/
						writemsg(result[i].device,0,1);
						
						/*store error message to print later*/
						storemsg();
					}					
				
					break;
			

			}
			/*  end of the checking temperature process*/
			
			
			/* Can implement other features here such as checking memory usage */
			
			result[i].live=0;
			unlock();
			
			/* mathematic calculations when the programme is pause*/
			if (mode==0) {
//				printf("a\n");
				if ((current+1)==active){
					next=0;	//start from first device after resuming
				}else{
					next=current+1;	//start from next device after resuming
					lock();
					cycle--;
					unlock();
				} 
				break;
			}
			
			if (mode==2)break;
			
			
			pthread_join(k, NULL); 	//wait for the thread to finish counting down
//			sleep(rate);	//break time between each device
			
		}	
		if (mode==2)break;
		lock();
		cycle++;	//counting cycles
		unlock();
		
		temp2=0;
		sleep(rate2-rate);	//wait time gate between 2 cycles
	}	
	
	pthread_exit(NULL);
}



int main()
{
	
	//create variables
	int i;	
	char buffer[100];
	int temp;

	
	struct para result[10];
	for (i=0;i<10;i++){
		result[i].fault=-1;
		result[i].live=0;
		strcpy(msg[i],"hi");
		
	}
	
	pthread_t t; 	//first thread UI
	pthread_t t2; 	//second thread monitoring
	pthread_t t3;	//thread three for updating time
	
	welcome();
	
	/* start loading the device configs*/
	while (1){
		
		printf("Loading configurations...\n");
		sleep(1);

		FILE *fp=fopen("device.txt", "r");//open the txt file that contains the data of the devices

			if(NULL == fp){	//check if the file is not existing
			
				printf("the device file is not exist\n");
				
				/* implement a function to wait to reload the file here*/

			}
			
		/*

		
		implement a function here to check if 'device.txt' is a empty file
		if empty, wait and load again
		if not, continue
		

		
		*/
		
		/* if the file is exists and it isn't empty, then start scanning*/
		
		fscanf_s(fp,"%d",&x);	//how many devices we r entering
		
		/*check constrain*/
		if (x>10){
			printf("no more than 10 device\n");
		}else{
			
			/* implement the here to deal with the case if more than device in the system */
		}
		
		printf("There are %d devices details loaded\n" , x);

		/* read the temperature configurations for each device*/
		for (i=0;i<x;i++){	
			fscanf(fp,"%s",&list[i].name); 
			
			/* 
			
			a function need to be implement here to check the length of device name, which is no more than 32 digits
			
			
			*/
			
			fscanf_s(fp,"%d",&list[i].tempup);
			fscanf_s(fp,"%d",&list[i].templow);
			
			/*check constrains*/
			if (list[i].tempup>high){
				list[i].tempup=high;
			}
			if (list[i].tempup<low){
				list[i].tempup=low;
			}
			
			if (list[i].templow>high){
				list[i].templow=high;
			}
			if (list[i].templow<low){
				list[i].templow=low;
			}
			
			
			//swap for upper and lower limits
			if (list[i].templow>list[i].tempup){
				temp=list[i].templow;
				list[i].templow=list[i].tempup;
				list[i].tempup=temp;
				
			}
			/*
			
			if other features such as memory usage threshold is added, please add more fscan functions here to load the values

			*/
			printf("Device %d is: %s with upper temperature limit of %d and lower temperature limit of %d \n" , (i+1) , list[i].name, list[i].tempup,list[i].templow);
			
		}
		fclose(fp);
		

		/* wait to confirm the configs */
		while(1){
			printf("Please confirm the device configurations. Enter Y to continue or N to reload:");
			
			
			
			fgets(buffer, sizeof(buffer), stdin);
//			strcpy(buffer, readstring());


			if (buffer[0]=='Y'){
				break;
		
			}else if (buffer[0]=='N'){
				break;
				
			}else{
				errormsg(1);
//				printf("Invalid. Please enter again\n");
				continue;
			}
			
		}
		if (buffer[0]=='Y'){
			break;
		}
	}		
	
	/* end of device configurations part */
	
	clrscr();
	
	/* Load from config from local*/
	FILE *fp=fopen("config.txt", "r");//open the txt file that contains the rest of the config
	
	if(NULL == fp){	//check if the file is not existing
	
		printf("the config file is not exist\n");
		
		/* skip to enter the config manually */
		config=0;
		
	}else{
		
		/*
		
		a function need to be implement here to check if the file is empty
		
		*/
		
		
		/* how many device want to be monitored*/
		fscanf_s(fp,"%d",&active);
		
		/* what are their number*/
		for (i=0;i<active;i++){
			
			fscanf_s(fp,"%d",&result[i].device);
		}
		/* entering rate of monitoring */
		fscanf_s(fp,"%d",&rate);
		
		/* Entering gap between each cycle*/
		fscanf_s(fp,"%d",&rate2);
		
		fclose(fp);// close file

		/* Display configs that loaded from config.txt*/
		printf("Configs are showing here:\n");
		printf("Monitoring %d devices:",active);
		for (i=0;i<active;i++){
			printf("%d ",result[i].device);
		}
		printf("\nRate of monitoring is %d seconds\nGap between each cycle is %d second\n",rate,rate2);

		printf("Would you like to use this config or enter them manually? Enter Y to use or enter N to enter them manually:");

		while (1){
			scanf_s("%s",&buffer);
			if (buffer[0]=='Y'){
				config=1;
				break;
			}else if(buffer[0]=='N'){
				config=0;
				break;
				
			}else{
				errormsg(1);
		//			printf("Invalid. Please enter again\n");
				printf("Would you like to use this config or enter them manually? Enter Y to use or enter N to enter them manually:");
			}
		}		
		
		
	}

	/* start of entering rest of config*/
	
	
	switch(config){
		case 0:
		
			/*  Entering number of devices that want to be monitored */
			printf("Please enter the amount of device that would like to monitor:");
			scanf_s("%d",&active);
			while (active>x || active<=0){
				errormsg(1);
				// printf("Invalid. Please enter the number again.\n");
				printf("Please enter the amount of device that would like to monitor:");
				scanf_s("%d",&active);
			}
			
			int number[10];
			printf("Please enter the number # of device that would like to monitor in ascending order, one number at a time\n");
			for (i=0;i<active;i++){
				printf("Please enter the number # of device that would like to monitor and press enter:");
				
				scanf_s("%d",&temp);
				while (temp>x){
					errormsg(1);
					// printf("Invalid. Please enter the number again.\n");
					printf("Please enter the number # of device that would like to monitor and press enter:");
					scanf_s("%d",&temp);
				}
		//		number[i]=temp;
				result[i].device=temp;
			}
			
			/* entering rate of monitoring */
			while (1){
				printf("Please enter the rate of monitoring(by seconds),equal or greater than 1:");
				scanf_s("%d",&rate);
				if (rate>=1){
					break;
				}else{
					errormsg(1);
					// printf("Invalid. Please enter again.\n");
					continue;
				}			
			}
			
			/* Entering gap between each cycle*/
			while (1){
				printf("Please enter the the gap time between each cycle(by seconds), larger or equal to rate of monitoring:");
				scanf_s("%d",&rate2);
				if (rate2>=rate){
					break;
				}else{
					errormsg(1);
					// printf("Invalid. Please enter again.\n");
					continue;
				}				
			}
			/* end of manually entering configs*/
			
			/*option to save the configs locally*/
			printf("Would you like to save those configs so that be use in the future? Enter Y to save or enter N to skip");
			
			while (1){
				scanf_s("%s",&buffer);
				if (buffer[0]=='Y'){
				
					FILE * fp;	//open file
					fp = fopen ("config.txt","w");
					fprintf (fp, "%d\n",active);
					for (i=0;i<active;i++){
						fprintf (fp, "%d\n",result[i].device);
					}
					
					fprintf (fp, "%d\n",rate);
					fprintf(fp, "%d",rate2);
					
					fclose (fp); //close file
					break;
				}else if (buffer[0]=='N'){
					break;
					
				}else{
					errormsg(1);
//					printf("Invalid. Please enter again.\n");
					printf("Would you like to save those configs so that be use in the future? Enter Y to save or enter N to skip:");
					continue;
				}
			}
			
			
			break;
		case 1:
			break;
	}
	
	
	pthread_create(&t, NULL, UI, &result); 	//create thread to continuously print out the monitoring result
	pthread_create(&t2,NULL, read, &result);	//create thread to continuously check the temperature data
	pthread_create(&t3,NULL, count, &result);
	while (1){
		
		/* main function: checking key pressing */

		/*checking P Key*/
		if (check('P')==1){
			if (mode==1) mode=0;

		}
		/*checking R Key*/
		if (check('R')==1){
			while (check('R')==1){
				
			}
			if (mode==0){
				
				mode=1;
				pthread_create(&t2,NULL, read, &result);	//create thread to continuously check the temperature data

			}


		}
		/*checking S and T Key*/
		if (check ('S')==1 && check('T')==1){
			mode=2;
			
			
			break;
		}
		
		/* set wait time to 20ms is fast enough for physical pressing*/
		Sleep(20);

	}
	
	pthread_join(t, NULL); 		//wait for thread 1
	pthread_join(t2, NULL); 	//wait for thread 2
	pthread_join(t3, NULL); 	//wait for thread 3
	return 0;
}


/* temperature sensing function*/

	/* 
	input: Device number
	output:  Device current temperature
	*/
int tempscan(int n){
	/*
	
	currently the function is simulating the device temperature from a local txt file
	Future implementation: Please replace the entire function with proper communication functions: 
	1.setting up transmission protocols between the programme and the target device
	2.sending command messages to the target device
	3.wait for target device to response
	4.retrieve the temperature data from target device.
	5.return the data
	
	*/
	
	FILE *fp=fopen("temp.txt", "r");
	int i=0;
	int c;
	for (i=0; i<n; i++){
		fscanf_s(fp,"%d",&c);
	}
	// printf("temp is %d\n",c);
	fclose(fp);
	return c;
}



/* clear screen */
void clrscr(){
    system("@cls||clear");
}


/* input: device number, devices current status, last update time (in second)*/ 
void printstatus(int a,int b, int c){
	if (b==-1){
		printf("%s status is unknown\n",list[a-1].name);
	}else if (b==1){
		printf("%s is working fine. Last update %d seconds ago\n",list[a-1].name,c);
		
	}else{
		printf("%s is not working fine. Last update %d seconds ago\n",list[a-1].name,c);
	}
}

/* check keyboard pressing */
int check(char c){	//
	if(!KEY_DOWN(c)){
		
		return 0;
	}else {

//		printf("buttom is pressed\n");
		return 1;
	}
}

	/* 
	function to print out what's the current cycle and what device will be check next. Can indicate if the programme is running, pausing or stopping.
	input: status 
	*/
	
void printprogress(int now){
	printf("Current progress: ");
	switch(mode){
		case 1:
			printf("Cycles %d waiting to check %s\n",cycle,list[now-1].name);
			break;
			
		case 0:
			printf("Pausing\n");
			break;
		case 2:
			printf("Programme stopped");
			break;
		
	}
	
}

	/* welcome message with the logo of the programme*/
void welcome(){
	clrscr();
	printf("Thank you for using Cooloo Device Monitoring Software\n");
	printf(" ######   #######   #######  ##        #######   #######  \n##    ## ##     ## ##     ## ##       ##     ## ##     ## \n##       ##     ## ##     ## ##       ##     ## ##     ## \n##       ##     ## ##     ## ##       ##     ## ##     ## \n##       ##     ## ##     ## ##       ##     ## ##     ## \n##    ## ##     ## ##     ## ##       ##     ## ##     ## \n ######   #######   #######  ########  #######   #######  \n");
	printf("Cooloo Version 1.0 Created by Haowei Zhu\n");
	
}

/* logo at the top */
void logo(){
	printf("Cooloo V1.0 powered by 4rf \n");
}

	/* message to let the user know what key to be press*/
void buttommsg(){
	switch(mode){
		case 1:
			printf("\nPress P key at any time during the operating to PAUSE the programme.\nPress S and T key at the same time during the operating to STOP the programme.\n");
			break;
		case 0:
			printf("\nPress R key at any time to RESUME.\nPress S and T key at the same time to STOP the programme.\n");
			break;
	}
	
	/* please comment out the following line in future implementation*/
	printf("Please modify temp.txt to change temperature data.\n");
	
}

/* To print different type of error messages such as invalid configurations and invalid command, can be implemented more */
void errormsg(int a){
	switch (a){
		case 1:
			printf("Invalid. Please enter again\n");
			break;
		

	}
	
	
}

/*This function is used for writing to alerts history to the log file in the same folder*/	

	/* 
	device:standards for device number
	good: 0 indicates setting up alert, 1 indicates clearing alert
	type: 1 temperature, can implement more later
	*/
void writemsg(int device, int good,int type){
	

	
	//open the log file
	FILE * fp;
	fp = fopen ("log.txt","a");
	
	//read the current time
	time_t now = time(0);
	char buffer[100];
	struct tm *ltm;
	
    ltm = localtime(&now);
	
	sprintf(buffer, "[%d.%d.%d]%d:%d:%d",(1900+ltm->tm_year),(1 + ltm->tm_mon),ltm->tm_mday,ltm->tm_hour,ltm->tm_min,ltm->tm_sec);
//	printf("%s\n",buffer);

	switch (type){
		
		/* temperature */
		case 1:	
		
		switch (good){	
			
			//set up alert
			case 0:
				sprintf(buffer, "[%d.%d.%d]%d:%d:%d %s temperature alert raised",(1900+ltm->tm_year),(1 + ltm->tm_mon),ltm->tm_mday,ltm->tm_hour,ltm->tm_min,ltm->tm_sec,list[device-1].name);
				strcpy(msg2,buffer);
				fprintf (fp, "%s\n",buffer);
				break;
			
			//clear alert
			case 1:
				sprintf(buffer, "[%d.%d.%d]%d:%d:%d %s temperature alert cleared",(1900+ltm->tm_year),(1 + ltm->tm_mon),ltm->tm_mday,ltm->tm_hour,ltm->tm_min,ltm->tm_sec,list[device-1].name);
				strcpy(msg2,buffer);
				fprintf (fp, "%s\n",buffer);
				break;
			
		}
		break;
		
		/* implement other alerts here */
	}	
	/* close the file*/  
	fclose (fp);
}

/* shift up the alert messages that stored in the memory*/
void msgshiftup(){
	for (int i=0;i<8;i++){
		strcpy(msg[i],msg[i+1]);

	}
	
}

/*store message to the end of the array */
void storemsg(){
	/* storing the new alert message*/
	if (point<10){
		strcpy(msg[point],msg2);
		point++;
	}else{
		msgshiftup();
		strcpy(msg[9],msg2);
	}	
}

void lock(){
	
	pthread_mutex_lock( &mutex1 ); // Mutex lock
			
	
}

void unlock(){
	pthread_mutex_unlock( &mutex1 ); // Mutex unlock
	
}
