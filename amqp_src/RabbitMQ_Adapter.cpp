
#include "RabbitMQ_Adapter.h"
#include "amqp_tcp_socket.h"
#include <string>
#include <vector>
#include <iostream>
#include <vector>
using namespace std;
 
//定义全局变量
string err = "";
//获取全局变量的函数
extern Publish string& GetErr()
{
	return err;
}

CRabbitMQ_Adapter:: CRabbitMQ_Adapter(string HostName, uint32_t port,string usr,string psw, bool useConChan)
{
	this->m_hostName = HostName;
	this->m_channel = 1; //默认用1号通道，通道无所谓 
	this->m_port = port;
	this->m_bUseConfirmChan = useConChan;

	m_sock = NULL;
    m_conn = NULL;
	m_user = usr;
	m_psw =  psw;
	m_exchange = NULL;
	m_queue = NULL;
	
	//m_exchange = new CExchange("cfdefault.direct",0,"direct");
	//m_queue    = new CQueue("cfdefaultqueue",0);
}

CRabbitMQ_Adapter::~CRabbitMQ_Adapter()
{
	this->m_hostName ="";
	this->m_port = 0;
	this->m_bUseConfirmChan = false;
	string errmsg;
	if(NULL!=m_conn) 
	{
		Disconnect(errmsg);  
	    m_conn = NULL;
	}
	if(m_exchange != NULL)
	{
		delete m_exchange;
		m_exchange = NULL;
	}
	if(m_queue != NULL)
	{
		delete m_queue;
		m_queue = NULL;
	}
}

int32_t CRabbitMQ_Adapter::Disconnect(string &ErrorReturn)
{
	if(NULL!=m_conn) 
	{
		if(1!=AssertError(amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS), "Closing channel",ErrorReturn))
			return -1;
		if(1!=AssertError(amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS), "Closing connection",ErrorReturn))
			return -1;
		
		if(amqp_destroy_connection(m_conn)<0)
			return -1;
		
		m_conn = NULL;
	}
	return 0;
}

int32_t CRabbitMQ_Adapter::Connect(string &ErrorReturn)
{
	if (NULL != m_conn)
	{
		if(amqp_destroy_connection(m_conn) < 0)
		{
			ErrorReturn = "无法释放连接MQ的资源,请确认";
			return -1;
		}

		m_conn = NULL;
	}

	m_conn = amqp_new_connection();
	if(NULL==m_conn)
	{
		ErrorReturn = "无法获得连接";
		return -2;
	}
	m_sock =  amqp_tcp_socket_new(m_conn);
	if(NULL==m_sock)
	{
		ErrorReturn = "无法获得套接字";
		return -3;
	}

	int status = amqp_socket_open(m_sock,m_hostName.c_str(),m_port);
	if(status<0)
	{
		ErrorReturn = "无法连接目标主机";
		return -4;
	}
	
	if(1 == AssertError(amqp_login(m_conn, "/", 0, 131072, 30, AMQP_SASL_METHOD_PLAIN,m_user.c_str(),m_psw.c_str()),"Loging in",ErrorReturn))
	{
		amqp_channel_open(m_conn, m_channel);
		if (m_bUseConfirmChan)
		{
			amqp_confirm_select(m_conn, m_channel);
		}
		return 0;
	}
	else
		return -5;
};

