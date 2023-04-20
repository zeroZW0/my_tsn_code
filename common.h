#pragma once
#include<fstream>
#include<string>
#include<iostream>
#include<sstream>
#include<vector>
#include<iomanip>
#include<iostream>
#include<unordered_set>
#include<set>
#include<vector>
#include<algorithm>
#include<cstdio> 
#include<cstdlib>
#include<map>
#include <iomanip> 

#define PC_NUMS 32

#define LINK_NUM 64
#define BE_PC_NUM 4
#define LINK_SPEED 1e9
#define	QUEUE_NUMS 8
#define SW_PROCESS_TIME 5	//�����������ӳ�   us
#define LINK_TRANS_TIME 0.05	//��·����ʱ���ӳ�+ifg
#define IFG 0.046
#define MIN_TRANS_TIME 1

int FLOW_NUMS = 50;


using namespace std;
class link;
class flow {
public:
	flow() {}
public:
	int  flowid;
	string source;
	int  des;
	int playload;
	int period;				//��������
	int deadline;			//�������̵����˵����ӳ�
	int priority;
	double offset;
	int des_port;
	int jetter;				//����ӳٶ���
	vector<link*> flow_link;//flow_link[0]�ǵ�һ��link //flow_link[1]�ǵڶ���link
	link* l1;
	link* l2;
	int alloc_port;			//flowi��l_sour_PC��������>l_des_pc�ϵĶ˿ڷ���
	double trans_time;		//����·�ϵĴ���ʱ��
};

class pc {
public:
	pc() {}
public:
	vector<flow*> pc_flow;	//�Ӵ�pc���͵���
	string pc_name;
	vector<int> localport;	//����app�Ķ˿ں�
};

class link {
public:
	pc l_sour_PC;			//Դpc
	pc l_des_pc;			//Ŀ��pc
	vector<flow*> link_flow;//��· l_sour_PC��������>l_des_pc�ϵ�������
	int h_lcm;				//��· l_sour_PC��������>l_des_pc�ϵ����ĳ�����
	double last_time;		//��·������û�б�ռ�õ�ʱ��
	double load;			//��·����
	vector<pair<double, double>>start_end;	//flowi ��ָ����·�ϵ���ֹʱ��
	vector<pair<pair<double, double>, flow*>>  link_gcl;
};

class queue {
public:
	vector<flow*>buff;		//������flow�Ļ���
	double sum_transtime =0;		//һ�������ڶ������ܵ��غ�
	double que_transtime_oneperiod;   //һ��С���ڶ��еķ�����
	double period2000;			//һ��С���ڶ����е�����Ϊ2000+1000�ķ�����
	double period5000;			//һ��С���ڶ����е�����Ϊ5000+1000�ķ�����
	double period1000;			//һ��С���ڶ����е�����Ϊ1000�ķ�����
};

class port {
public:
	vector<queue*> que;		//�˿��ϵĶ���
	double port_sum_transtime;	//һ�������ڶ˿ڷ��͵��ܵ�������
	double port_transtime_oneperiod;   //һ��С���ڶ˿ڵķ�����
	double port_period2000;			//һ��С���ڶ����е�����Ϊ2000+1000�ķ�����
	double port_period5000;			//һ��С���ڶ����е�����Ϊ5000+1000�ķ�����
	double port_period1000;			//һ��С���ڶ����е�����Ϊ1000�ķ�����
};

class m_swtich {
public:
	m_swtich(){}
	string name;
	vector<port*> sw_port;	//�������Ķ˿�
};

struct BE {
	int pcp;
	int desaddress;
	int destPort;
	int sendInterval;
	int messageLenth;
};

