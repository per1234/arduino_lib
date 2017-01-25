#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H
#include <SoftwareSerial.h>
#include <Arduino.h>
#include "Musio_Common.h"
#include "QueueArray.h"


SoftwareSerial musio_serial(SERIAL_RX, SERIAL_TX);
class SerialPortManager{
  public :
  Message* fetchMSG();
  void writeResponsePacket(Message *msg);
  boolean readPacket();
  static SerialPortManager* getInstance(){
    static SerialPortManager serialPort;
    return &serialPort;
  }
  boolean check_connection();
  void sendDevinfoPacket(uint8_t* dev_infos, int size);
  int send_devinfo_flag=0;
  int pin_test_flag =0;
  void writeStr(char* str, int len){musio_serial.write(str,len);}
  private :
  void parsePacket(int packet_end, int index);
  void flushBuffer();
  char rxbuf[RXBUF_SIZE]={};
  int rxbuf_index=0;
  char txbuf[TXBUF_SIZE]={};
  int txbuf_index=0;
  QueueArray<Message> msgQueue;
  SerialPortManager(){
    musio_serial.begin(9600);
    flushBuffer();
  }
};
void SerialPortManager::writeResponsePacket(Message *msg){
  int index=0;
  int len = msg->len_data;
  txbuf[index++] = PACKET_START_CODE;
  txbuf[index++] = RESPONSE_PACKET;
  txbuf[index++] = msg->len_data;
  txbuf[index++] = msg->dev_pos;
  txbuf[index++] = msg->devid;
  txbuf[index++] = msg->code;
  uint8_t *ptr_d = msg->data;
  while(len-- > 1){
    txbuf[index++] = *(ptr_d++);
  }
  txbuf[index++] = PACKET_END_CODE;  
  musio_serial.write(txbuf, index);
}

 Message* SerialPortManager::fetchMSG(){
    Message* msg = msgQueue.pop();
    return msg;
  }
  
  
boolean SerialPortManager::readPacket(){
    int i=0;
    while(musio_serial.available() > 0 && (i < RXBUF_SIZE)){            
      rxbuf[i++] = musio_serial.read();
      delay(2);
    }
    flushBuffer();
    if(i > 0) {
      parsePacket(i,0);
      return true;
    }
    return false;
}


void SerialPortManager::parsePacket(int packet_end, int index){
    uint8_t len = 0;
    uint8_t packet_type ;   
      if(index + 2 >= packet_end) return;
      if(rxbuf[index++]==PACKET_START_CODE){
        // TYPE
        // HANDSHAKE : 0X01, CMD : 0X02, DATA : 0X03, RESTART : 0x04
        packet_type = rxbuf[index++];
        
        if(packet_type == ASK_DEVINFO_PACKET && rxbuf[index] == PACKET_END_CODE) { //give up remain packets
          send_devinfo_flag = 1;
          parsePacket(packet_end, index++);
          return;
        }
        else if(packet_type == PIN_TEST_PACKET && rxbuf[index] == PACKET_END_CODE) { //give up remain packets
          pin_test_flag = 1;
          return ;
        }
        else{     
          if(packet_type != CMD_PACKET)  {
            parsePacket(packet_end, index);  // search again
            return;
          }
          // LEN : CMD + DATA LENGTH >= 2, <= 8 BYTES
          
          len = rxbuf[index++];
          Message msg = {};
          msg.len_data = len;
          msg.dev_pos = rxbuf[index++];
          msg.devid = rxbuf[index++];  
          msg.code = rxbuf[index++];
          len--;
          msg.data = (uint8_t*)malloc(sizeof(char)*(len));
          for(int l=0;l<len;l++)        msg.data[l] = rxbuf[index++];       
            
          if(rxbuf[index++] == PACKET_END_CODE)        msgQueue.push(msg);           
          parsePacket(packet_end, index);
        }
      }else        parsePacket(packet_end, index);           
    return;
}
  

  
void SerialPortManager::sendDevinfoPacket(uint8_t* dev_infos, int dev_num){
    int index =0;
    int len = dev_num * 2;
    int i=0;
    
    txbuf[index++] = PACKET_START_CODE;
    txbuf[index++] = DEVINFO_PACKET;
    txbuf[index++] = dev_num;
    while(i < len){
      txbuf[index++] = dev_infos[i++];
    }
    txbuf[index++] = PACKET_END_CODE;
    
    free(dev_infos);
    
    musio_serial.write(txbuf,index);
    send_devinfo_flag = 0 ;
}

void SerialPortManager::flushBuffer(){
    while(musio_serial.available()){
      musio_serial.read();
    }
}


  
#endif