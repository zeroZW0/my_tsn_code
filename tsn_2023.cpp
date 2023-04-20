#include"common.h"

pc* m_pc = new pc[PC_NUMS+1];
int flowid = 1;
vector<link*> v_link(LINK_NUM);
vector<flow*> all_flow;
m_swtich* sw1 = new m_swtich();
vector<string> gcl{ "00000001","00000010","00000100","00001000","00010000","00100000","01000000","10000000" };


bool cmp_small_first(const flow* f1, const flow* f2) {
	if (f1->priority == f2->priority) return f1->playload < f2->playload;
	return f1->priority > f2->priority;
}

bool cmp_big_first(const flow* f1, const flow* f2) {
	if (f1->priority == f2->priority) return f1->playload > f2->playload;
	return f1->priority > f2->priority;
}
bool cmp_suiji(const flow *f1, const flow *f2) {
	
	return f1->priority > f2->priority;
}

bool cmp2(const pair<pair<double,double>,flow*> f1, const pair< pair<double, double>,flow*> f2) {
	return f1.first.first <  f2.first.first;
}

void init() {
	sw1->name = "switch1";
	for (int i =0; i < PC_NUMS; i++) {
		port* p = new port;			//为交换机初始化32端口
		for (int j = 0; j < QUEUE_NUMS; j++) {
			queue* q = new queue();	//为每个端口初始化8个队列
			p->que.push_back(q);
		}
		sw1->sw_port.push_back(p);
	}

	int m_period[3]{ 1000,2000,5000 };
	for (int i = 1; i <=PC_NUMS; i++) {
		m_pc[i].pc_name = "ES" + to_string(i ); //初始化pc名字
		m_pc[i].pc_flow.resize(FLOW_NUMS);
		for (int j = 0; j < FLOW_NUMS; j++) {
			m_pc[i].pc_flow[j] = new flow();
			m_pc[i].pc_flow[j]->l1 = new link();
			m_pc[i].pc_flow[j]->l2 = new link();
			m_pc[i].pc_flow[j]->flowid = flowid;
			m_pc[i].pc_flow[j]->source = m_pc[i].pc_name; //初始化源pc

			int r = rand() % PC_NUMS+1;
			while (r == i) { r = rand() % PC_NUMS +1 ; }
			m_pc[i].pc_flow[j]->des =r;		//目的地址1-32,与自身i不冲突

			m_pc[i].pc_flow[j]->playload = rand() % 1300 +100;
			m_pc[i].pc_flow[j]->period = m_period[rand() % 3];
			m_pc[i].pc_flow[j]->deadline = m_pc[i].pc_flow[j]->period;
			m_pc[i].pc_flow[j]->priority = rand() % 4 + 4;
			//m_pc[i].pc_flow[j]->priority = 7;
			m_pc[i].pc_flow[j]->offset = 0;		//偏移量的设置
			m_pc[i].pc_flow[j]->trans_time = (m_pc[i].pc_flow[j]->playload+58) * 8 / (double)1000;	//单位us
			m_pc[i].pc_flow[j]->des_port = flowid++;
			//根据目的地址的端口号，向pc机中添加对应的端口号
			m_pc[m_pc[i].pc_flow[j]->des].localport.push_back(m_pc[i].pc_flow[j]->des_port);
			//向sw1的出端口的对应队列添加消息
			sw1->sw_port[m_pc[i].pc_flow[j]->des-1]->que[m_pc[i].pc_flow[j]->priority ]->buff.push_back(m_pc[i].pc_flow[j]);
			//将流添加到对应pc的集合中
			m_pc[i].pc_flow.push_back(m_pc[i].pc_flow[j]);

			//将所有流都加入到一个集合中
			all_flow.push_back(m_pc[i].pc_flow[j]);
		}

	}
	//排序每个终端中的流，先安排优先级高，优先级相同选择载荷小的先转发
	//sort(all_flow.begin(), all_flow.end(), cmp_suiji);
	sort(all_flow.begin(), all_flow.end(), cmp_small_first);
	//sort(all_flow.begin(), all_flow.end(), cmp_big_first);
	//初始化link 
	for (int i = 1; i <= LINK_NUM / 2; i++) {
		link* l = new link();
		l->l_sour_PC.pc_name = "ES" + to_string(i );
		l->l_des_pc.pc_name = sw1->name;		//初始化每条从pc到交换机的链路
		for (auto& x : all_flow) {
			if (x->source == l->l_sour_PC.pc_name)
				l->link_flow.push_back(x);	//向链路中添加对应的流
		}
		l->last_time = 0;
		v_link[i-1] = l;
	}
	for (int i = LINK_NUM / 2+1; i <=LINK_NUM; i++) { //i从32-63
		link* l = new link();
		l->l_sour_PC.pc_name = sw1->name;		//初始化每个从交换机到pc的链路
		l->l_des_pc.pc_name = "ES" + to_string(i - PC_NUMS);  //1-32
		for (auto& x : all_flow) {
			if ("ES" + to_string(x->des) == l->l_des_pc.pc_name)
				l->link_flow.push_back(x);	//向链路中添加对应的流
		}
		l->last_time = 0;
		v_link[i-1] = l;;
	}

	//向流中添加路径
	for (auto& f : all_flow) {
		for (auto& l : v_link) {
			if (l->l_sour_PC.pc_name == f->source) {
				f->flow_link.push_back(l);
			}
			if (l->l_des_pc.pc_name == ("ES" + to_string(f->des))) {
				f->flow_link.push_back(l);
			}
		}
	}


	
}

