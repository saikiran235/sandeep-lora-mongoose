#include"mgos.h"
#include "mgos_arduino_lora.h"
#include "mgos_adc.h"
#include "mgos_mqtt.h"
//#include "mgos_arduino.h"

#define TX_PAYLOAD_LENGTH 20
#define RX_PAYLOAD_LENGTH 20
#define MOTOR_ON 1
#define MOTOR_OFF 0
#define  TELEMETRY_TOPIC "v1/devices/me/telemetry"

 LoRaClass* lora;


//const char *dev_id;
const char *src_id;
int led_rx = 0;
int led_tx = 0;
int relay_pin = 0;
int ldr_pin = 0;
int status_led = 0;
bool motor_state=0;
const char *dest_id ;
unsigned long status_time=0;
bool flag = 0;

enum stat_dev
{
  BUTTON_ON = 1,
  BUTTON_OFF = 2,
  ACKNOWLEDGEMENT_ON = 3,
  ACKNOWLEDGEMENT_OFF = 4,
  HEARTBEAT_RESPONSE = 5
};


/* mqtt callback function */
static void mqtt_cb(void* arg)
{
   bool res = mgos_mqtt_pubf(TELEMETRY_TOPIC, 0, false /* retain */, "{status:ALIVE}" );
   if(res)
   {
          LOG(LL_INFO, ("MQTT Published"));
   }
   (void) arg;
}


void createCmd(char *tx_buf, int cmd, const char *dest_id_,bool state)
{
    char cmd_buf[2];
  strcpy(tx_buf, src_id);
  strcat(tx_buf, dest_id_);
  itoa(cmd, cmd_buf, 10);
  strcat(tx_buf, cmd_buf);
  itoa(state, cmd_buf, 10);
  strcat(tx_buf,cmd_buf);
  tx_buf[strlen(tx_buf) + 1] = '\0';
}

static void status_cb(void *arg) 
{

  char tx_buf[TX_PAYLOAD_LENGTH] = {0};
  dest_id = mgos_sys_config_get_app_dest_id();
  createCmd(tx_buf, (int)HEARTBEAT_RESPONSE, dest_id,motor_state);
  mgos_send_lora(lora,tx_buf);
  //{
    LOG(LL_INFO, ("LoRa ALIVE sent : %s", tx_buf));
  //}
 // else
 // {
  //  LOG(LL_INFO, ("LoRa ALIVE send FAILED"));
  //} 
   memset(tx_buf,'\0',sizeof(tx_buf));
  mgos_lora_receive_mode(lora); 

  (void) arg;
}

static void status_timer(void *arg) 
{
  if(millis() - status_time > 65000)
  {
    mgos_gpio_blink(status_led,0,0);
    mgos_gpio_write(led_tx, LOW);
    status_time = millis();
    flag = 0;
  }
else if(flag == 1)
{
 mgos_gpio_blink(status_led,500,500);
}

}



static void button_on(int pin, void *arg)
{
  dest_id = mgos_sys_config_get_app_dest_id();
  char tx_buf[TX_PAYLOAD_LENGTH] = {0};
  createCmd(tx_buf, (int)BUTTON_ON, dest_id, MOTOR_OFF);
 // bool res = mgos_mqtt_pubf(TELEMETRY_TOPIC, 0, false /* retain */,
                            /*"{total_ram: %lu, free_ram: %lu, command: %s}",
                            (unsigned long)mgos_get_heap_size(),
                            (unsigned long)mgos_get_free_heap_size(),
                            tx_buf);
  char buf[8];
  LOG(LL_INFO,
      ("Pin: %s, published: %s", mgos_gpio_str(pin, buf), res ? "yes" : "no"));*/
  mgos_send_lora(lora,tx_buf);
  LOG(LL_INFO, ("LoRa BUTTON_ON Sent: %s", tx_buf));
  //else
 // {
  //  LOG(LL_INFO, ("LoRa BUTTON_ON Sent: FAILED"));
  //}
  memset(tx_buf,'\0',sizeof(tx_buf));
  mgos_lora_receive_mode(lora); 

  (void)pin;
  (void)arg;
}

