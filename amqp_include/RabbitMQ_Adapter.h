#ifndef RABBITMQ_ADAPTER_H
#define RABBITMQ_ADAPTER_H

#include <string>
#include <vector>
#include <cstdio>

using namespace std;
#include "amqp.h"
#include "MessageQueue/SysDefine.h"
#include "MessageQueue/MessageBody.h"

//����ȫ�ֱ���
extern Publish string err;
extern Publish string& GetErr();

/** 
*   @brief ��Ϣ���й�����
* 
*   class CRabbitMQ in "RabbitMQ.h"
**/
class CRabbitMQ_Adapter
{
private:
	string                  m_hostName;    //��Ϣ��������
	uint32_t                m_port;        //��Ϣ���ж˿�
	amqp_socket_t           *m_sock;
    amqp_connection_state_t m_conn;
	string					m_user;
	string					m_psw;
	uint32_t				m_channel; 

	string m_routkey;
	CExchange *m_exchange;
	CQueue    *m_queue;

public:

	/**
	* @brief CRabbitMQ ���캯��
	* @param [int] HostName   ��Ϣ��������
	* @param [int] port       ��Ϣ���ж˿ں�
	* @return ��
	* @par ʾ��:
	*  @code
	*  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
	CRabbitMQ_Adapter(string HostName="localhost",uint32_t port=5672,string usr="guest",string psw="guest");
   //��������
	~CRabbitMQ_Adapter();
   
	/**
	* @brief connect  ������Ϣ���з�����
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ����ӣ�С��0������󣬴�����Ϣ��ErrorReturn����
    *  @par ʾ��:
	*  @code
	*  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
	int32_t Connect(string &ErrorReturn);
    
   /**
	* @brief Disconnect  ����Ϣ���з������Ͽ�����
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ��Ͽ����ӣ�С��0������󣬴�����Ϣ��ErrorReturn����
	*
	*  @par ʾ��:
	*  @code
	*  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
   int32_t Disconnect(string &ErrorReturn);

       /**
	*   @brief       exchange_declare   ����exchange
	*	@param       [in]               exchange       ������ʵ��
	*   @param        [out] ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ�����exchange��С��0������󣬴�����Ϣ��ErrorReturn����
	*   @par ʾ��:
	*   @code
    *   @endcode
    *   @see      
	*   @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
   int32_t exchange_declare(CExchange &exchange,string &ErrorReturn=GetErr());

    /**
	*   @brief       queue_declare                    ������Ϣ����
	*	@param       [in]               queue         ��Ϣ����ʵ��
	*   @param       [out]              ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ�����queue��С��0������󣬴�����Ϣ��ErrorReturn����
	*   @par ʾ��:
	*   @code
    *   @endcode
    *   @see      
	*   @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
   int32_t queue_declare(CQueue &queue,string &ErrorReturn);

    /**
	*   @brief       queue_bind                       �����У��������Ͱ󶨹���������γ�һ��·�ɱ�
	*	@param       [in]               queue         ��Ϣ����
	*	@param       [in]               exchange      ����������
	*	@param       [in]               bind_key      ·������  ��msg.#����msg.weather.**��
	*   @param       [out]              ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ��󶨣�С��0������󣬴�����Ϣ��ErrorReturn����
	*   @par ʾ��:
	*   @code
    *   @endcode
    *   @see      
	*   @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
    int32_t queue_bind(CQueue &queue,CExchange &exchange,const string bind_key,string &ErrorReturn);

  /**
	*   @brief       queue_bind                       �����У��������Ͱ󶨹���󶨽��
	*	@param       [in]               queue         ��Ϣ����
	*	@param       [in]               exchange      ����������
	*	@param       [in]               bind_key      ·������  ��msg.#����msg.weather.**��
	*   @param       [out]              ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ��󶨣�С��0������󣬴�����Ϣ��ErrorReturn����
	*   @par ʾ��:
	*   @code
    *   @endcode
    *   @see      
	*   @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
    int32_t queue_unbind(CQueue &queue,CExchange &exchange,const string bind_key,string &ErrorReturn);

   /**
	* @brief publish  ������Ϣ
	* @param [in] messag         ��Ϣʵ��
	* @param [in] rout_key       ·�ɹ��� 
    *   1.Direct Exchange �C ����·�ɼ�����Ҫ��һ�����а󶨵��������ϣ�Ҫ�����Ϣ��һ���ض���·�ɼ���ȫƥ�䡣
    *   2.Fanout Exchange �C ������·�ɼ��������а󶨵��������ϡ�һ�����͵�����������Ϣ���ᱻת������ý������󶨵����ж����ϡ�
	*   3.Topic Exchange �C ��·�ɼ���ĳģʽ����ƥ�䡣��ʱ������Ҫ��Ҫһ��ģʽ�ϡ����š�#��ƥ��һ�������ʣ����š�*��ƥ�䲻�಻��һ���ʡ�
    *      ��ˡ�audit.#���ܹ�ƥ�䵽��audit.irs.corporate�������ǡ�audit.*�� ֻ��ƥ�䵽��audit.irs��
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ�������Ϣʵ�������С��0�����ʹ��󣬴�����Ϣ��ErrorReturn����
	*
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
   int32_t publish(vector<CMessage> &message,string routkey,string &ErrorReturn);

   int32_t publish(CMessage &message,string routkey,string &ErrorReturn);

   int32_t publish(const string &message,string routkey,string &ErrorReturn);

  /** 
	* @brief consumer  ������Ϣ
	* @param [in]  queue        ����
	* @param [out] message      ��Ϣʵ��
    * @param [int] GetNum       ��Ҫȡ�õ���Ϣ����
	* @param [int] timeout      ȡ�õ���Ϣ���ӳ٣���ΪNULL����ʾ����ȡ�����ӳ٣�����״̬
    * @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ���С��0������󣬴�����Ϣ��ErrorReturn����
	*
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
   int32_t consumer(CQueue &queue,vector<CMessage> &message, uint32_t GetNum=1,struct timeval *timeout=NULL,string &ErrorReturn=err);
  

    /**
	*   @brief       queue_delete                     ɾ����Ϣ���С�
	*	@param       [in]               queuename     ��Ϣ��������
	*	@param       [in]               if_unused     ��Ϣ�����Ƿ����ã�1 �����Ƿ����ö�ɾ��
	*   @param       [out]              ErrorReturn   ������Ϣ
	*   @return ����0ֵ����ɹ�ɾ��queue��С��0������󣬴�����Ϣ��ErrorReturn����
	*   @par ʾ��:
	*   @code
    *   @endcode
    *   @see      
	*   @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/
    int32_t queue_delete(const string queuename,int32_t if_unused=0,string &ErrorReturn=GetErr());
	
  
	/**
	* @brief getMessageCount      ��ö�����Ϣ���� 
	* @param [in]  Queue          Ҫ��ȡ��Ϣ��������Ϣ����
    * @param [out] ErrorReturn    ������Ϣ
	* @return ����-1�������ȡ��Ϣ����ʧ�ܣ����ش��ڵ��ڵ���0ֵ������Ϣ������������Ϣ��ErrorReturn����
	*
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/ 
	int32_t getMessageCount(const CQueue &queue,string &ErrorReturn=GetErr());
	int32_t getMessageCount(const string &queuename,string &ErrorReturn=GetErr());

	/**
	* @brief IsConnect  �ж��Ƿ��Ѿ�����MQ
	* @param [out] ErrorReturn   ������Ϣ
	* @return ����trueֵ����ɹ����ӣ�false���������Ѿ��Ͽ���������Ϣ��ErrorReturn����
    *  @par ʾ��:
	*  @code
	*  @endcode
    *  @see
	*/
	bool IsConnect(string &ErrorReturn);
   