void no_wait() {
	//依次规划每个流在链路上的传输时间  核心代码
	for (int i = 0; i < all_flow.size(); i++) {
		//两条链路上都有数据

		double link0_start = all_flow[i]->flow_link[0]->last_time;
		double	link1_start = all_flow[i]->flow_link[1]->last_time;
		//计算消息在link0真实的完成传输时间 是否大于 link1假设的开始时间
		if (link0_start + SW_PROCESS_TIME + LINK_TRANS_TIME + all_flow[i]->trans_time > link1_start) {
			//大于的话将link1的开始传输时间设置为  link0真实的完成传输时间
			link1_start = link0_start + SW_PROCESS_TIME + LINK_TRANS_TIME + all_flow[i]->trans_time;
		}
		else {
			//否则 将link0上的传输时间延后 
			link0_start = link1_start - (SW_PROCESS_TIME + LINK_TRANS_TIME + all_flow[i]->trans_time) ;
		}
		all_flow[i]->offset = link0_start;			//重置流的起始时间


		//总体的link上就会有这条流的起止时间,但是这种做法只能有一个周期的
		all_flow[i]->flow_link[0]->start_end.push_back({ link0_start ,link0_start + LINK_TRANS_TIME + all_flow[i]->trans_time });
		all_flow[i]->flow_link[1]->start_end.push_back({ link1_start ,link1_start + LINK_TRANS_TIME + all_flow[i]->trans_time });

		//更新last-time有用
		all_flow[i]->flow_link[0]->last_time = link0_start + LINK_TRANS_TIME + all_flow[i]->trans_time+IFG;
		all_flow[i]->flow_link[1]->last_time = link1_start + LINK_TRANS_TIME + all_flow[i]->trans_time + IFG;


		//这种做法只能有一个周期的
		for (int j = 0; j < 10000 / all_flow[i]->period; j++) {
			all_flow[i]->l1->start_end.push_back({ j * all_flow[i]->period + link0_start ,j * all_flow[i]->period + link0_start + LINK_TRANS_TIME + all_flow[i]->trans_time });
			all_flow[i]->l2->start_end.push_back({ j * all_flow[i]->period + link1_start ,j * all_flow[i]->period + link1_start + LINK_TRANS_TIME + all_flow[i]->trans_time });
		}

		for (auto& lk : v_link) {
			if (lk->l_des_pc.pc_name == "ES" + to_string(all_flow[i]->des)) {
				for (int j = 0; j < 10000 / all_flow[i]->period; j++) {

					lk->link_gcl.push_back({ { j * all_flow[i]->period + link1_start, j * all_flow[i]->period + link1_start + LINK_TRANS_TIME + all_flow[i]->trans_time },all_flow[i] });
				}
			}
		}

	}

}