static void button_off(int pin, void *arg)
{
  dest_id = mgos_sys_config_get_app_dest_id();
  char tx_buf[TX_PAYLOAD_LENGTH] = {0};
  createCmd(tx_buf, (int)BUTTON_OFF, dest_id, MOTOR_OFF);

  //bool res = mgos_mqtt_pubf(TELEMETRY_TOPIC, 0, false /* retain */,
                            /*"{total_ram: %lu, free_ram: %lu, command: %s}",
                            (unsigned long)mgos_get_heap_size(),
                            (unsigned long)mgos_get_free_heap_size(),
                            tx_buf);
  char buf[8];
  LOG(LL_INFO,
      ("Pin: %s, published: %s", mgos_gpio_str(pin, buf), res ? "yes" : "no"));*/
  mgos_send_lora(lora,tx_buf);
 // {
   LOG(LL_INFO, ("LoRa BUTTON_OFF Sent: %s", tx_buf));
  //}
 // else
 // {
  //  LOG(LL_INFO, ("LoRa BUTTON_OFF Sent :FAILED"));
 // }
    memset(tx_buf,'\0',sizeof(tx_buf));
    mgos_lora_receive_mode(lora); 
  (void)pin;
  (void)arg;
}

void relay_on(char *tx_buf)
{

   mgos_send_lora(lora,tx_buf);
 // {
    LOG(LL_INFO, ("LoRa ACKNOWLEDGEMENT_ON sent: %s\n", tx_buf));
    mgos_gpio_write(led_rx, HIGH);
    mgos_gpio_write(relay_pin, HIGH);
//  }
 // else
  //{
  //  LOG(LL_INFO, ("LoRa Tx ON packet FAILED"));
  //}

}


bool ldr_status(void)
{
  int vol = mgos_adc_read_voltage(ldr_pin);
  LOG(LL_INFO, ("voltage:%d", vol));
  /* Reference Voltage 5000 mv(As per tested conditions) */
  if (vol > mgos_sys_config_get_sys_esp32_adc_vref())
    return true;
  else
    return false;
}

void lora_data_receive(int packet_size)
{
 if(packet_size < RX_PAYLOAD_LENGTH)
 {
        uint8_t recvbuf[RX_PAYLOAD_LENGTH] = {0};
        char tx_buf[TX_PAYLOAD_LENGTH] = {0};
        char cmd[2] = {0};
        char received_dest_id[10]={0};
      
        /*Receiving timer code */
   LOG(LL_INFO, ("\n[INFO]\t: LoRa Packet received"));
    for (int i = 0; i < packet_size; i++)
    {
      recvbuf[i] = (char)mgos_LoRa_read(lora);
    }
   
    LOG(LL_INFO, ("RECEIVED PACKET :%s",recvbuf));
   
      strncpy(received_dest_id,(const char *)recvbuf+5,5);
      
      //LOG(LL_INFO, ("RECEIVED ID :%s",received_dest_id));
      
      //LOG(LL_INFO, ("DEST ID :%s",dest_id));
     
     
       if(strcmp(received_dest_id,dest_id) == 0)
       {
        
          cmd[0] = recvbuf[strlen((const char *)recvbuf) - 2];
          int val_Rx = atoi(cmd);
     
          cmd[0] = recvbuf[strlen((const char *)recvbuf) - 1];
          motor_state = atoi(cmd); 
       
         // LOG(LL_INFO, ("val_Rx : %d", val_Rx));
         // LOG(LL_INFO, ("motor : %d", motor_state));

        switch (val_Rx)
        {
         case BUTTON_OFF:
         {
            LOG(LL_INFO, ("LoRa BUTTON_OFF command received: %d", BUTTON_OFF));
          //char tx_buf[data_len+1] = {0};
            createCmd(tx_buf, (int)ACKNOWLEDGEMENT_OFF, received_dest_id, MOTOR_OFF);
            motor_state = 0;
            mgos_send_lora(lora,tx_buf);
            mgos_gpio_write(led_rx, LOW);
            mgos_gpio_write(relay_pin, LOW);
            LOG(LL_INFO, ("LoRa ACKNOWLEDGEMENT_OFF sent: %s\n", tx_buf));
          //else
          //{
           // LOG(LL_INFO, ("LoRa Tx OFF packet FAILED"));
         // }
         }
         break;
         case BUTTON_ON:
         {
          LOG(LL_INFO, ("LoRa BUTTON_ON command: %d received", BUTTON_ON));
          //char tx_buf[data_len+1] = {0};


          if (!mgos_sys_config_get_app_ldr_enable())
          {
            //if ldr disabled then turn ON relay
            motor_state = 1;
            createCmd(tx_buf, (int)ACKNOWLEDGEMENT_ON,received_dest_id, MOTOR_ON);
            relay_on(tx_buf);
          }
          //if ldr enabled then turn ON relay after checking ref voltage
          else
          {
             if (ldr_status())
             {
                motor_state = 1;
               createCmd(tx_buf, (int)ACKNOWLEDGEMENT_ON,received_dest_id, MOTOR_ON);
               LOG(LL_INFO, ("LDR Voltage OK"));
               relay_on(tx_buf);
             }
          }
         }
         break;
         case ACKNOWLEDGEMENT_ON:{
          LOG(LL_INFO, ("LoRa ACKNOWLEDGEMENT_ON: %d received\n", ACKNOWLEDGEMENT_ON));
          mgos_gpio_write(led_tx, HIGH);}
          break;
         case ACKNOWLEDGEMENT_OFF:{
          LOG(LL_INFO, ("LoRa ACKNOWLEDGEMENT_OFF: %d received\n", ACKNOWLEDGEMENT_OFF));
          mgos_gpio_write(led_tx, LOW);}
          break;
         case HEARTBEAT_RESPONSE:{
          LOG(LL_INFO, ("LoRa HEARTBEAT_RESPONSE : %d  received TOGGLE\n", HEARTBEAT_RESPONSE));
          flag = 1;
          mgos_gpio_write(led_tx, motor_state);
          status_time = millis();
           }
          break;
        default:LOG(LL_INFO, ("INVALID OPERATION"));
          break;
        }
      }
     memset(tx_buf,'\0',sizeof(tx_buf));
     memset(recvbuf,'\0',sizeof(recvbuf));
     mgos_lora_receive_mode(lora); 
  }
}




