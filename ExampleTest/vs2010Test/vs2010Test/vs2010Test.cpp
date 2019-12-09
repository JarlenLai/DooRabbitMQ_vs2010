// vs2010temp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <process.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "MessageQueue/RabbitMQ.h"
#include "MessageQueue/MessageBody.h"
#include <ctime>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>


#pragma comment(lib,"ws2_32.lib")
using namespace std;

class CSync
{
private:
	CRITICAL_SECTION  m_cs;

public:
	CSync()  { ZeroMemory(&m_cs,sizeof(m_cs)); InitializeCriticalSection(&m_cs); }
	~CSync()  { DeleteCriticalSection(&m_cs); }
	//----
	inline void       Lock()   { EnterCriticalSection(&m_cs);  }
	inline void       Unlock() { LeaveCriticalSection(&m_cs);  }
};

HANDLE stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
HANDLE do_event = CreateEvent(NULL, FALSE, FALSE, NULL);

CSync g_sync;


static UINT __stdcall WaitAck(LPVOID param)
{
	if (param == NULL) return 0;

	amqp_connection_state_t conn = *((amqp_connection_state_t *)param);

	amqp_frame_t frame;
	amqp_rpc_reply_t ret;
	int statue;
	while( (statue = amqp_simple_wait_frame(conn, &frame)) == AMQP_STATUS_OK)
	{
	
    /* Publish消息后需要在当前通道上监听返回的信息，来判断消息是否成功投递
     * 这里要息根据投递消息的方式来过滤判断几个方法
     */
 
    if (AMQP_FRAME_METHOD == frame.frame_type) {
      amqp_method_t method = frame.payload.method;
      fprintf(stdout, "method.id=%08X,method.name=%s\n",method.id, amqp_method_name(method.id));
      switch (method.id) {
        case AMQP_BASIC_ACK_METHOD:
          /* if we've turned publisher confirms on, and we've published a message
           * here is a message being confirmed
           */
          {
            amqp_basic_ack_t *s;
            s = (amqp_basic_ack_t *) method.decoded;
            //fprintf(stdout, "Ack.delivery_tag=%d\n", s->delivery_tag);
            //fprintf(stdout, "Ack.multiple=%d\n", s->multiple);
			//if (s->delivery_tag < MESSAGE_COUNT) goto LOOP;
          }
		  
          break;
 
        case AMQP_BASIC_NACK_METHOD:
          /* if we've turned publisher confirms on, and we've published a message
           * here is a message not being confirmed
           */
          {
            amqp_basic_nack_t *s;
            s = (amqp_basic_nack_t *) method.decoded;
            //fprintf(stdout, "NAck.delivery_tag=%d\n", s->delivery_tag);
            //fprintf(stdout, "NAck.multiple=%d\n", s->multiple);
            //fprintf(stdout, "NAck.requeue=%d\n", s->requeue);
          }
 
          break;
 
        case AMQP_BASIC_RETURN_METHOD:
          /* if a published message couldn't be routed and the mandatory flag was set
           * this is what would be returned. The message then needs to be read.
           */
          {
            amqp_message_t message;
            amqp_basic_return_t *s;
            char str[1024];
            s = (amqp_basic_return_t *) method.decoded;
            //fprintf(stdout, "Return.reply_code=%d\n", s->reply_code);
            strncpy(str, (const char *)s->reply_text.bytes, s->reply_text.len); str[s->reply_text.len] = 0;
            //fprintf(stdout, "Return.reply_text=%s\n", str);
 
            ret = amqp_read_message(conn, frame.channel, &message, 0);
            if (AMQP_RESPONSE_NORMAL != ret.reply_type) {
              return 1;
            }
            strncpy(str, (const char *)message.body.bytes, message.body.len); str[message.body.len] = 0;
            fprintf(stdout, "Return.message=%s\n", str);
 
            amqp_destroy_message(&message);
          }
 
          break;
 
        case AMQP_CHANNEL_CLOSE_METHOD:
          /* a channel.close method happens when a channel exception occurs, this
           * can happen by publishing to an exchange that doesn't exist for example
           *
           * In this case you would need to open another channel redeclare any queues
           * that were declared auto-delete, and restart any consumers that were attached
           * to the previous channel
           */
          break;
 
        case AMQP_CONNECTION_CLOSE_METHOD:
          /* a connection.close method happens when a connection exception occurs,
           * this can happen by trying to use a channel that isn't open for example.
           *
           * In this case the whole connection must be restarted.
           */
          break;
 
        default:
          fprintf(stderr ,"An unexpected method was received %d\n", frame.payload.method.id);
          return 1;

    }
  }

 }
	cout<<"amqp_simple_wait_frame not ok and quit  "<<statue<<endl;
	SetEvent(stop_event);
}

