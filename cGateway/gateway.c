#include "gateway.h"

/* callbacks */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static const struct unicast_callbacks unicast_call = {unicast_recv};

/* connection */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;
/*---------------------------------------------------------------------------*/
PROCESS(gateway_process, "gateway");
AUTOSTART_PROCESSES(&gateway_process);
/*---------------------------------------------------------------------------*/

/* Global an others */

unsigned char level = 0; //depth level of the node
bool mode = true; //true : periodic mode -- false : change mode
int send = false; //if the node must send values or not

/*---------------------------------------------------------------------------*/
/* FUNCTIONS */
/*---------------------------------------------------------------------------*/

static void printMode(){
   if (mode){
      printf("Mode : Values send periodicaly\n");
   }
   else{
      printf("Mode : Values send when changed\n");
   }
}

static void printSend(){
   if (send){
      printf("Sending values -> At least one subscriber\n");
   }
   else{
      printf("Stop Sending -> no subscriber\n");
   }
}


/*---------------------------------------------------------------------------*/
/* RECEIVERS */
/*---------------------------------------------------------------------------*/

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
   //Receive orphan request
   char msg[5] = {0,1,level,mode,send};
   packetbuf_copyfrom(msg, 3);
   unicast_send(&unicast, from);
}


static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from)
{
   if ( ((char *)packetbuf_dataptr())[0] == 0){ //If it is a tree message (0)
      if ( ((char *)packetbuf_dataptr())[1] == 2){ 
         //If it is a parent verification (2)
         char msg[5] = {0,3,level,mode,send};
         packetbuf_copyfrom(msg, 5);
         unicast_send(&unicast, from);
      }
   }
   
   else if (((char *)packetbuf_dataptr())[0] == 1){ //If it is sensor data
      if (send){ 
         //print data to get them in the gateway
         printf("Sensor:%d:%d:%d\n", ((char *)packetbuf_dataptr())[1],
         ((char *)packetbuf_dataptr())[2],((char *)packetbuf_dataptr())[3]);
      }
   }
}


/*---------------------------------------------------------------------------*/
/* Process */
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(gateway_process, ev, data)
{
   PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
   PROCESS_EXITHANDLER(unicast_close(&unicast);)
   PROCESS_BEGIN();
   
   broadcast_open(&broadcast, 129, &broadcast_call);
   unicast_open(&unicast, 144, &unicast_call);
      
   while(1) {
      PROCESS_WAIT_EVENT();
      if(ev == serial_line_event_message) { //Wait for a message from gateway
         if ( ((char *)data)[0] == 'M'){
            mode = !mode; //change mode
            printMode();
         }
         
         else if ( ((char *)data)[0] == 'S'){
            send = !send; //change send
            printSend();
         }
      }
   }

   PROCESS_END();
}
/*---------------------------------------------------------------------------*/