enum mgos_app_init_result mgos_app_init(void) {
	
   lora = mgos_LoRa_create();
   mgos_LoRa_setpins(lora,mgos_sys_config_get_spi_cs0_gpio(), mgos_sys_config_get_lora_reset(),mgos_sys_config_get_lora_dio0());

  long int frequency = mgos_sys_config_get_lora_frequency();
  if(!mgos_LoRa_begin(lora,frequency))
  {
    LOG(LL_INFO,("[INFO]:LORA NOT INTIALIZED"));
  }
  else
  {
     LOG(LL_INFO,("[INFO]:LORA INTIALIZED"));
  }

 // dev_id = mgos_sys_config_get_device_id();
  src_id = mgos_sys_config_get_app_src_id();
  dest_id = mgos_sys_config_get_app_dest_id();
 
 int btn1_pin = mgos_sys_config_get_board_btn1_pin();

  if (btn1_pin > 0)
    mgos_gpio_set_button_handler(btn1_pin, MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_NEG, 100 /* debounce_ms */, button_on, NULL);

  /* OFF Button at transmitter  */
  int btn2_pin = mgos_sys_config_get_board_btn2_pin();

  if (btn2_pin > 0)
    mgos_gpio_set_button_handler(btn2_pin, MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_NEG, 100 /* debounce_ms */, button_off, NULL);

  led_tx = mgos_sys_config_get_board_led1_pin();

  if (led_tx > 0)
  {
    mgos_gpio_set_mode(led_tx, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(led_tx, LOW);
  }

  /* LED at receiver */
  led_rx = mgos_sys_config_get_board_led2_pin();

  if (led_rx > 0)
  {
    mgos_gpio_set_mode(led_rx, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(led_rx, LOW);
  }

  /* Relay at receiver */
  relay_pin = mgos_sys_config_get_board_led3_pin();

  if (relay_pin > 0)
  {
    mgos_gpio_set_mode(relay_pin, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(relay_pin, LOW);
  }

  /* Alive status pin */
  status_led = mgos_sys_config_get_app_status_led();
  
  if(status_led > 0)
  {
    mgos_gpio_set_mode(status_led, MGOS_GPIO_MODE_OUTPUT); 
    mgos_gpio_write(status_led, LOW);
  }  

  /* LDR at receiver*/
  ldr_pin = mgos_sys_config_get_app_ldr_gpio();
  if(ldr_pin > 0)
  {
    mgos_gpio_set_mode(ldr_pin, MGOS_GPIO_MODE_INPUT);
    mgos_adc_enable(ldr_pin);
  } 

  if (mgos_sys_config_get_app_receiver())
  {
    // At RECEIVER sending response
    mgos_set_timer(30000 /* ms */, MGOS_TIMER_REPEAT, status_cb, NULL);
  }
  if (!mgos_sys_config_get_app_receiver())
  {
    // At  TRANSMITTER for checking response
    mgos_set_timer(1000 /* ms */, MGOS_TIMER_REPEAT, status_timer, NULL);
  }

   mgos_lora_receive_callback(lora,lora_data_receive);
   mgos_lora_receive_mode(lora);
   
  if (mgos_sys_config_get_app_receiver())
  {
    status_cb(NULL); 
  }

      mgos_set_timer(30000 /* ms */, MGOS_TIMER_REPEAT, mqtt_cb, NULL);

  return MGOS_APP_INIT_SUCCESS;
}