//step1 declare an exchange
 int32_t CRabbitMQ_Adapter::exchange_declare(CExchange &exchange,string &ErrorReturn)
{
	//创建exchange
	
	amqp_bytes_t _exchange = amqp_cstring_bytes(exchange.m_name.c_str());
	amqp_bytes_t _type = amqp_cstring_bytes(exchange.m_type.c_str());
	int32_t  _passive=exchange.m_passive;    
	int32_t  _durable=exchange.m_durable;      //交换机是否持久化
	amqp_exchange_declare(m_conn,m_channel,_exchange,_type,_passive,_durable, 0, 0, amqp_empty_table);
	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"exchange_declare",ErrorReturn))
	{
		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	//amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
	if (this->m_exchange != NULL)//重新声明Exchange时需释放上一个的资源
	{
		delete this->m_exchange;
		this->m_exchange = NULL;
	}
	
	this->m_exchange = new CExchange(exchange);
	return 0;
}

 //step2 declare a queue
 int32_t CRabbitMQ_Adapter::queue_declare(CQueue &queue,string &ErrorReturn)
{
	//amqp_channel_open(m_conn, m_channel);
	amqp_bytes_t _queue =amqp_cstring_bytes(queue.m_name.c_str());
	int32_t _passive = queue.m_passive;
	int32_t _durable = queue.m_durable; 
	int32_t _exclusive = queue.m_exclusive;
	int32_t _auto_delete =queue.m_auto_delete;
    amqp_queue_declare(m_conn,m_channel,_queue,_passive,_durable,_exclusive,_auto_delete,amqp_empty_table);
	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"queue_declare",ErrorReturn) )
	{
		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	
	if (this->m_queue != NULL)//重新声明Queue时需释放上一个的资源
	{
		delete this->m_queue;
		this->m_queue = NULL;
	}

	this->m_queue = new CQueue(queue);
	//amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
	return 0;
 }

 //step3 bind
 int32_t CRabbitMQ_Adapter::queue_bind(CQueue &queue,CExchange &exchange,const string bind_key,string &ErrorReturn)
 {
	 //amqp_channel_open(m_conn, m_channel);
	 amqp_bytes_t _queue    = amqp_cstring_bytes(queue.m_name.c_str());
	 amqp_bytes_t _exchange = amqp_cstring_bytes(exchange.m_name.c_str());
	 amqp_bytes_t _routkey  = amqp_cstring_bytes(bind_key.c_str());
	 amqp_queue_bind(m_conn,m_channel,_queue,_exchange,_routkey,amqp_empty_table);
	 if(1!=AssertError(amqp_get_rpc_reply(m_conn),"queue_bind",ErrorReturn) )
	 {
		 amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		 return -1;
	 }
	 //amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
	 return 0;
 }

int32_t  CRabbitMQ_Adapter::queue_unbind(CQueue &queue,CExchange &exchange,const string bind_key,string &ErrorReturn)
 {
	 //amqp_channel_open(m_conn, m_channel);
	 amqp_bytes_t _queue    = amqp_cstring_bytes(queue.m_name.c_str());
	 amqp_bytes_t _exchange = amqp_cstring_bytes(exchange.m_name.c_str());
	 amqp_bytes_t _routkey  = amqp_cstring_bytes(bind_key.c_str());
	 amqp_queue_unbind(m_conn,m_channel,_queue,_exchange,_routkey,amqp_empty_table);
	 if(1!=AssertError(amqp_get_rpc_reply(m_conn),"queue_unbind",ErrorReturn) )
	 {
		 amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		 return -1;
	 }
	 //amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
	 return 0;
 }

//step 4 publish message 
int32_t CRabbitMQ_Adapter::publish(vector<CMessage> &message,string routekey,string &ErrorReturn)
{
	
	if(NULL ==m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
	
// 	amqp_channel_open(m_conn, m_channel);
// 	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"open channel",ErrorReturn) )
// 	{
// 		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
// 		return -1;
// 	}

   amqp_basic_properties_t props;
   vector<CMessage>::iterator it;
   for(it=message.begin(); it!=message.end(); ++it) 
   {
	  amqp_bytes_t message_bytes;
	  message_bytes.len =(*it).m_data.length();
	  message_bytes.bytes =(void *)((*it).m_data.c_str());
	  props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	  props.content_type = amqp_cstring_bytes((*it).m_type.c_str());
	  props.delivery_mode = (*it).m_durable; /* persistent delivery mode */
	  
	  amqp_bytes_t _exchange = amqp_cstring_bytes(this->m_exchange->m_name.c_str());
	  amqp_bytes_t _rout_key = amqp_cstring_bytes(routekey.c_str());
	  //printf("message: %.*s\n",(int)message_bytes.len,(char *)message_bytes.bytes);
	  
	  if(amqp_basic_publish(m_conn,m_channel,_exchange,_rout_key,0,0,&props,message_bytes)!=0)
	  {	  
		  //printf("发送消息失败。");
		  if(1!=AssertError(amqp_get_rpc_reply(m_conn),"amqp_basic_publish",ErrorReturn) )
		  {
			  amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);//感觉这里会有梗(是否不关闭会更好一点呢)
			  return -1;
		  }
	  }
	//printf("message: %.*s\n",(int)message_bytes.len,(char *)message_bytes.bytes);
   }
  // amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
   return 0;
}

int32_t CRabbitMQ_Adapter::publish(CMessage &message,string routkey,string &ErrorReturn)
{
	vector<CMessage> msg;
	msg.push_back(message);
	return publish(msg,routkey,ErrorReturn);
}

