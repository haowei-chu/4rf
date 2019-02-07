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


int mode=1;	//mode 1: operating, mode 0: pausing, mode 2: stopping and waiting to exit
int cycle=1;
int x=0;
int active=0;
int rate;	//rate of monitoring
int rate2;	//gap between each cycle
int current=0;
int pause=0;
int next=0;
char msg[10][100];
int msgp=0;
char msg2 [100];
int point=0;
int config=1;



// add Mutex for data modifying
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

struct Device{
	char name[33];
	int tempup;	//upper temperature threshold 
	int templow;//lower temperature 
	/*
	others can be added here in the future 
	*/
};
struct Device list[10];	//create an array for the device

struct para{
	int device;
	int fault;	//-1 unknown, 0 fault, 1 working fine
	int live;	//live result (in second)
};

int tempscan(int x);
void clrscr();	//clear screen function
void printstatus(int a,int b, int c);
char readstring();
int readnumber();
int check(char c);
void printprogress(int now);
void welcome();
void logo();
void buttommsg();
void errormsg(int a);
void writemsg(int device, int good,int type);
void msgshiftup();
void storemsg();

void* UI(void* data) {
  // char *str = (char*) data; 
	struct para *result=(struct para*)data;	//input data
//	for(int i = 0;i < 5;++i) {
	
	//while loop to display UI during the operation
	while (mode!=2){
		
		//Normal UI for the programme
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
			
			pthread_mutex_lock( &mutex1 ); // Mutex lock
			printstatus(result[j].device,result[j].fault,result[j].live);	//print the status out		
			result[j].live++;	//update the time for 1 second
			pthread_mutex_unlock( &mutex1 ); // Mutex unlock
		}
		buttommsg();
		
		
		/*Print out the latest 10 alert messages here */
		
		printf("\nImportant messages(latest 10 if more than 10, from latest to oldest):\n");
		for (int j=(point-1);j>=0;j--){
			printf("%s\n",msg[j]);
			
		}
		
//		printf("hi %d\nlive %d ago\n",result[i].device,result[i].fault); 	//Output in CMD every second
		
		/* UI refreshing rate is 1 second*/
		sleep(1);	
	}
	
	//Display for stopping the programme
	clrscr();
	logo();
	printprogress(2);
	
	pthread_exit(NULL); 	//exit the thread
}

void* read(void* data){		//the thread that use for comparing the temperature of the device and the limits
	struct para *result=(struct para*)data;	//input data
	int temp2=next;
	while (mode==1){
//		next=0;
		for (int i=temp2;i<active;i++){
			
//			current=result[i].device;
	//			temp=tempscan(number[i]);
			int temp=tempscan(result[i].device);	//read the temperature data from device
//			printf("%d %d %d \n",temp,list[result[i].device-1].templow,list[result[i].device-1].tempup);
			
			//check if the temp is in the range
			
			pthread_mutex_lock( &mutex1 ); // Mutex lock
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
						
						/*
						
						//print error messages
						
						*/
						
						writemsg(result[i].device,0,1);
						storemsg();

						
						
					}
					break;
					
				//if its originally outside the limits
				case 0:
					if (temp>=list[result[i].device-1].templow && temp<=list[result[i].device-1].tempup){	//check if the temp is inside the limits
						
						//if inside the limits
						result[i].fault=1;
						
						/*
						print update messages						
						*/
						writemsg(result[i].device,1,1);
						storemsg();
						
					}else{
						
						//if still outside
						
						//do nothing
//						result[i].fault=0;
						
					}				
				
				
					break;
				

				//if originally at working temperature
				case 1:
					if (temp>=list[result[i].device-1].templow && temp<=list[result[i].device-1].tempup){	//check if the temp is inside the limits
						
						//if still inside the limits
						
						//do nothing
//						result[i].fault=1;
					}else{
						
						//if changed to outside
						result[i].fault=0;
						
						/*
						
						//print error messages
						
						*/
						writemsg(result[i].device,0,1);
						storemsg();
					}					
				
					break;
			

			}
			/*  end of the checking temperature process*/
			
			
			/* Can implement other features here such as checking memory usage */
			
			result[i].live=0;
			pthread_mutex_unlock( &mutex1 ); // Mutex unlock
			
			/* mathematic calculations when the programme is pause*/
			if (mode==0) {
//				printf("a\n");
				if ((current+1)==active){
					next=0;
				}else{
					next=current+1;
					pthread_mutex_lock( &mutex1 ); // Mutex lock
					cycle--;
					pthread_mutex_unlock( &mutex1 ); // Mutex unlock
				} 
				break;
			}
			
			
			sleep(rate);	//break time between each device
			
		}	
		pthread_mutex_lock( &mutex1 ); // Mutex lock
		cycle++;