void able_schedule_num() {
	int num = 0;
	long long sum_packet = 0;
	cout << "总共" << all_flow.size() << "待调度的包" << endl;
	for (auto f : all_flow) {
		if (f->offset + f->trans_time * 2 + 2 * LINK_TRANS_TIME + SW_PROCESS_TIME < f->deadline) {
			++num;
			sum_packet += f->playload;
		}
	}
	cout << "可以调度" << num << "个包；" << "不可调度" << all_flow.size() - num << endl;
	cout << "可以调度" << sum_packet << "字节" << endl;
	cout << "调度率为" << setprecision(2) << fixed<<((double)num * 100/ all_flow.size())  << "%" << endl;


	for (auto& x : v_link) {
		double res = 0;
		for (auto& f : x->link_flow) {
			res += f->trans_time / f->period * 1e6;
		}
		x->load = res;

	}
}

//生成流的描述文件
void get_flow_file(string filename) {  
	fstream outfile;
	string outt = "C:/Users/lzw_4/Desktop/tsn_code/tsn_2023/flow"+filename+".xml";
	outfile.open(outt, ios::app);
	outfile << "<schedule>" << endl;

	for (int i = 1; i <=PC_NUMS; i++) {
		for (int j = 0; j < FLOW_NUMS; j++) {
			outfile << "  <stream id=\"" << m_pc[i].pc_flow[j]->flowid << "\">" << endl;
			outfile << "<datagramSchedule baseTime=\""<<m_pc[i].pc_flow[j]->offset<<"us\" cycleTime=\"" << m_pc[i].pc_flow[j]->period << "us\">" << endl;
			outfile << " <event payloadSize=\"" << m_pc[i].pc_flow[j]->playload << "B\"" << " destAddress=\"" << "224.0.0." << m_pc[i].pc_flow[j]->des
				<< "\" destPort =\"" << m_pc[i].pc_flow[j]->des_port << "\" pcp = \""<< m_pc[i].pc_flow[j]->priority << "\" timeInterval=\"" << m_pc[i].pc_flow[j]->period << "us\"/>" << endl;
			outfile << " </datagramSchedule>" << "\n" << "</stream>" << endl;
		}
	}
	outfile << "</schedule>" << endl;
	outfile.close();
}

void  get_ini_file() {
	fstream outfile;
	string outt = "C:/Users/lzw_4/Desktop/tsn_code/tsn_2023/omnetpp.ini";
	outfile.open(outt, ios::app);
	outfile << "**.filteringDatabase.database=xmldoc(\"rout.xml\",\"/filteringDatabases/\")" << endl;
	
	int localport = 10000;
	//生成app对应流
	for (int i = 1; i <= PC_NUMS; i++) {
		int portNum = FLOW_NUMS + m_pc[i].localport.size();  //总app数目 发送+接收
		outfile << "**." << m_pc[i].pc_name << ".numApps=" << portNum+1 << endl;
		for (int j = 0; j < FLOW_NUMS; j++) {
			outfile << "**." << m_pc[i].pc_name << ".app[" << j << "].typename= \"UdpScheduledTrafficApp\""<< endl;
			outfile << "**." << m_pc[i].pc_name << ".app[" << j << "].scheduleManager.initialAdminSchedule=xmldoc(\"flow.xml\",\"/schedule/stream[@id=\'" << m_pc[i].pc_flow[j]->flowid << "\']/datagramSchedule\")" << endl;
			outfile << "**." << m_pc[i].pc_name << ".app[" << j << "].trafficGenerator.localPort=" << localport++ << endl;
		}
		for (int j = 0; j < m_pc[i].localport.size(); j++) {
			outfile << "**." << m_pc[i].pc_name << ".app[" << j+ FLOW_NUMS << "].typename= \"UdpSink\"" << endl;
			outfile << "**." << m_pc[i].pc_name << ".app[" << j + FLOW_NUMS  << "].localPort= " <<m_pc[i].localport[j]<< endl;
		}
		
		//接受BE流的
		outfile << "**." << m_pc[i].pc_name << ".app[" << portNum  << "].typename= \"UdpSink\"" << endl;
		outfile << "**." << m_pc[i].pc_name << ".app[" << portNum  << "].localPort= " << 5000+i << endl;

		outfile << "\n" << endl;
	}
	for (int i = PC_NUMS+1; i <= PC_NUMS + BE_PC_NUM; ++i) {

		outfile <<  "**.ES" << i << ".numApps=" << BE_PC_NUM << endl;
		for (int j = 0; j < 2*BE_PC_NUM; ++j) {
			outfile << "**.ES" << i << ".app[" << j << "].typename= \"AppVlanWrapper\"" << endl;
			outfile << "**.ES" << i << ".app[" << j << "].app.typename= \"UdpBasicApp\"" << endl;
			outfile << "**.ES" << i << ".app[" << j << "].vlanRequester.pcp=intuniform(0,2)" << endl;
			int r = rand() % PC_NUMS + 1;
			while (r == i) r = rand() % PC_NUMS + 1;
			outfile << "**.ES" << i << ".app[" << j << "].app.destPort= " << 5000 + r << endl;
			outfile << "**.ES" << i << ".app[" << j << "].app.destAddresses=\"224.0.0." << r << "\"" << endl;
			outfile << "**.ES" << i << ".app[" << j << "].app.sendInterval=exponential(" << 300 << "us)" << endl;
			outfile << "**.ES" << i << ".app[" << j << "].app.messageLength=intuniform(100B,1100B)" << endl;
			outfile << "**.ES" << i << ".app[" << j << "].app.packetName=\"UdpBestEffortTraffic\"" << endl << endl;

		}
		
	}

	//生成交换机对应的gcl
	for (int i = 0; i < PC_NUMS; i++) {
		outfile << "**.switch1.eth[" << i << "].queue.gateController.initialSchedule=xmldoc(\"schedules.xml\",\"/schedule/switch[@name='switch1']/port[@id=\'" << i << "\']/schedule\")" << endl;
	}
	outfile.close();
}