int32_t CRabbitMQ_Adapter::publish(const string &message,string routekey,string &ErrorReturn)
{
	CMessage msg(message);
	return publish(msg,routekey,ErrorReturn);

}

int32_t CRabbitMQ_Adapter::publish_ack(vector<CMessage> &message,string routekey,string &ErrorReturn, string &FailMessage)
{
	if(NULL ==m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}

	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	amqp_bytes_t _exchange = amqp_cstring_bytes(this->m_exchange->m_name.c_str());
	amqp_bytes_t _rout_key = amqp_cstring_bytes(routekey.c_str());

	vector<CMessage>::iterator it;
	for(it=message.begin(); it!=message.end(); ++it) 
	{
		amqp_bytes_t message_bytes;
		message_bytes.len =(*it).m_data.length();
		message_bytes.bytes =(void *)((*it).m_data.c_str());
		props.content_type = amqp_cstring_bytes((*it).m_type.c_str());
		props.delivery_mode = (*it).m_durable; /* persistent delivery mode */

		if(amqp_basic_publish(m_conn,
							m_channel,
							_exchange,
							_rout_key,
							1,//mandatory标志位,消息不能到达队列则返回basic.return
							0,//immediate标志位,消息不能到达消费者返回basic.return
							&props,message_bytes)!=0)
		{
			if(1!=AssertError(amqp_get_rpc_reply(m_conn),"amqp_basic_publish",ErrorReturn) )
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
	if ((statue = amqp_simple_wait_frame_noblock(m_conn, &frame, &timeout)) != AMQP_STATUS_OK) 
	{
	  ErrorReturn = "wait ack public message return maybe timeout";
	  FailMessage = ErrorReturn;
      return -1;
    }
 
    if (AMQP_FRAME_METHOD == frame.frame_type)
	{
      amqp_method_t method = frame.payload.method;
      //fprintf(stdout, "method.id=%08X,method.name=%s\n",method.id, amqp_method_name(method.id));
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
			if (MessageCount > 1 &&  Seq < MessageCount)//确认到的数量和发送的数量不符合继续等
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
			//fprintf(stdout, "NAck.delivery_tag=%d\n", s->delivery_tag);
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

            ret = amqp_read_message(m_conn, frame.channel, &message, 0);
            if (AMQP_RESPONSE_NORMAL != ret.reply_type) {
				sprintf_s(str1, "RouteKey:%s, ErrMessage: amqp_read_message err\r\n", szRouteKey);
				FailMessage += str1;//记录MQ处理失败的消息以及原因
              return -1;
            }
            strncpy_s(str1, (const char*)message.body.bytes, message.body.len); str1[message.body.len] = 0; 

			sprintf_s(str2, "RouteKey:%s, PubilcMessage: %s\r\n", szRouteKey, str1);
            FailMessage += str2;//记录MQ处理失败的消息以及原因

			//重新发送消息(注意props变量用来上面的最后一次的配置,目前来说都是一样的)
			if(amqp_basic_publish(m_conn,m_channel,_exchange,_rout_key,0,0,&props,message.body)!=0)
			{
				AssertError(amqp_get_rpc_reply(m_conn),"amqp_basic_publish",ErrorReturn);
			}
            amqp_destroy_message(&message);

			if (MessageCount > 1 && Seq < MessageCount)//确认到的数量和发送的数量不符合继续等
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

	return returnRet;
}

int32_t CRabbitMQ_Adapter::publish_ack(CMessage &message,string routkey,string &ErrorReturn, string &FailMessage)
{
	vector<CMessage> msg;
	msg.push_back(message);
	return publish_ack(msg,routkey,ErrorReturn, FailMessage);
}

int32_t CRabbitMQ_Adapter::publish_ack(const string &message,string routekey,string &ErrorReturn, string &FailMessage)
{
	CMessage msg(message);
	return publish_ack(msg,routekey,ErrorReturn, FailMessage);

}

int32_t CRabbitMQ_Adapter::getMessageCount(const CQueue &queue,string &ErrorReturn)
{

	int TotalMessage = -1 ;
	if(NULL ==m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
	
	
	amqp_channel_open(m_conn, m_channel);
	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"open channel",ErrorReturn) )
	{
		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_bytes_t _queue =amqp_cstring_bytes(queue.m_name.c_str());
	int32_t _passive = queue.m_passive;
	int32_t _durable = queue.m_durable; 
	int32_t _exclusive = queue.m_exclusive;
	int32_t _auto_delete =queue.m_auto_delete;
    amqp_queue_declare_ok_t *p = amqp_queue_declare(m_conn,m_channel,_queue,_passive,_durable,_exclusive,_auto_delete,amqp_empty_table);
	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"Get Message count",ErrorReturn) )
	{
		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
	TotalMessage = p->message_count;
	return TotalMessage;

}

int32_t CRabbitMQ_Adapter::getMessageCount(const string &queuename,string &ErrorReturn)
{
	CQueue queue(queuename,1);
	return getMessageCount(queue);
}

bool CRabbitMQ_Adapter::IsConnect(string &ErrorReturn)
{
	if(NULL == m_conn)
	{
		ErrorReturn = "还未创建连接";
		return false;
	}

	if (this->m_exchange == NULL)
	{
		ErrorReturn = "Exchange Is NULL";
		return false;//交换机未绑定下面无法继续,这里返回false表示未连接（这里后面得找个好方法判断一下才行,因为有可能已经连接但是没有声明交换机的情况呀）
	}

	amqp_bytes_t _exchange = amqp_cstring_bytes(this->m_exchange->m_name.c_str());
	amqp_bytes_t _type = amqp_cstring_bytes(this->m_exchange->m_type.c_str());
	int32_t  _passive=this->m_exchange->m_passive;    
	int32_t  _durable=this->m_exchange->m_durable;      //交换机是否持久化
	amqp_exchange_declare(m_conn,m_channel,_exchange,_type,_passive,_durable, 0, 0, amqp_empty_table);
	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"exchange_declare",ErrorReturn))
	{
		return false;
	}

	return true;
}

