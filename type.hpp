#pragma ones

#include <iostream>
#include <queue>

using namespace std;

enum message_type { msg, ack, resend, msgLost };

enum tcp_state { normal, lost };
enum tcp_type { LISTEN, SYN, ESTABLISH, CLOSE };

struct message {
  int seq;
  message_type type;
  int ack;
  int ACK;
  int syn;
  char buf[1024];
};

class tcp_struct {
public:
  int type;
  int cur;
  int ack;
  int window_size;
  tcp_state state;
};