void get_rout() {
	fstream outfile;
	string outt = "C:/Users/lzw_4/Desktop/tsn_code/tsn_2023/rout.xml";
	outfile.open(outt, ios::app);
	outfile << "<filteringDatabases> \n<filteringDatabases id=\"switch1\">\n<static>\n<forward>" << endl;
	for (int i = 1; i <= PC_NUMS; i++) {
		if(i<=9)
		outfile << "<multicastAddress ports=\"" << i-1 << "\"  macAddress=\"01:00:5e:00:00:0"  << i << "\"/>"<<endl;
		else {
			
			char s[100];
			_itoa_s(i, s, 16);
			if(i<=16)
				outfile << "<multicastAddress ports=\"" << i - 1 << "\"  macAddress=\"01:00:5e:00:00:0" << s << "\"/>" << endl;
			else
			outfile << "<multicastAddress ports=\"" << i-1 << "\"  macAddress=\"01:00:5e:00:00:" << s << "\"/>" << endl;
		}
		
	}
	outfile << "</forward>\n</static>\n</filteringDatabases>\n</filteringDatabases>" << endl;
	outfile.close();
}

void get_schedules() {
	fstream outfile;
	string outt = "C:/Users/lzw_4/Desktop/tsn_code/tsn_2023/schedules.xml";
	outfile.open(outt, ios::app);
	outfile << "<schedule>" << endl;
	outfile <<"<switch name=\"switch1\">"<<endl;
	for (int i = 0; i < PC_NUMS; i++) {
		outfile << "\t<port id=\"" << i << "\">" << endl;
		outfile << "\t\t<schedule baseTime=\"" << 0 << "us\"" << "\tcycleTime=\"" << 10000 << "us\">" << endl;
		outfile << "\t\t<entry>\n\t\t\t<length>" << 10000 - (double)sw1->sw_port[i]->port_sum_transtime  << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
		for (int j = 4; j <= QUEUE_NUMS - 1; j++) {
			outfile << "\t\t<entry>\n\t\t\t<length>" <<(double) sw1->sw_port[i]->que[j]->sum_transtime  << "us</length>\n\t\t\t<bitvector>" << gcl[j] << "</bitvector>\n\t\t</entry>" << endl;
		}
		outfile << "\t\t</schedule>"<<endl;
		outfile << "\t\t</port>" << endl;
	}
	outfile << "</switch>" << endl;
	outfile << "</schedule>" << endl;
	outfile.close();
}