//移植过来的不用管
int32_t die_on_amqp_error(amqp_rpc_reply_t x, string context,string &ErrorReturn)
{
	char rtnmsg[1024];
	switch (x.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		return 1;

	case AMQP_RESPONSE_NONE:
		sprintf(rtnmsg, "%s: missing RPC reply type!\n", context.c_str());
		break;

	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		sprintf(rtnmsg, "%s: %s\n", context.c_str(), amqp_error_string2(x.library_error));
		break;

	case AMQP_RESPONSE_SERVER_EXCEPTION:
		switch (x.reply.id) {
		case AMQP_CONNECTION_CLOSE_METHOD: {
			amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
			sprintf(rtnmsg, "%s: server connection error %d, message: %.*s\n",
				context.c_str(),
				m->reply_code,
				(int) m->reply_text.len, (char *) m->reply_text.bytes);
			break;
										   }
		case AMQP_CHANNEL_CLOSE_METHOD: {
			amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
			sprintf(rtnmsg, "%s: server channel error %d, message: %.*s\n",
				context.c_str(),
				m->reply_code,
				(int) m->reply_text.len, (char *) m->reply_text.bytes);
			break;
										}
		default:
			sprintf(rtnmsg, "%s: unknown server error, method id 0x%08X\n", context, x.reply.id);
			break;
		}
		break;
	}
	ErrorReturn = rtnmsg;
	return -1;
}

//移植过来的不用管
int32_t die_on_error(int x, string context,string &ErrorReturn) {
	char rtnmsg[1024];
	if (x < 0) {
		sprintf(rtnmsg, "%s: %s\n", context.c_str(), amqp_error_string2(x));
		ErrorReturn = rtnmsg;
		return -1;
	}
}

int _tmain1(int argc, _TCHAR* argv[])
{
  string hostname;
  int port, status;
  char const *exchange;
  string routingkey;
  string messagebody;
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;
  amqp_channel_t chan = 1;

  hostname = "127.0.0.1";
  port = 5672;
  exchange = "jarlen_temp";
  routingkey = "";
  messagebody = "test ack public message";
 
  conn = amqp_new_connection();
 
  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    cout<<("creating TCP socket")<<endl;
  }
 
  status = amqp_socket_open(socket, hostname.c_str(), port);
  if (status) {
    cout<<("opening TCP socket")<<endl;
  }

  string ErrorReturn;
 
  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"), "Logging in", ErrorReturn);
  amqp_channel_open(conn, 1);
 amqp_channel_open(conn, 2);
 die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel", ErrorReturn);

  amqp_confirm_select(conn, 1);  //在通道上打开Publish确认

  //开启线程
  HANDLE h2 = (HANDLE)_beginthreadex(NULL, 0, &WaitAck, &conn, 0, NULL);
  if (NULL != h2)
  {
	  cout<<"Start Create WaitAck success"<<endl;
	  ::CloseHandle(h2);
  }
  else
  {
	   cout<<"Start Create WaitAck error"<<endl;
  }

  amqp_basic_properties_t props;
  props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props.content_type = amqp_cstring_bytes("text/plain");
  props.delivery_mode = 2; /* persistent delivery mode */

#ifndef MESSAGE_COUNT1
  #define  MESSAGE_COUNT1 5
