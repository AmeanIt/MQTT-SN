#include "sensor.h"

/* callbacks */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static const struct unicast_callbacks unicast_call = {unicast_recv};

/* connection */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

/*---------------------------------------------------------------------------*/
PROCESS(sensor_process, "sensor Tree");
PROCESS(data_process, "sensor Data");
AUTOSTART_PROCESSES(&sensor_process, &data_process);
/*---------------------------------------------------------------------------*/

/* Global an others */
enum sensorState {ORPHAN, PARENT};

bool isTree = false; //node is part of the tree or not
enum sensorState state = ORPHAN;
linkaddr_t parent;
unsigned char level = 255;  //depth level of the node
int nCheck = 0; //number of time the parent was checked
int maxCheck = 5; //maximum parent check

bool mode = true; //true : periodic mode -- false : change mode
bool send = false; //if the node must send values or not

int timer = 15; //time to wait
bool valueChanged = false; //sensor data changed
char coin; //random value
signed char temperature;
signed char oldTemperature;
unsigned char humidity;
unsigned char oldHumidity;


/*---------------------------------------------------------------------------*/
/* FUNCTIONS */
/*---------------------------------------------------------------------------*/

static void reset()
{  
   printf("PARENT MAYBE LOST ----> RESET\n");
   isTree = false;
   state = ORPHAN;
   level = 255;
   nCheck = 0;
   mode = true;
   send = false;
}

/*---------------------------------------------------------------------------*/
/* RECEIVERS */
/*---------------------------------------------------------------------------*/

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
   if (isTree) //Receive orphan request and node is in the tree to accept it
   {
      char msg[5] = {0,1,level,mode,send};
      packetbuf_copyfrom(msg, 5);
      unicast_send(&unicast, from);
   }
}


static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from)
{
   if (((char *)packetbuf_dataptr())[0] == 0){ //If it is a tree message (0)
      
      if ( ((char *)packetbuf_dataptr())[1] == 1){ //parent offer (1)
         if ( ((char *)packetbuf_dataptr())[2] < level-1){
            parent = *from;
            isTree = true;
            state = PARENT;
            level = ((char *)packetbuf_dataptr())[2] +1;
            mode = ((char *)packetbuf_dataptr())[3];
            send = ((char *)packetbuf_dataptr())[4];
            
            printf("Parent is now : %d.%d with level %d\n", 
            from->u8[0], from->u8[1], level);
         }
      }
      
      else if (((char *)packetbuf_dataptr())[1] == 2){ 
         //If it is a parent verification (2)
         unsigned char childLevel = ((char *)packetbuf_dataptr())[2]; 
         if (isTree && childLevel > level){ //If node still valid as parent
            char msg[5] = {0,3,level,mode,send}; //'update' values
            packetbuf_copyfrom(msg, 5);
            unicast_send(&unicast, from);
         }
         else {
            char msg[2] = {0,4}; //child should be orphan
            packetbuf_copyfrom(msg, 2);
            unicast_send(&unicast, from);
         }
      }
      
      else if (((char *)packetbuf_dataptr())[1] == 3){ //parent still valid
         level = ((char *)packetbuf_dataptr())[2] +1; //update all
         mode = ((char *)packetbuf_dataptr())[3];
         send = ((char *)packetbuf_dataptr())[4];
         nCheck = 0;
      }
      
      else if (((char *)packetbuf_dataptr())[1] == 4){ 
      //parent not valid -> orphan
         if( isTree ){
            reset(); 
         }
      }  
   }
   if (((char *)packetbuf_dataptr())[0] == 1){ //If it is sensor data
      if (send){
         //send to parent
         packetbuf_copyfrom(((char *)packetbuf_dataptr()), 4);
         unicast_send(&unicast, &parent);
      }
   }
}


/*---------------------------------------------------------------------------*/
/* Process */
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(sensor_process, ev, data)
{
   static struct etimer et;

   PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
   PROCESS_EXITHANDLER(unicast_close(&unicast);)
   PROCESS_BEGIN();
   
   broadcast_open(&broadcast, 129, &broadcast_call);
   unicast_open(&unicast, 144, &unicast_call);

   
   while(1) {
  
      if(state == ORPHAN)
      { 
         etimer_set(&et, CLOCK_SECOND * 3 + random_rand() % (CLOCK_SECOND * 3));
         PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
         
         char msg[2] = {0,0};
         packetbuf_copyfrom(msg, 2);
         broadcast_send(&broadcast); //search a valid parent
      }
      else
      {  
         etimer_set(&et, CLOCK_SECOND * 3);
         PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

         char msg[3] = {0,2, level};
         packetbuf_copyfrom(msg, 3);
         unicast_send(&unicast, &parent); //verify validity of the parent
         nCheck++;
         
         if (nCheck >= maxCheck) //if no response from the parent
         {
            reset(); //node become orphan        
         }
      }
   }

   PROCESS_END();
}

/*---------------------------------------------------------------------------*/


PROCESS_THREAD(data_process, ev, data)
{
   static struct etimer et;

   PROCESS_EXITHANDLER(unicast_close(&unicast);)
   PROCESS_BEGIN();
   
   unicast_open(&unicast, 144, &unicast_call);
   
   temperature = random_rand()%51-15;
   humidity = random_rand()%101;
   
   while(1) {
      coin = random_rand()%2;
      if (coin){ //to not change every time
         oldTemperature = temperature;
         temperature = random_rand()%51-15;
         oldHumidity = humidity;
         humidity = random_rand()%101;
         
         if (temperature != oldTemperature || humidity != oldHumidity)
         {
            valueChanged = true; 
            //likely to change but we verify it to be sure
         }
      }
      
      if (isTree && mode) { //if periodic mode and the node is in the tree
         etimer_set(&et, CLOCK_SECOND * timer);
         PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
         if (send){
            char msg[4] = {1, linkaddr_node_addr.u8[0], temperature, humidity};
            packetbuf_copyfrom(msg, 4);
            unicast_send(&unicast, &parent); //Send sensor data
            valueChanged = false;
         }
      }
      
      else if (isTree && !mode && valueChanged){ 
      //if change mode and the node is in the tree and value has changed
         if (send){
            char msg[4] = {1, linkaddr_node_addr.u8[0], temperature, humidity};
            packetbuf_copyfrom(msg, 4);
            unicast_send(&unicast, &parent); //Send sensor data
         }
         
         valueChanged = false;
         etimer_set(&et, CLOCK_SECOND * 10);
         PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
         
      }
      else {
         etimer_set(&et, CLOCK_SECOND * 10);
         PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      }
   }

   PROCESS_END();
}
