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
#define SW_PROCESS_TIME 5	//交换机处理延迟   us
#define LINK_TRANS_TIME 0.05	//链路传播时间延迟+ifg
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
	int period;				//发送周期
	int deadline;			//所能容忍的最大端到端延迟
	int priority;
	double offset;
	int des_port;
	int jetter;				//最大延迟抖动
	vector<link*> flow_link;//flow_link[0]是第一条link //flow_link[1]是第二条link
	link* l1;
	link* l2;
	int alloc_port;			//flowi在l_sour_PC————>l_des_pc上的端口分配
	double trans_time;		//在链路上的传输时间
};

class pc {
public:
	pc() {}
public:
	vector<flow*> pc_flow;	//从此pc发送的流
	string pc_name;
	vector<int> localport;	//本地app的端口号
};

class link {
public:
	pc l_sour_PC;			//源pc
	pc l_des_pc;			//目的pc
	vector<flow*> link_flow;//链路 l_sour_PC————>l_des_pc上的流集合
	int h_lcm;				//链路 l_sour_PC————>l_des_pc上的流的超周期
	double last_time;		//链路上最新没有被占用的时间
	double load;			//链路负载
	vector<pair<double, double>>start_end;	//flowi 在指定链路上的起止时间
	vector<pair<pair<double, double>, flow*>>  link_gcl;
};

class queue {
public:
	vector<flow*>buff;		//队列上flow的缓存
	double sum_transtime =0;		//一个超周期队列上总的载荷
	double que_transtime_oneperiod;   //一个小周期队列的发送量
	double period2000;			//一个小周期队列中的周期为2000+1000的发送量
	double period5000;			//一个小周期队列中的周期为5000+1000的发送量
	double period1000;			//一个小周期队列中的周期为1000的发送量
};

class port {
public:
	vector<queue*> que;		//端口上的队列
	double port_sum_transtime;	//一个超周期端口发送的总的数据量
	double port_transtime_oneperiod;   //一个小周期端口的发送量
	double port_period2000;			//一个小周期队列中的周期为2000+1000的发送量
	double port_period5000;			//一个小周期队列中的周期为5000+1000的发送量
	double port_period1000;			//一个小周期队列中的周期为1000的发送量
};

class m_swtich {
public:
	m_swtich(){}
	string name;
	vector<port*> sw_port;	//交换机的端口
};

struct BE {
	int pcp;
	int desaddress;
	int destPort;
	int sendInterval;
	int messageLenth;
};