#endif

  unsigned int uiCount = 0;
 
  for (unsigned int i = 0; i < MESSAGE_COUNT1; i++)
  {
	ostringstream oss;
	oss << messagebody <<i;
	string str = oss.str();

	int ret = 0;
    die_on_error(ret = amqp_basic_publish(conn,
                                    chan,
                                    amqp_cstring_bytes(exchange),
                                    amqp_cstring_bytes(routingkey.c_str()),
                                    1,   //mandatory标志位,消息不能到达队列则返回basic.return
                                    0,   //immediate标志位,消息不能到达消费者返回basic.return
                                    &props,
                                    amqp_cstring_bytes(str.c_str())),
                 "Publishing", ErrorReturn);

  }

  while(1)
  {
	  int a;
	  cin>>a;
	  for (unsigned int i = 0; i < MESSAGE_COUNT1; i++)
	  {
		  ostringstream oss;
		  oss << messagebody <<i;
		  string str = oss.str();

		  int ret = 0;
		  die_on_error(ret = amqp_basic_publish(conn,
			  chan,
			  amqp_cstring_bytes(exchange),
			  amqp_cstring_bytes(routingkey.c_str()),
			  1,   //mandatory标志位,消息不能到达队列则返回basic.return
			  0,   //immediate标志位,消息不能到达消费者返回basic.return
			  &props,
			  amqp_cstring_bytes(str.c_str())),
			  "Publishing", ErrorReturn);

	  }
  }


  WaitForSingleObject(stop_event, INFINITE);
  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel", ErrorReturn);
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection", ErrorReturn);
  die_on_error(amqp_destroy_connection(conn), "Ending connection", ErrorReturn);
 
  system("pause");
  return 0;

}


static UINT __stdcall WaitAck2(LPVOID param)
{
	if (param == NULL) return 0;

	CRabbitMQ *mq = (CRabbitMQ *)param;

	string Err;
	string Mes;
	while(1)
	{
		g_sync.Lock();
		if(!mq->IsConnect(Err))
		{
			cout<<"mq is not connect"<<endl;
			g_sync.Unlock();
			Sleep(1000);
			continue;
		}
		g_sync.Unlock();

		if(mq->publish_ack_wait(Err, Mes)<0)
		{
			cout<<"Err Reason: "<<Err<<", Message: "<<Mes<<endl;
		}
		

		Sleep(10);
		cout<<"do"<<endl;
	}

}

int _tmain(int argc, _TCHAR* argv[])
{
	string hostname;
	int port, status;
	char const *exchange;
	string routingkey;
	string messagebody;
	amqp_socket_t *socket = NULL;
	amqp_connection_state_t conn;
	amqp_channel_t chan = 1;

	hostname = "127.0.0.1";
	port = 5672;
	exchange = "jarlen_temp";
	routingkey = "";
	messagebody = "test ack public message";

	string ErrorReturn;
	string FailMessage;

	CRabbitMQ mq(hostname, port, "guest", "guest", true);
	if (mq.Connect(ErrorReturn) < 0)
	{
		cout<<"mq connect err "<<ErrorReturn<<endl;
	}

	CExchange ex(exchange);
	if(mq.exchange_declare(ex, ErrorReturn)<0)
	{
		cout<<"mq declare exchange  err "<<  ErrorReturn<<endl;
	}


	//开启线程
	HANDLE h2 = (HANDLE)_beginthreadex(NULL, 0, &WaitAck2, &mq, 0, NULL);
	if (NULL != h2)
	{
		cout<<"Start Create WaitAck2 success"<<endl;
		::CloseHandle(h2);
	}
	else
	{
		cout<<"Start Create WaitAck2 error"<<endl;
	}



#ifndef MESSAGE_COUNT
#define  MESSAGE_COUNT 2
#endif

	vector<CMessage> vecMe;

	for (unsigned int i = 0; i < MESSAGE_COUNT; i++)
	{
		ostringstream oss;
		oss << messagebody <<i;
		string str = oss.str();
		if (i == MESSAGE_COUNT/2) 
		{
			CMessage me(oss.str(), "1234");
			vecMe.push_back(me);
		}
		else 
		{
			CMessage me(oss.str());
			vecMe.push_back(me);
		}
	}

	while(1)
	{
		int a;
		if((a = mq.publish(vecMe, "jarlen_temp", ErrorReturn))<0)
		{
			g_sync.Lock();
			cout<<"a="<<a<<endl;
			mq.Connect(ErrorReturn);
			g_sync.Unlock();
		}

		Sleep(1000);
	}
	
	system("pause");
	return 0;
}