int32_t CRabbitMQ_Adapter::queue_delete(const string queuename,int32_t if_unused,string &ErrorReturn)
{
	if(NULL ==m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
	amqp_channel_open(m_conn, m_channel);
	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"open channel",ErrorReturn) )
	{
		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_queue_delete(m_conn,m_channel,amqp_cstring_bytes(queuename.c_str()),if_unused,0);
	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"delete queue",ErrorReturn) )
	{
		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
	return 0;
}
//返回0是成功 否则全是失败
int32_t CRabbitMQ_Adapter::consumer(CQueue &queue,vector<CMessage> &message, uint32_t GetNum,timeval *timeout,string &ErrorReturn)
{
	if(NULL ==m_conn)
	{
		ErrorReturn = "还未创建连接";
		return -1;
	}
// 	amqp_channel_open(m_conn, m_channel);
// 	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"open channel",ErrorReturn) )
// 	{
// 		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
// 		return -1;
// 	}
 	amqp_bytes_t queuename= amqp_cstring_bytes(queue.m_name.c_str());
// 	amqp_queue_declare(m_conn,m_channel,queuename,0,queue.m_durable,0,0,amqp_empty_table);

	amqp_basic_qos(m_conn, m_channel,0,GetNum,0);
	int ack = 1; // no_ack    是否需要确认消息后再从队列中删除消息
	amqp_basic_consume(m_conn,m_channel,queuename,amqp_empty_bytes,0,ack,0,amqp_empty_table);

	if(1!=AssertError(amqp_get_rpc_reply(m_conn),"Consuming",ErrorReturn) )
	{
		amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
		return -1;
	}
	
	CMessage tmp("tmp");
	amqp_rpc_reply_t res;
	amqp_envelope_t envelope;
	int hasget = 0;
	while(GetNum>0)
	{
		amqp_maybe_release_buffers(m_conn);
		res = amqp_consume_message(m_conn, &envelope,timeout, 0);
		if (AMQP_RESPONSE_NORMAL != res.reply_type)
		{
			//amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
			ErrorReturn = "无法取得消息\n";
			if(0==hasget)
				return -res.reply_type;
			else
				return hasget;
		}

		string str((char *)envelope.message.body.bytes,(char *)envelope.message.body.bytes+envelope.message.body.len);
		tmp.m_data = str;
		tmp.m_data = tmp.m_data.substr(0,(int)envelope.message.body.len);
		tmp.m_routekey = (char *) envelope.routing_key.bytes;
		tmp.m_routekey = tmp.m_routekey.substr(0,(int)envelope.routing_key.len);
		message.push_back(tmp);
		//delete p;
		amqp_destroy_envelope(&envelope);
// 		int rtn = amqp_basic_ack(m_conn,m_channel,envelope.delivery_tag,1);
// 		if(rtn!=0)
// 		{
// 			amqp_channel_close(m_conn,m_channel, AMQP_REPLY_SUCCESS);
// 			return -1;
// 		}
		GetNum--;
		hasget++;
		// __sleep(1);
	}

	return hasget;
}

