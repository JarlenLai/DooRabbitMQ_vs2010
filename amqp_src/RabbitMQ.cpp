
#include "MessageQueue/RabbitMQ.h"
#include "amqp_tcp_socket.h"
#include <string>
#include <vector>
#include <iostream>
#include <vector>

using namespace std;

#include "RabbitMQ_Adapter.h"
 
CRabbitMQ::CRabbitMQ(string HostName, uint32_t port,string usr,string psw, bool useConChan)
{
	this->adapter = new CRabbitMQ_Adapter(HostName,port,usr,psw,useConChan);
}

CRabbitMQ::~CRabbitMQ()
{
	delete this->adapter;
}

int32_t CRabbitMQ::Disconnect(string &ErrorReturn)
{
	
	return this->adapter->Disconnect(ErrorReturn);
}

int32_t CRabbitMQ::Connect(string &ErrorReturn)
{
	return this->adapter->Connect(ErrorReturn);
};

bool CRabbitMQ::IsConnect(string &ErrorReturn)
{
	return this->adapter->IsConnect(ErrorReturn);
}

//step1 declare an exchange
 int32_t CRabbitMQ::exchange_declare(CExchange &exchange,string &ErrorReturn)
{
	return this->adapter->exchange_declare(exchange,ErrorReturn);
}

 //step2 declare a queue
 int32_t CRabbitMQ::queue_declare(CQueue &queue,string &ErrorReturn)
{
	return this->adapter->queue_declare(queue,ErrorReturn);
 }

 //step3 bind
 int32_t CRabbitMQ::queue_bind(CQueue &queue,CExchange &exchange,const string bind_key,string &ErrorReturn)
 {
	return this->adapter->queue_bind(queue,exchange,bind_key,ErrorReturn);
 }

int32_t  CRabbitMQ::queue_unbind(CQueue &queue,CExchange &exchange,const string bind_key,string &ErrorReturn)
 {
	 return this->adapter->queue_unbind(queue,exchange,bind_key,ErrorReturn);
 }

//step 4 publish message 
int32_t CRabbitMQ::publish(vector<CMessage> &message,string routekey,string &ErrorReturn)
{
	return this->adapter->publish(message,routekey,ErrorReturn);
}

int32_t CRabbitMQ::publish(CMessage &message,string routkey,string &ErrorReturn)
{
	return this->adapter->publish(message,routkey,ErrorReturn);
}

int32_t CRabbitMQ::publish(const string &message,string routekey,string &ErrorReturn)
{
	return this->adapter->publish(message,routekey,ErrorReturn);
}

int32_t CRabbitMQ::publish_ack_wait(string &ErrorReturn, string &FailMessage)
{
	return this->adapter->publish_ack_wait(ErrorReturn, FailMessage);
}

int32_t CRabbitMQ::getMessageCount(const CQueue &queue,string &ErrorReturn)
{
	return this->adapter->getMessageCount(queue,ErrorReturn);	
}


int32_t CRabbitMQ::getMessageCount(const string &queuename,string &ErrorReturn)
{
	return this->adapter->getMessageCount(queuename,ErrorReturn);
}

int32_t CRabbitMQ::queue_delete(const string queuename,int32_t if_unused,string &ErrorReturn)
{
	return this->adapter->queue_delete(queuename,if_unused,ErrorReturn);
}
//返回0是成功 否则全是失败
int32_t CRabbitMQ::consumer(CQueue &queue,vector<CMessage> &message, uint32_t GetNum,timeval *timeout,string &ErrorReturn)
{
	return this->adapter->consumer(queue,message,GetNum,timeout,ErrorReturn);
}

void CRabbitMQ::setUser(const string UserName)
{
	this->adapter->setUser(UserName);
}

string CRabbitMQ::getUser() const
{
	return this->adapter->getUser();
}

void CRabbitMQ::setPassword(const string password)
{
	this->adapter->setPassword(password);
}
string CRabbitMQ::getPassword() const
{
	return  this->adapter->getPassword();
}

void CRabbitMQ::__sleep(uint32_t millsecond)
{
	this->adapter->__sleep(millsecond);
}