int public_ack(vector<CMessage> &message, amqp_connection_state_t conn, char const *exchange,string &routekey,	string &ErrorReturn, string &FailMessage)
{
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	CExchange ex(exchange);
	amqp_bytes_t _exchange = amqp_cstring_bytes(ex.m_name.c_str());
	amqp_bytes_t _rout_key = amqp_cstring_bytes(routekey.c_str());
	vector<CMessage>::iterator it;
	for(it=message.begin(); it!=message.end(); ++it) 
	{
		amqp_bytes_t message_bytes;
		message_bytes.len =(*it).m_data.length();
		message_bytes.bytes =(void *)((*it).m_data.c_str());
		props.content_type = amqp_cstring_bytes((*it).m_type.c_str());
		props.delivery_mode = (*it).m_durable; /* persistent delivery mode */

		if(amqp_basic_publish(conn,
							1,
							_exchange,
							_rout_key,
							1,//mandatory标志位,消息不能到达队列则返回basic.return
							0,//immediate标志位,消息不能到达消费者返回basic.return
							&props,message_bytes)!=0)
		{
			if(1!=die_on_amqp_error(amqp_get_rpc_reply(conn),"amqp_basic_publish",ErrorReturn) )
			{
				return -1;
			}
		}
	}


	amqp_frame_t frame;
	amqp_rpc_reply_t ret;
	timeval timeout;//一秒超时
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	int statue;
	int returnRet = 0;
	uint64_t Seq = 0;
	uint64_t MessageCount = message.size();

	Loop:
	if ((statue = amqp_simple_wait_frame_noblock(conn, &frame, &timeout)) != AMQP_STATUS_OK) 
	{
	  ErrorReturn = "wait ack public message return maybe timeout";
	  FailMessage = ErrorReturn;
      return -1;
    }
 
    if (AMQP_FRAME_METHOD == frame.frame_type)
	{
      amqp_method_t method = frame.payload.method;
      fprintf(stdout, "method.id=%08X,method.name=%s\n",method.id, amqp_method_name(method.id));
      switch (method.id) 
	  {
        case AMQP_BASIC_ACK_METHOD:
          /* if we've turned publisher confirms on, and we've published a message
           * here is a message being confirmed
           */
          {
            amqp_basic_ack_t *s;
            s = (amqp_basic_ack_t *) method.decoded;
            //fprintf(stdout, "Ack.delivery_tag=%d\n", s->delivery_tag);
            //fprintf(stdout, "Ack.multiple=%d\n", s->multiple);
			Seq = s->delivery_tag;
			//if (MessageCount > 1)//确认到的数量和发送的数量不符合继续等
			{
				goto Loop;
			}
		  }
 
          break;
 
        case AMQP_BASIC_NACK_METHOD:
          /* if we've turned publisher confirms on, and we've published a message
           * here is a message not being confirmed
           */
          {
			returnRet = -1;
			ErrorReturn = "basic.nack";
            amqp_basic_nack_t *s;
			char str[502] = {0};
            s = (amqp_basic_nack_t *) method.decoded;
			sprintf_s(str, "ErrReson: basic.nack, ReceveMessage: NAck.delivery_tag=%d\r\n", s->delivery_tag);
			FailMessage += str;
			fprintf(stdout, "NAck.delivery_tag=%d\n", s->delivery_tag);
			Seq = s->delivery_tag;
			if (MessageCount > 1 && Seq < MessageCount)//确认到的数量和发送的数量不符合继续等
			{
				goto Loop;
			}
          }
 
          break;
 
        case AMQP_BASIC_RETURN_METHOD:
          /* if a published message couldn't be routed and the mandatory flag was set
           * this is what would be returned. The message then needs to be read.
           */
          {
			returnRet = -1;
			ErrorReturn = "basic.return";
            amqp_message_t message;
            amqp_basic_return_t *s;
			char str1[1024 * 5] = {0};
			char str2[1024 * 6] = {0};
			char szRouteKey[20] = {0};
			strncpy_s(szRouteKey, (const char*)_rout_key.bytes, _rout_key.len); szRouteKey[_rout_key.len] = 0;

            s = (amqp_basic_return_t *) method.decoded;
            strncpy_s(str1, (const char*)s->reply_text.bytes, s->reply_text.len); str1[s->reply_text.len] = 0;
			sprintf_s(str2, "ErrReason: %s basic.return, ", str1);
			FailMessage += str2;
			memset(str1, 0, sizeof(str1));
			memset(str2, 0, sizeof(str2));

            ret = amqp_read_message(conn, frame.channel, &message, 0);
            if (AMQP_RESPONSE_NORMAL != ret.reply_type) {
				sprintf_s(str1, "RouteKey:%s, ErrMessage: amqp_read_message err\r\n", szRouteKey);
				FailMessage += str1;//记录MQ处理失败的消息以及原因
              return -1;
            }
            strncpy_s(str1, (const char*)message.body.bytes, message.body.len); str1[message.body.len] = 0; 

			sprintf_s(str2, "RouteKey:%s, PubilcMessage: %s\r\n", szRouteKey, str1);
            FailMessage += str2;//记录MQ处理失败的消息以及原因

			//重新发送消息(注意props变量用来上面的最后一次的配置,目前来说都是一样的)
			if(amqp_basic_publish(conn,1,_exchange,_rout_key,0,0,&props,message.body)!=0)
			{
				die_on_amqp_error(amqp_get_rpc_reply(conn),"amqp_basic_publish",ErrorReturn);
			}
            amqp_destroy_message(&message);

			if (MessageCount > 1)//确认到的数量和发送的数量不符合继续等
			{
				goto Loop;
			}
          }
 
          break;
 
        case AMQP_CHANNEL_CLOSE_METHOD:
          /* a channel.close method happens when a channel exception occurs, this
           * can happen by publishing to an exchange that doesn't exist for example
           *
           * In this case you would need to open another channel redeclare any queues
           * that were declared auto-delete, and restart any consumers that were attached
           * to the previous channel
           */
			returnRet = -1;
			FailMessage += "ErrReason: channel.close \r\n";
			ErrorReturn = "channel.close";
          break;
 
        case AMQP_CONNECTION_CLOSE_METHOD:
          /* a connection.close method happens when a connection exception occurs,
           * this can happen by trying to use a channel that isn't open for example.
           *
           * In this case the whole connection must be restarted.
           */
			returnRet = -1;
			FailMessage += "ErrReason: connection.close\r\n";
			ErrorReturn = "connection.close";
          break;
 
        default:
		   returnRet = -1;
		   FailMessage += "Ack pubic message an unexpected method was received\r\n";
		   ErrorReturn = "Ack pubic message an unexpected method was received";
          //sprintf_s(err ,"Ack pubic message an unexpected method was received %d\n", frame.payload.method.id);
		   break;
    }
  }
}