void get_schedules_jiange() {
	fstream outfile;
	string outt = "C:/Users/lzw_4/Desktop/tsn_code/tsn_2023/schedules_jiange.xml";
	outfile.open(outt, ios::app);
	outfile << "<schedule>" << endl;
	outfile << "<switch name=\"switch1\">" << endl;
	for (int i = 0; i < PC_NUMS; i++) {		//sw 端口号
		outfile << "\t<port id=\"" << i << "\">" << endl;
		outfile << "\t\t<schedule baseTime=\"" << 0 << "us\"" << "\tcycleTime=\"" << 10000 << "us\">" << endl;
		
		for (int j = 0; j < 10; j++) {	//j代表超周期中的每个周期，分为10个小周期
			if (j == 0) {
				outfile << "\t\t<entry>\n\t\t\t<length>" << 1000 - (double)sw1->sw_port[i]->port_transtime_oneperiod  << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
				for (int k = 4; k <= QUEUE_NUMS - 1; k++) {

					outfile << "\t\t<entry>\n\t\t\t<length>" << (double)sw1->sw_port[i]->que[k]->que_transtime_oneperiod  << "us</length>\n\t\t\t<bitvector>" << gcl[k] << "</bitvector>\n\t\t</entry>" << endl;
				}

			}
			else if (j % 2 == 0) {
				outfile << "\t\t<entry>\n\t\t\t<length>" << 1000 - (double)sw1->sw_port[i]->port_period2000  << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
				for (int k = 4; k <= QUEUE_NUMS - 1; k++) {

					outfile << "\t\t<entry>\n\t\t\t<length>" << (double)sw1->sw_port[i]->que[k]->period2000  << "us</length>\n\t\t\t<bitvector>" << gcl[k] << "</bitvector>\n\t\t</entry>" << endl;
				}
			}
			else if (j % 5 == 0) {
				outfile << "\t\t<entry>\n\t\t\t<length>" << 1000 - (double)sw1->sw_port[i]->port_period5000  << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
				for (int k = 4; k <= QUEUE_NUMS - 1; k++) {

					outfile << "\t\t<entry>\n\t\t\t<length>" << (double)sw1->sw_port[i]->que[k]->period5000 << "us</length>\n\t\t\t<bitvector>" << gcl[k] << "</bitvector>\n\t\t</entry>" << endl;
				}
			}
			else {
				outfile << "\t\t<entry>\n\t\t\t<length>" << 1000 - (double)sw1->sw_port[i]->port_period1000  << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
				for (int k = 4; k <= QUEUE_NUMS - 1; k++) {

					outfile << "\t\t<entry>\n\t\t\t<length>" << (double)sw1->sw_port[i]->que[k]->period1000 << "us</length>\n\t\t\t<bitvector>" << gcl[k] << "</bitvector>\n\t\t</entry>" << endl;
				}
			}

		}
		
		outfile << "\t\t</schedule>" << endl;
		outfile << "\t\t</port>" << endl;

	}
	outfile << "</switch>" << endl;
	outfile << "</schedule>" << endl;
	outfile.close();
}

