#!/usr/local/bin/pike

/* $Id: socktest.pike,v 1.4 1997/09/16 06:03:22 hubbe Exp $ */

import Stdio;
import String;

#if !efun(strerror)
#define strerror(X) ("ERRNO = "+(string)(X))
#endif

class Socket {
  import Stdio;
  import String;

  inherit File;

  object daemon=function_object(backtrace()[-2][2]);
  int num=daemon->num_running;

  string input_buffer="";

  string output_buffer="";
  string expected_data="";

  int input_finished;
  int output_finished;

  void cleanup()
  {
    if(input_finished && output_finished)
    {
      daemon->finish();
      close();
      destruct(this_object());
    }
  }

  void close_callback()
  {
    daemon->got_callback();
    if(input_buffer != expected_data)
    {
      werror("Failed to read complete data.\n");
      if(strlen(input_buffer) < 100)
      {
	werror(num+":Input buffer: "+input_buffer+"\n");
      }else{
	werror(num+":Input buffer: "+strlen(input_buffer)+" bytes.\n");
      }

      if(strlen(expected_data) < 100)
      {
	werror(num+":Expected data: "+expected_data+"\n");
      }else{
	werror(num+":Expected data: "+strlen(expected_data)+" bytes.\n");
      }

      exit(1);
    }

    input_finished++;
    cleanup();
  }

  void write_callback()
  {
    daemon->got_callback();
    if(strlen(output_buffer))
    {
      int tmp=write(output_buffer);
      if(tmp >= 0)
      {
	output_buffer=output_buffer[tmp..];
      } else {
	werror("Failed to write all data.\n");
	exit(1);
      }
    }else{
      set_write_callback(0);
      close("w");
      output_finished++;
      cleanup();
    }
  }

  void read_callback(mixed id, string foo)
  {
    daemon->got_callback();
    input_buffer+=foo;
  }

  varargs void create(object o)
  {
    daemon->got_callback();
    daemon->start();
    if(o)
    {
      o->set_id(0);
      assign(o);
      destruct(o);
    }else{
      if(!open_socket())
      {
	werror("Failed to open socket: "+strerror(errno())+"\n");
	exit(1);
      }
    }
    set_id(0);
    set_nonblocking(read_callback,write_callback,close_callback);
  }
};

void die()
{
  werror("No callbacks for 20 seconds!\n");
  exit(1);
}

int counter;

void got_callback()
{
  counter ++;
  if(!(counter & 0xf))
    predef::write(sprintf("%c\b","|/-\\" [ (counter++>>4) & 3 ]));
  remove_call_out(die);
  call_out(die,20);
}

int num_running;
int _tests;

void start()
{
  num_running++;
}


inherit Port : port1;
inherit Port : port2;

int portno1;
int portno2;

array(object(Socket)) stdtest()
{
  object sock,sock2;
  sock=Socket();
  sock->connect("127.0.0.1",portno2);
  sock2=port2::accept();
  if(!sock2)
  {
    werror("Accept returned 0\n");
    exit(1);
  }
  sock2=Socket(sock2);
  sock->output_buffer="foo";
  sock2->expected_data="foo";

  return ({sock,sock2});
}

object *spair(int type)
{
  object sock1,sock2;
  sock1=File();
  if(!type)
  {
    sock1->connect("127.0.0.1",portno2);
    sock2=port2::accept();
    if(!sock2)
    {
      werror("Accept returned 0\n");
      exit(1);
    }
  }else{
    sock2=sock1->pipe();
  }
  return ({sock1,sock2});
}

mixed keeper;

void finish()
{
  num_running--;
  if(!num_running)
  {
    werror("\n");

    object sock1, sock2, *socks;
    int tests;

    _tests++;
    switch(_tests)
    {
      case 1:
	werror("Testing dup & assign. ");
	sock1=stdtest()[0];
	sock1->assign(sock1->dup());
	break;
	
      case 2:
	werror("Testing accept. ");
	string data1=strmult("foobar",4711);
	for(int e=0;e<10;e++)
	{
	  sock1=Socket();
	  sock1->connect("127.0.0.1",portno1);
	  sock1->output_buffer=data1;
	}
	break;
	
      case 3:
	werror("Testing uni-directional shutdown on socket ");
	socks=spair(0);
	num_running=1;
	socks[1]->set_nonblocking(lambda() {},lambda(){},finish);
	socks[0]->close("w");
	keeper=socks;
	break;

      case 4:
	werror("Testing uni-directional shutdown on pipe ");
	socks=spair(1);
	num_running=1;
	socks[1]->set_nonblocking(lambda() {},lambda(){},finish);
	socks[0]->close("w");
	keeper=socks;
	break;

      case 5..26:
	tests=(_tests-2)*2;
	werror("Testing "+(tests*2)+" sockets. ");
	for(e=0;e<tests;e++) stdtest();
	break;

      case 27..48:
	tests=_tests-25;
	werror("Copying "+((tests/2)*(2<<(tests/2))*11)+" bytes of data on "+(tests&~1)+" "+(tests&1?"pipes":"sockets")+" ");
	for(int e=0;e<tests/2;e++)
	{
	  string data1=strmult("foobar",2<<(tests/2));
	  string data2=strmult("fubar",2<<(tests/2));
	  socks=spair(!(tests & 1));
	  sock1=socks[0];
	  sock2=socks[1];
	  if(!sock2)
	  {
	    werror("Failed to open pipe: "+strerror(sock1->errno())+".\n");
	    exit(1);
	  }
	  sock1=Socket(sock1);
	  sock2=Socket(sock2);
	  
	  sock1->output_buffer=data1;
	  sock2->expected_data=data1;
	  
	  sock2->output_buffer=data2;
	  sock1->expected_data=data2;
	}
	break;

    case 49:
      exit(0);
    }
  }
}

void accept_callback()
{
  object o=port1::accept();
  if(!o)
  {
    perror("Accept failed");
  }
  o=Socket(o);
  o->expected_data=strmult("foobar",4711);
}

int main()
{
  for(portno1=2001;portno1<65536;portno1++)
    if(port1::bind(portno1, accept_callback))
      break;

  if(portno1==65536) perror("Bind failed.\n");

  for(portno2=2001;portno2<65536;portno2++)
    if(port2::bind(portno2))
      break;

  if(portno2==65536) perror("Bind failed.\n");

  werror("Doing simple tests. ");
  stdtest();
  return -1;
}
