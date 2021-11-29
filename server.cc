#include "type.hpp"
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class server {
public:
  tcp_struct body;
  int fd_write, fd_read;
  int ret_write, ret_read;
  int start;
  int recieve_window[1000];
  message send;
  message recieve;
  void init();
  void handshake();
  void sendMessage();
  void readMessage();
  void printSend();
  void printRecieve();
  void SRLoopRead();
  void GBNLoopRead();
};

void server::SRLoopRead() {
  start = 0;
  while (body.type != CLOSE) {
    readMessage();
    printRecieve();
    if (rand() % 100 < 90) {
      recieve_window[recieve.seq] = 1;
      send.ack = recieve.seq;
      while (recieve_window[start] != 0) {
        start++;
      }
      send.seq = recieve.ack;
      send.ACK = 1;
      send.type = msg;
    } else {
      send.type = msgLost;
    }
    sendMessage();
    printSend();
  }
}

void server::GBNLoopRead() {
  while (body.type != CLOSE) {
    readMessage();
    printRecieve();
    if (rand() % 100 < 70) {
      send.ack = start;
      if (recieve.seq == start + 1) {
        printf("start: %d\n", start);
        send.ack = start + 1;
        start += 1;
      }
      send.ACK = 1;
      send.seq = recieve.ack + 1;
      sendMessage();
      printSend();
    }
  }
}

void server::printSend() {
  printf("send.ACK : %d\n send.ack : %d\n send.buf : %s\n send.seq : %d\n "
         "send.syn : %d\n send.type : %d\n",
         send.ACK, send.ack, send.buf, send.seq, send.syn, send.type);
}
void server::printRecieve() {
  printf("recieve.ACK : %d\n recieve.ack : %d\n recieve.buf : %s\n recieve.seq "
         ": %d\n recieve.syn : %d\n recieve.type : %d\n",
         recieve.ACK, recieve.ack, recieve.buf, recieve.seq, recieve.syn,
         recieve.type);
}

void server::init() {
  ret_read = mkfifo("server_fifo_pipe", 0666);  //创建命名管道
  ret_write = mkfifo("client_fifo_pipe", 0666); //创建命名管道
  if (ret_write != 0) {
    perror("mkfifo server");
  }

  if (ret_read != 0) {
    perror("mkfifo client");
  }

  printf("before open\n");

  fd_read = open("server_fifo_pipe", O_RDONLY); //等着只写;
  if (fd_read < 0) {
    perror("open read fifo");
  }

  fd_write = open("client_fifo_pipe", O_WRONLY); //等着只读;
  if (fd_write < 0) {
    perror("open wirte fifo");
  }
  start = -1;
}
void server::handshake() {
  readMessage();
  printRecieve();
  if (recieve.syn == 1) {
    send.syn = 1;
    send.ACK = 1;
    send.seq = 0;
    send.ack = recieve.seq + 1;
  }
  body.type = SYN;
  sendMessage();
  printf("send success");
  printSend();
  readMessage();
  printRecieve();
  body.type = ESTABLISH;
  printf("connect success");
}

void server::sendMessage() { write(fd_write, &send, sizeof(send)); }
void server::readMessage() { read(fd_read, &recieve, sizeof(recieve)); }

using namespace std;

int main() {
  server *s = new server();
  s->init();
  s->handshake();
  s->SRLoopRead();

  return 0;
}