int schedule_count = 0;
void get_schedules_no_wait() {
	fstream outfile;
	string outt = "C:/Users/lzw_4/Desktop/tsn_code/tsn_2023/schedules_no_wait.xml";
	outfile.open(outt, ios::app);
	outfile << "<schedule>" << endl;
	outfile << "<switch name=\"switch1\">" << endl;
	for (int i = 0; i < PC_NUMS; i++) {
		sort(v_link[i + PC_NUMS]->link_gcl.begin(), v_link[i + PC_NUMS]->link_gcl.end(), cmp2);
		outfile << "\t<port id=\"" << i << "\">" << endl;
		outfile << "\t\t<schedule baseTime=\"" << 0 << "us\"" << "\tcycleTime=\"" << 10000 << "us\">" << endl;
		//对于每个出口链路上的流集，流都是排序的 所以流集中的流也是排序好的
			/*for (int j = 0; j < v_link[i+ PC_NUMS]->link_flow.size();j++) {
				if (j == 0) {
					outfile << "\t\t<entry>\n\t\t\t<length>" <<  v_link[i + PC_NUMS]->link_flow[j]->l2->start_end[0].first << "us</length>\n\t\t\t<bitvector>" << gcl[v_link[i + PC_NUMS]->link_flow[j]->priority] << "</bitvector>\n\t\t</entry>" << endl;
					outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_flow[j]->l2->start_end[0].second - v_link[i + PC_NUMS]->link_flow[j]->l2->start_end[0].first << "us</length>\n\t\t\t<bitvector>" << gcl[v_link[i + PC_NUMS]->link_flow[j]->priority] << "</bitvector>\n\t\t</entry>" << endl;
				}
				else {
					if(v_link[i + PC_NUMS]->link_flow[j]->l2->start_end[0].first - v_link[i + PC_NUMS]->link_flow[j - 1]->l2->start_end[0].second)
					outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_flow[j]->l2->start_end[0].first - v_link[i + PC_NUMS]->link_flow[j - 1]->l2->start_end[0].second << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
					if(v_link[i + PC_NUMS]->link_flow[j]->l2->start_end[0].second - v_link[i + PC_NUMS]->link_flow[i]->l2->start_end[0].first)
					outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_flow[j]->l2->start_end[0].second - v_link[i + PC_NUMS]->link_flow[j]->l2->start_end[0].first << "us</length>\n\t\t\t<bitvector>" << gcl[v_link[i + PC_NUMS]->link_flow[j]->priority] << "</bitvector>\n\t\t</entry>" << endl;
				}
			}
		*/

		//在超周期中的后面的周期如何体现
		
		for (int j = 0; j < v_link[i + PC_NUMS]->link_gcl.size(); ++j) {
			if (j == 0) {
				outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_gcl[j].first.first << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
				outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_gcl[j].first.second - v_link[i + PC_NUMS]->link_gcl[j].first.first << "us</length>\n\t\t\t<bitvector>" << gcl[v_link[i + PC_NUMS]->link_gcl[j].second->priority] << "</bitvector>\n\t\t</entry>" << endl;
				++schedule_count;
			}
			else {
			
				//合并表项
				//如果表项间隔大于 设定时间，直接输出表项，
				if (v_link[i + PC_NUMS]->link_gcl[j].first.first - v_link[i + PC_NUMS]->link_gcl[j - 1].first.second > MIN_TRANS_TIME)
					outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_gcl[j].first.first - v_link[i + PC_NUMS]->link_gcl[j - 1].first.second << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
				//找到相连表项的最后一个 的end，减去第一个的start.
				if (j+1< v_link[i + PC_NUMS]->link_gcl.size()&&v_link[i + PC_NUMS]->link_gcl[j + 1].first.first - v_link[i + PC_NUMS]->link_gcl[j].first.second<v_link[i + PC_NUMS]->link_gcl[j + 1].first.first< MIN_TRANS_TIME && v_link[i + PC_NUMS]->link_gcl[j + 1].second->priority== v_link[i + PC_NUMS]->link_gcl[j].second->priority) {
					int temp = j;
					while (j + 1 < v_link[i + PC_NUMS]->link_gcl.size() && v_link[i + PC_NUMS]->link_gcl[j + 1].first.first- v_link[i + PC_NUMS]->link_gcl[j].first.second < MIN_TRANS_TIME && v_link[i + PC_NUMS]->link_gcl[j + 1].second->priority == v_link[i + PC_NUMS]->link_gcl[j].second->priority) {
						j++;
					}
					outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_gcl[j].first.second - v_link[i + PC_NUMS]->link_gcl[temp].first.first << "us</length>\n\t\t\t<bitvector>" << gcl[v_link[i + PC_NUMS]->link_gcl[j].second->priority] << "</bitvector>\n\t\t</entry>" << endl;
				}
				else {
					outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_gcl[j].first.second - v_link[i + PC_NUMS]->link_gcl[j].first.first << "us</length>\n\t\t\t<bitvector>" << gcl[v_link[i + PC_NUMS]->link_gcl[j].second->priority] << "</bitvector>\n\t\t</entry>" << endl;
				}


				//不合并列表
				/*if (v_link[i + PC_NUMS]->link_gcl[j].first.first - v_link[i + PC_NUMS]->link_gcl[j - 1].first.second>0.01)
					outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_gcl[j].first.first - v_link[i + PC_NUMS]->link_gcl[j-1].first.second << "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
				if (v_link[i + PC_NUMS]->link_gcl[j].first.second - v_link[i + PC_NUMS]->link_gcl[j].first.first>0.01)
					outfile << "\t\t<entry>\n\t\t\t<length>" << v_link[i + PC_NUMS]->link_gcl[j].first.second - v_link[i + PC_NUMS]->link_gcl[j].first.first << "us</length>\n\t\t\t<bitvector>" << gcl[v_link[i + PC_NUMS]->link_gcl[j].second->priority] << "</bitvector>\n\t\t</entry>" << endl;
		*/	}
		}
		if(v_link[i + PC_NUMS]->link_gcl.size()>0)
		outfile << "\t\t<entry>\n\t\t\t<length>" <<10000- v_link[i + PC_NUMS]->link_gcl[v_link[i + PC_NUMS]->link_gcl.size()-1].first.second<< "us</length>\n\t\t\t<bitvector>" << "00001111" << "</bitvector>\n\t\t</entry>" << endl;
		outfile << "\t\t</schedule>" << endl;
		outfile << "\t\t</port>" << endl;
	}
	outfile << "</switch>" << endl;
	outfile << "</schedule>" << endl;
}



