#include "type.hpp"
#include <chrono>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

class client {
public:
  client *poi;
  tcp_struct body;
  int fd_write, fd_read;
  int ret_write, ret_read;
  int curAck;
  int count;
  int countSend;
  int start;
  int sendingWindow[1000];
  message send;
  message recieve;
  void handshake();
  void sendMessage();
  void readMessage();
  void GBNLoopRead();
  void SRLoopRead();
  void SRSend();
  void GBNSend();

  void printSend();

  void printRecieve();

  static void *runGBNREAD(void *args) {
    static_cast<client *>(args)->GBNLoopRead();
    return nullptr;
  }

  static void *runSRREAD(void *args) {
    static_cast<client *>(args)->SRLoopRead();
    return nullptr;
  }

  void init();
};

void client::printSend() {
  printf("send.ACK : %d\n send.ack : %d\n send.buf : %s\n send.seq : %d\n "
         "send.syn : %d\n send.type : %d\n",
         send.ACK, send.ack, send.buf, send.seq, send.syn, send.type);
}
void client::printRecieve() {
  printf("recieve.ACK : %d\n recieve.ack : %d\n recieve.buf : %s\n recieve.seq "
         ": %d\n recieve.syn : %d\n recieve.type : %d\n",
         recieve.ACK, recieve.ack, recieve.buf, recieve.seq, recieve.syn,
         recieve.type);
}

void client::init() {
  countSend = 0;
  ret_write = mkfifo("server_fifo_pipe", 0666); //创建命名管道
  ret_read = mkfifo("client_fifo_pipe", 0666);  //创建命名管道
  if (ret_write != 0) {
    perror("mkfifo server");
  }

  if (ret_read != 0) {
    perror("mkfifo client");
  }

  printf("before open\n");

  fd_write = open("server_fifo_pipe", O_WRONLY); //等着只写;
  if (fd_write < 0) {
    perror("open write fifo");
  }

  fd_read = open("client_fifo_pipe", O_RDONLY); //等着只读;
  if (fd_read < 0) {
    perror("open read fifo");
  }
  start = 0;
  curAck = -1;
  count = 1;
}

void client::sendMessage() {
  countSend++;
  write(fd_write, &send, sizeof(send));
}

void client::readMessage() { read(fd_read, &recieve, sizeof(recieve)); }

void client::GBNLoopRead() {
  for (int i = 0; i < 4; i++) {
    printf("curAck: %d\n", curAck);
    printf("count: %d\n", count);
    if (curAck >= 200) {
      return;
    }
    printf("before read\n");
    readMessage();
    printRecieve();
    if (curAck == recieve.ack) {
      count++;
      if (count >= 3) {
        start = curAck + 1;
      }
    } else {
      curAck = recieve.ack;
      count = 1;
    }
  }
}

void client::SRLoopRead() {
  for (int i = 0; i < 4; i++) {
    readMessage();
    printRecieve();
    sendingWindow[recieve.ack] = 1;
    while (sendingWindow[start] != 0) {
      start++;
    }
  }
}

void client::handshake() {

  send.ack = 0;
  send.syn = 1;
  send.seq = 0;
  send.type = ack;

  sendMessage();

  printf("send handshake request success\n");
  printSend();

  readMessage();
  printf("read handshake ack success\n");
  printRecieve();

  if (recieve.ACK != 1 && recieve.syn != 1 && recieve.ack != send.seq + 1) {
    perror("connect error");
    return;
  }
  printf("first handshake success\n");

  send.ack = recieve.seq + 1;
  send.seq = recieve.ack + 1;
  send.ACK = 1;

  sendMessage();
  printSend();
  printf("connect success\n");
}

void client::GBNSend() {
  recieve.ack += 1;

  while (start < 201) {
    printf("before loop\n");
    for (int i = 0; i < 4 && start + i < 201; i++) {
      sprintf(send.buf, "mes num: %d", start + i);
      send.ack = recieve.seq + 1;
      send.seq = start + i;
      send.type = msg;
      sendMessage();
      printSend();
    }
    start += 4;
    pthread_t t;
    pthread_create(&t, NULL, runGBNREAD, poi);
    this_thread::sleep_for(chrono::milliseconds(100));
    pthread_cancel(t);
  }
}

void client::SRSend() {
  recieve.ack += 1;

  while (start < 201) {
    for (int i = 0; i < 4 && start + i < 201; i++) {
      if (sendingWindow[start + i] == 0) {
        sprintf(send.buf, "mes num: %d", start + i);
        send.ack = recieve.seq + 1;
        send.seq = start + i;
        send.type = msg;
        sendMessage();
        printSend();
      }
    }
    pthread_t t;
    pthread_create(&t, NULL, runSRREAD, poi);
    this_thread::sleep_for(chrono::milliseconds(100));
    pthread_cancel(t);
  }
}

int main(int argc, char *argv[])

{
  client *c = new client();
  c->init();
  c->poi = c;

  printf("after open\n");

  printf("before write\n");

  c->handshake();

  c->SRSend();
  printf("countSend: %d\n", c->countSend);
  // c->SRSend();

  return 0;
}
