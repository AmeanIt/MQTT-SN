#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/serial-line.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#include "lib/list.h"
#include "lib/memb.h"

#include <stdio.h>
#include <stdbool.h>


static void printMode();
//Print the new mode

static void printSend();
//Print if sensor must send or not


/*---------------------------------------------------------------------------*/
/* RECEIVERS */
/*---------------------------------------------------------------------------*/

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from);
//handle broadcast messages

static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from);
//handle unicast messages