void calculate() {
	for (int i = 0; i < PC_NUMS; i++) {
		for (int j = 4; j < QUEUE_NUMS; j++) {
			for (auto x : sw1->sw_port[i]->que[j]->buff) {
				sw1->sw_port[i]->que[j]->sum_transtime += ((x->trans_time)*(double)(10000/x->period ));//10000us内发送的总的数据量
				sw1->sw_port[i]->que[j]->que_transtime_oneperiod += (x->trans_time) ;//1000us内发送的总的数据量
				if (x->period == 1000) {
					sw1->sw_port[i]->que[j]->period1000 +=( x->trans_time);
					sw1->sw_port[i]->que[j]->period2000 += (x->trans_time);
					sw1->sw_port[i]->que[j]->period5000 += (x->trans_time);
				}
				if (x->period == 2000) {
					sw1->sw_port[i]->que[j]->period2000 += (x->trans_time);
				}
				if (x->period == 5000) {
					sw1->sw_port[i]->que[j]->period5000 += (x->trans_time);
				}
			}
			sw1->sw_port[i]->port_sum_transtime += sw1->sw_port[i]->que[j]->sum_transtime;
			sw1->sw_port[i]->port_transtime_oneperiod += sw1->sw_port[i]->que[j]->que_transtime_oneperiod;
			sw1->sw_port[i]->port_period1000 += sw1->sw_port[i]->que[j]->period1000;
			sw1->sw_port[i]->port_period2000 += sw1->sw_port[i]->que[j]->period2000;
			sw1->sw_port[i]->port_period5000 += sw1->sw_port[i]->que[j]->period5000;

		
		}
	}
}

void get_config() {
	fstream outfile;
	string outt = "C:/Users/lzw_4/Desktop/tsn_code/tsn_2023/config.xml";
	outfile.open(outt, ios::app);
	outfile << "<config>" << endl;
	outfile << "<interface host=\"**\" address=\"10.x.x.x\" netmask=\"255.x.x.x\"/>"<<endl;
	for (int i = 1; i <= PC_NUMS; i++) {
		outfile << "<multicast-group host =\"ES" << i << "\" address=\"224.0.0." << i << "\"/>\n";
	}
	outfile << "</config>" << endl;
	outfile.close();
}
int main() {
	init();
	//get_ini_file();  //生成ini文件
	//get_flow_file(""); //生成流文件
	//get_config();
	//get_rout();      //生成路由
	
	
	calculate();  //计算每个pc的每个端口的载荷分布
	//get_schedules(); //生成调度

	no_wait();
	get_flow_file("");
	//get_schedules_jiange();
	get_schedules_no_wait();

	able_schedule_num();
	return -1;
}