//		printf("finish one cycle\n");
		pthread_mutex_unlock( &mutex1 ); // Mutex unlock
//		sleep(1);
		temp2=0;
		sleep(rate2-rate);
	}	
	
	pthread_exit(NULL);
}

int main()
{
	
	//create variables
//	int x;	//how many devices
	int i;	
	char buffer[100];
//	int active;	//
	int temp;
//	int c;
//	int currenttemp[10];
//	int live[10];
//	struct Device list[10];	//create an array for the device
	
	struct para result[10];
	for (i=0;i<10;i++){
		result[i].fault=-1;
		result[i].live=0;
		strcpy(msg[i],"hi");
		
	}
	
	// for (i=0;i<10;i++){
		// result[i].live=0;
		
	// }
	
	
//	char buffer[33];
	pthread_t t; 	//first thread UI
	pthread_t t2; 	//second thread monitoring
	
	welcome();
	while (1){
//		clrscr();
		
		printf("Loading configurations...\n");
		sleep(1);

		FILE *fp=fopen("device.txt", "r");//open the txt file that contains the data of the devices

			if(NULL == fp){	//check if the file is not existing
			
				printf("the device file is not exist\n");

			}
		
		fscanf_s(fp,"%d",&x);	//how many devices we r entering
		if (x>10){
			printf("no more than 10 device\n");
		}
		printf("There are %d devices details loaded\n" , x);

		
		for (i=0;i<x;i++){	//load the config for each device
			fscanf(fp,"%s",&list[i].name); 
			fscanf_s(fp,"%d",&list[i].tempup);
			fscanf_s(fp,"%d",&list[i].templow);
			printf("Device %d is: %s with upper temperature limit of %d and lower temperature limit of %d \n" , (i+1) , list[i].name, list[i].tempup,list[i].templow);
			
		}
		fclose(fp);
		

		
		while(1){
			printf("Please confirm the device configurations. Enter Y to continue or N to reload:");
			
			
			
			scanf_s("%s",&buffer);
//			strcpy(buffer, readstring());


			if (buffer[0]=='Y'){
				break;
		
			}else if (buffer[0]=='N'){
				break;
				
			}else{
				printf("Invalid. Please enter again\n");
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

	}else{
		
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
	}
	fclose(fp);// close file
	
	printf("Configs are showing here:\n");
	printf("Monitoring %d devices:",active);
	for (i=0;i<active;i++){
		printf("%d ",result[i].device);
	}
	printf("\nRate of monitoring is %d seconds\nGap between each cycle is %d second\n",rate,rate2);
	
	printf("Would you like to use this config or enter them manually? Enter Y to use or enter N to enter them manually");
	
	while (1){
		scanf_s("%s",&buffer);
		if (buffer[0]=='Y'){
			config=1;
			break;
		}else if(buffer[0]=='N'){
			config=0;
			break;
			
		}else{
			printf("Invalid. Please enter again\n");
			printf("Would you like to use this config or enter them manually? Enter Y to use or enter N to enter them manually:");
		}
	}
	/* start of entering rest of config*/
	
	
	switch(config){
		case 0:
			printf("Please enter the amount of device that would like to monitor:");
			scanf_s("%d",&active);
			while (active>x){
				printf("Invalid. Please enter the number again.\n");
				printf("Please enter the amount of device that would like to monitor:");
				scanf_s("%d",&active);
			}
			
			int number[10];
			printf("Please enter the number # of device that would like to monitor in ascending order, one number at a time\n");
			for (i=0;i<active;i++){
				printf("Please enter the number # of device that would like to monitor and press enter:");
				
				scanf_s("%d",&temp);
				while (temp>x){
					printf("Invalid. Please enter the number again.\n");
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
					printf("Invalid. Please enter again.\n");
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
					printf("Invalid. Please enter again.\n");
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
					printf("Invalid. Please enter again.\n");
					printf("Would you like to save those configs so that be use in the future? Enter Y to save or enter N to skip");
					continue;
				}
			}
			
			
			break;
		case 1:
			break;
	}
	
	
	pthread_create(&t, NULL, UI, &result); 	//create thread to continuously print out the monitoring result
	pthread_create(&t2,NULL, read, &result);	//create thread to continuously check the temperature data
	while (1){
//		check('K');

		//checking P Key
		if (check('P')==1){
			if (mode==1) mode=0;
//			sleep(2);
		}
		
		if (check('R')==1){
			while (check('R')==1){
				
			}
			if (mode==0){
				
				mode=1;
				pthread_create(&t2,NULL, read, &result);	//create thread to continuously check the temperature data
//				printf("back to mode 1\n");
			}

//			Sleep(2);
		}
		
		if (check ('S')==1 && check('T')==1){
			mode=2;
			
			
			break;
		}
		Sleep(20);
		
		
		
//		for (i=0;i<active;i++){
//			temp=tempscan(number[i]);
//			temp=tempscan(result[i].device);	//read the temperature data from device
//			if (temp>=list[i])
//			clrscr();
//			printf("tempeture is %d\n",temp);
			
//			sleep(rate);
//		}

	}
	
	pthread_join(t, NULL); 		//wait for thread 1
	pthread_join(t2, NULL); 	//wait for thread 2
	
	
	
//	printf("Hello World!");
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

void clrscr(){
    system("@cls||clear");
}

void printstatus(int a,int b, int c){
	if (b==-1){
		printf("%s status is unknown\n",list[a-1].name);
	}else if (b==1){
		printf("%s is working fine. Last update %d seconds ago\n",list[a-1].name,c);
		
	}else{
		printf("%s is not working fine. Last update %d seconds ago\n",list[a-1].name,c);
	}
}


char readstring(){
	char temp;
	scanf_s("%s",&temp);
	return temp;
}
int readnumber(){
	int temp;
	scanf_s("%d",&active);
	return temp;
}

int check(char c){	//
	if(!KEY_DOWN(c)){
		
		return 0;
	}else {

//		printf("buttom is pressed\n");
		return 1;
	}
}

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

void welcome(){
	clrscr();
	printf("Thank you for using Cooloo Device Monitoring Software\n");
	printf(" ######   #######   #######  ##        #######   #######  \n##    ## ##     ## ##     ## ##       ##     ## ##     ## \n##       ##     ## ##     ## ##       ##     ## ##     ## \n##       ##     ## ##     ## ##       ##     ## ##     ## \n##       ##     ## ##     ## ##       ##     ## ##     ## \n##    ## ##     ## ##     ## ##       ##     ## ##     ## \n ######   #######   #######  ########  #######   #######  \n");
	printf("Cooloo Version 1.0 Created by Haowei Zhu\n");
	
}

/* logo at the top */
void logo(){
	printf("Cooloo V1.0\n");
}

void buttommsg(){
	switch(mode){
		case 1:
			printf("\nPress P key at any time during the operating to PAUSE the programme.\nPress S and T key at the same time during the operating to STOP the programme.\n");
			break;
		case 0:
			printf("\nPress R key at any time to RESUME.\nPress S and T key at the same time to STOP the programme.\n");
			break;
	}
	
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