int _tmain3(int argc, _TCHAR* argv[])
{
	string hostname;
	int port, status;
	char const *exchange;
	string routekey;
	string messagebody;
	amqp_socket_t *socket = NULL;
	amqp_connection_state_t conn;
	amqp_channel_t chan = 1;

	hostname = "127.0.0.1";
	port = 5672;
	exchange = "jarlen_temp";
	routekey = "";
	messagebody = "test ack public message";

	conn = amqp_new_connection();

	socket = amqp_tcp_socket_new(conn);
	if (!socket) {
		cout<<("creating TCP socket")<<endl;
	}

	status = amqp_socket_open(socket, hostname.c_str(), port);
	if (status) {
		cout<<("opening TCP socket")<<endl;
	}

	string ErrorReturn;
	string FailMessage;

	die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 30, AMQP_SASL_METHOD_PLAIN, "guest", "guest"), "Logging in", ErrorReturn);
	amqp_channel_open(conn, 1);
	amqp_confirm_select(conn, 1);  //在通道上打开Publish确认
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel", ErrorReturn);


#ifndef MESSAGE_COUNT3
#define  MESSAGE_COUNT3 1
#endif

	vector<CMessage> message;

	for (unsigned int i = 0; i < MESSAGE_COUNT3; i++)
	{
		ostringstream oss;
		oss << messagebody <<i;
		string str = oss.str();
		if (i == MESSAGE_COUNT/2) 
		{
			CMessage me(oss.str(), "1234");
			message.push_back(me);
		}
		else 
		{
			CMessage me(oss.str());
			message.push_back(me);
		}
	}


	public_ack(message, conn, exchange, routekey, ErrorReturn, FailMessage);

	cout<<endl<<"2"<<endl;
	public_ack(message, conn, exchange, routekey, ErrorReturn, FailMessage);

	system("pause");
	return 0;
}