	/**
	* @brief setUser            ���õ�¼�û����� 
	* @param [in]  UserName     ��¼�û�����
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/ 
    void setUser(const string UserName);

    /**
	* @brief getUser       ��õ�¼�û����� 
	* @return              ���ص�ǰ��¼�û���
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/ 
    string getUser() const;

   	/**
	* @brief setPassword        ���õ�¼�û����� 
	* @param [in]  password     ��¼�û�����
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/ 
   void setPassword(const string password);

    /**
	* @brief getPassword   ��õ�¼�û����� 
	* @return              ���ص�ǰ��¼�û�������
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/ 
   string getPassword() const;
	
   void __sleep(uint32_t millsecond);

private:
	/**
	* @brief read  ȡ����Ϣ ȡ�ú�ɾ����Ϣʵ��
	* @param [in]  QueueName ��������
	* @param [out] message      ��Ϣʵ��
    * @param [int] GetNum       ��Ҫȡ�õ���Ϣ����
	* @param [int] timeout      ȡ�õ���Ϣ���ӳ٣���ΪNULL����ʾ����ȡ�����ӳ٣�����״̬
    * @param [out] ErrorReturn   ������Ϣ
	* @return ����0ֵ����ɹ���С��0������󣬴�����Ϣ��ErrorReturn����
	*
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ���������ȡ����
	*/ 
    int32_t read(const string QueueName,vector<string> &message, uint32_t GetNum=1,struct timeval *timeout=NULL,string &ErrorReturn=GetErr());


   /**
	* @brief setChannel         ����ͨ���� 
	* @param [in]  channel      ���õ�ͨ����
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/ 
	void setChannel(const uint32_t channel);

    /**
	* @brief getChannel    ��õ�ǰͨ���� 
	* @return              ���ص�ǰͨ����
	*  @par ʾ��:
	*  @code
    *  @endcode
    *  @see      
	*  @deprecated ���������ԭ������������ܻ��ڽ����İ汾��ȡ����
	*/ 
   uint32_t getChannel()const;
private:
  //����1�ɹ��������Ǵ���
  int32_t AssertError(amqp_rpc_reply_t x, string context,string &ErrorReturn);
   CRabbitMQ_Adapter(const CRabbitMQ_Adapter &other) //�������캯��
   {
   }
   CRabbitMQ_Adapter &operator=(const CRabbitMQ_Adapter &oter ) //��ֵ����
   {
	   return *this;
   }
	
};

#endif

