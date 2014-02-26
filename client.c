#include "contiki-net.h"
#include <stdio.h>
#include "dev/button-sensor.h"
#include "dev/leds.h"

static const int feeder_id = 1;
static struct psock ps;
static int *buffer;
static struct etimer et;
static int stock = 10;
static int *sender_pointer;
static int timer_interval;

PROCESS(animalibus_feeder, "Animalibus Feeder");

static int configuration(struct psock *p)
{
  PSOCK_BEGIN(p);

  sender_pointer = &feeder_id;
  PSOCK_SEND(p, sender_pointer, sizeof(sender_pointer));
  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  PSOCK_READBUF(p);
  timer_interval = *buffer;
  printf("[Feeder] - Got: timer_interval =  %i\n", timer_interval);
  sender_pointer = &stock;
  PSOCK_SEND(p, sender_pointer, sizeof(sender_pointer));
  PSOCK_END(p);
}

static int update(struct psock *p)
{
  PSOCK_BEGIN(p);
  printf("[Feeder] - FEED!\n");
  stock --;
  if(stock <= 0){
    leds_on(LEDS_RED);
  }
  sender_pointer = &stock;
  PSOCK_SEND(p, sender_pointer, sizeof(sender_pointer));
  PSOCK_CLOSE(p);
  PSOCK_END(p);
}


AUTOSTART_PROCESSES(&animalibus_feeder);
PROCESS_THREAD(animalibus_feeder, ev, data)
{
  uip_ipaddr_t addr;

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);

  uip_ipaddr(&addr, 172,16,2,0);
  //Estabelece a primeira conexao para informar o id e o estoque de comida no feeder
  tcp_connect(&addr, UIP_HTONS(5555), NULL);

  printf("Connecting...\n");
  PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

  if(uip_aborted() || uip_timedout() || uip_closed()) {
    printf("Could not establish connection...\n");
  } else if(uip_connected()) {
    printf("Connected\n");

    PSOCK_INIT(&ps, buffer, sizeof(buffer));

    do {
      PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
      configuration(&ps);
    } while(!(uip_closed() || uip_aborted() || uip_timedout()));

    printf("Connection closed.\n");
  }

  while(1){
    while(stock > 0){
      etimer_set(&et, CLOCK_SECOND*timer_interval);
      printf("[Feeder] - Set timer\n");
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      printf("[Feeder] - timer expired\n");
      uip_ipaddr(&addr, 172,16,2,0);
      //Estabelece a segunda conexao para informar que o estoque baixou
      tcp_connect(&addr, UIP_HTONS(5555), NULL);

      printf("Connecting...\n");
      PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

      if(uip_aborted() || uip_timedout() || uip_closed()) {
        printf("Could not establish connection...\n");
      } else if(uip_connected()) {
        printf("Connected\n");

        PSOCK_INIT(&ps, buffer, sizeof(buffer));

        do {
          PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
          update(&ps);
        } while(!(uip_closed() || uip_aborted() || uip_timedout()));

        printf("Connection closed.\n");
      }
    }
    PROCESS_WAIT_EVENT();
    if(ev == sensors_event && data == &button_sensor){
      stock = 10;
      leds_off(LEDS_RED);
    }
  }
  PROCESS_END();
}

