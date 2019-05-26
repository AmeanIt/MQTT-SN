#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#include "lib/list.h"
#include "lib/memb.h"

#include <stdio.h>
#include <stdbool.h>


static void reset();
// Reset the node values to an orphan node


/*---------------------------------------------------------------------------*/
/* RECEIVERS */
/*---------------------------------------------------------------------------*/

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from);
//handle broadcast messages

static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from);
//handle unicast messages