void CRabbitMQ_Adapter::__sleep(uint32_t millsecond)
{
	
	#if defined (__linux)
		usleep(millsecond);
	#elif defined (WIN32)
	  Sleep(millsecond);
	#endif
}

void CRabbitMQ_Adapter::setUser(const string UserName)
{
	this->m_user = UserName;
}

string CRabbitMQ_Adapter::getUser() const
{
	return m_user;
}

void CRabbitMQ_Adapter::setPassword(const string password)
{
	this->m_psw = password;
}
string CRabbitMQ_Adapter::getPassword() const
{
	return m_psw;
}


void CRabbitMQ_Adapter::setChannel(const uint32_t channel)
{
	this->m_channel = channel;
}
uint32_t CRabbitMQ_Adapter::getChannel()const
{
	return m_channel;
}

//返回1代表正常 其他都是错
int32_t CRabbitMQ_Adapter::AssertError(amqp_rpc_reply_t x, string context,string &ErrorReturn)
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

int32_t CRabbitMQ_Adapter::read(const string QueueName,vector<string> &message, uint32_t GetNum,struct timeval *timeout,string &ErrorReturn)
{
	return 0;
}


/*
AMQP_PUBLIC_FUNCTION amqp_channel_open_ok_t * AMQP_CALL amqp_channel_open(amqp_connection_state_t state, amqp_channel_t channel);

AMQP_PUBLIC_FUNCTION amqp_exchange_declare_ok_t * AMQP_CALL amqp_exchange_declare(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t exchange, amqp_bytes_t type, amqp_boolean_t passive, amqp_boolean_t durable, amqp_table_t arguments);
AMQP_PUBLIC_FUNCTION amqp_exchange_delete_ok_t * AMQP_CALL amqp_exchange_delete(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t exchange, amqp_boolean_t if_unused);

AMQP_PUBLIC_FUNCTION amqp_exchange_bind_ok_t * AMQP_CALL amqp_exchange_bind(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t destination, amqp_bytes_t source, amqp_bytes_t routing_key, amqp_table_t arguments);
AMQP_PUBLIC_FUNCTION amqp_exchange_unbind_ok_t * AMQP_CALL amqp_exchange_unbind(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t destination, amqp_bytes_t source, amqp_bytes_t routing_key, amqp_table_t arguments);

AMQP_PUBLIC_FUNCTION amqp_queue_declare_ok_t * AMQP_CALL amqp_queue_declare(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue, amqp_boolean_t passive, amqp_boolean_t durable, amqp_boolean_t exclusive, amqp_boolean_t auto_delete, amqp_table_t arguments);
AMQP_PUBLIC_FUNCTION amqp_queue_bind_ok_t * AMQP_CALL amqp_queue_bind(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue, amqp_bytes_t exchange, amqp_bytes_t routing_key, amqp_table_t arguments);

AMQP_PUBLIC_FUNCTION amqp_queue_purge_ok_t * AMQP_CALL amqp_queue_purge(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue);
AMQP_PUBLIC_FUNCTION amqp_queue_delete_ok_t * AMQP_CALL amqp_queue_delete(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue, amqp_boolean_t if_unused, amqp_boolean_t if_empty);

AMQP_PUBLIC_FUNCTION amqp_queue_unbind_ok_t * AMQP_CALL amqp_queue_unbind(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue, amqp_bytes_t exchange, amqp_bytes_t routing_key, amqp_table_t arguments);

AMQP_PUBLIC_FUNCTION amqp_basic_consume_ok_t * AMQP_CALL amqp_basic_consume(amqp_connection_state_t state, amqp_channel_t channel, amqp_bytes_t queue, amqp_bytes_t consumer_tag, amqp_boolean_t no_local, amqp_boolean_t no_ack, amqp_boolean_t exclusive, amqp_table_t arguments);
AMQP_PUBLIC_FUNCTION amqp_basic_qos_ok_t * AMQP_CALL amqp_basic_qos(amqp_connection_state_t state, amqp_channel_t channel, uint32_t prefetch_size, uint16_t prefetch_count, amqp_boolean_t global);

*/

