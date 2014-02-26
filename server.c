#include <string.h>
#include "contiki-net.h"
static struct psock ps;
static int *buffer;
static int intervals[3] = {5,3,2};
static int current_id;
static int stocks[3] = {-1,-1,-1};
static int *sender_pointer;

static PT_THREAD(handle_connection(struct psock *p))
{
  PSOCK_BEGIN(p);
  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  PSOCK_READBUF(p);
  current_id = *buffer;
  sender_pointer = &intervals[current_id];
  PSOCK_SEND(p,sender_pointer,sizeof(sender_pointer));
  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  PSOCK_READBUF(p);
  printf("[Manager] - Got stock = %i\n",*buffer);
  stocks[current_id] = *buffer;
  PSOCK_CLOSE(p);
  PSOCK_END(p);
}
static PT_THREAD(wait_update(struct psock *p))
{
  PSOCK_BEGIN(p);
  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  PSOCK_READBUF(p);
  printf("[Manager] - Got update stock = %i\n",*buffer);
  stocks[current_id] = *buffer;
  PSOCK_CLOSE(p);
  PSOCK_END(p);
}
PROCESS(animalibus_manager, "Animalibus Manager");
AUTOSTART_PROCESSES(&animalibus_manager);
PROCESS_THREAD(animalibus_manager, ev, data)
{
  PROCESS_BEGIN();
  tcp_listen(UIP_HTONS(5555));
  PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
  if(uip_connected()) {
    PSOCK_INIT(&ps, buffer, sizeof(buffer));
    while(!(uip_aborted() || uip_closed() || uip_timedout())) {
      handle_connection(&ps);
      PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    }
  }
  printf("[Manager] - Connection closed\n");
  while(1){
    //Aguarda atualizacoes
    tcp_listen(UIP_HTONS(5555));
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    if(uip_connected()) {
      PSOCK_INIT(&ps, buffer, sizeof(buffer));
      while(!(uip_aborted() || uip_closed() || uip_timedout())) {
        wait_update(&ps);
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
      }
    }
  }
  PROCESS_END();
}
