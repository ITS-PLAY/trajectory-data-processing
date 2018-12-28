#include "VspdCTOMySQL.h"
#include "trajectory_analysis.h"
#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <sstream>
#include <comutil.h>

using namespace std;

int main()
{

	char* host="localhost"; 

	char* user="root"; 

	char* port ="3306";

	char* passwd="admin123"; 

	char* dbname="tongxiang_test"; 

	char* charset = "utf-8";//支持中文

	char* Msg = "";//消息变量

	char SQL[1024];

	VspdCToMySQL *vspdctomysql = new VspdCToMySQL;

	if(vspdctomysql->ConnMySQL(host,port,dbname,user,passwd,charset,Msg) == 0)
		printf(Msg);

	char *split1="?@";
	char *split2="?";
	char *split3="@";

	//轨迹序列分段：时间分段、车辆

	//sprintf(SQL,"select NOW()");
	//string SP0 = vspdctomysql->SelectData(SQL,1,Msg);
	//string date_start = RETURN_NOWTIME(SP0,split1);          //必须的输入参数

	string date_end = "2017-11-29 12:20:00";                 //测试日期
	string date_start = "";

	int HOUR = 0, MINU = 0;
	HOUR = -TIME_INTERVAL / 60;
	MINU = -TIME_INTERVAL % 60;
	sprintf(SQL, "select DATE_ADD('%s',INTERVAL %d HOUR)", date_end.c_str(), HOUR);
	string SP0 = vspdctomysql->SelectData(SQL, 1, Msg);

	char *str = (char *)SP0.c_str();
	date_start = strtok(str, split1);

	sprintf(SQL, "select DATE_ADD('%s', INTERVAL %d MINUTE)", date_start.c_str(), MINU);
	SP0 = vspdctomysql->SelectData(SQL, 1, Msg);

	str = (char *)SP0.c_str();
	date_start = strtok(str, split1);


	sprintf(SQL, "select distinct HPHM from after_vehicle_total where GWSJ between '%s' and '%s' and HPHM <> '-' and KKID in (select KKID from tb_kk_sc where BOTTLENECK_AREA=1)",date_start.c_str(),date_end.c_str());     //查询after_vehicle_total表获取过车数据

	string SP = vspdctomysql->SelectData(SQL,1,Msg);

	vector<string> HPHM;
	if (SP.length() > 0){
		char *str = (char *)SP.c_str();
		str = strtok(str,split1);

		while(str != NULL){
			HPHM.push_back(str);

			str = strtok(NULL, split1);
		}
	}

	//轨迹缺失的判断
	vector<string> veh_info,GWSJ,HPHM_veh,SC_NO,FXBH;     //车辆信息的预处理

	vector<string> SC_INTERVAL;        //缺失的轨迹点对（包含起点和终点的交叉口编号）
	vector<string> GWSJ_INTERVAL;     //缺失的轨迹点对（包含起点和终点的过车时间）
	vector<string> FXBH_INTERVAL;    //缺失的轨迹点对（包含起点和终点的方向）

	vector<string>::iterator it_HPHM;
	for (it_HPHM=HPHM.begin(); it_HPHM != HPHM.end(); it_HPHM++){
		sprintf(SQL, "select a.GWSJ,b.CROSS_CODE,a.FXBH from"
			" (select GWSJ,KKID,FXBH from after_vehicle_total where GWSJ between '%s' and '%s' and HPHM = '%s') a"
			" LEFT JOIN tb_kk_sc b"
			" on a.KKID = b.KKID where b.SC_NO is not null  order by a.GWSJ",date_start.c_str(),date_end.c_str(),(*it_HPHM).c_str());

		SP = vspdctomysql->SelectData(SQL,3,Msg);

		int num = it_HPHM-HPHM.begin();

		cout<<num<<endl;

		if (SP.length() >0 ){

			char *str = (char *)SP.c_str();
			str = strtok(str,split3);

			while(str != NULL){

				veh_info.push_back(str);
				str = strtok(NULL,split3); 

			}

			for (int veh_num=0; veh_num <veh_info.size(); veh_num++){         //过车时间、车辆号牌和方向编号
				str = (char *)veh_info[veh_num].c_str();

				str = strtok(str,split2);
				GWSJ.push_back(str);

				str = strtok(NULL,split2);
				SC_NO.push_back(str);

				str = strtok(NULL,split2);
				FXBH.push_back(str);               

				vector<string> END_CROSS;         //轨迹点的下游交叉口集合

				if (veh_num > 0){
					sprintf(SQL,"select END_CROSS from tb_road where START_CROSS='%s'",SC_NO[veh_num-1].c_str());      //设置方向限制：FXBH
					SP = vspdctomysql->SelectData(SQL,1,Msg);

					str = (char *)SP.c_str();
					str = strtok(str, split1);
					while(str != NULL){
						END_CROSS.push_back(str);
						str = strtok(NULL, split1);
					}

					END_CROSS.push_back(SC_NO[veh_num-1]);         //下游交叉口和自身

					//int SC = atoi(SC_NO[veh_num].c_str());
					//char SC_CODE[25];
					//_itoa(SC, SC_CODE, 10);

					int cross_num = 0;
					for (cross_num=0; cross_num<END_CROSS.size(); cross_num++){   					//比较

						if (SC_NO[veh_num] == END_CROSS[cross_num])
							break;

					}

					if (cross_num==END_CROSS.size()){
						sprintf(SQL, "update after_vehicle_total set TZTP=1 where GWSJ in ('%s','%s') and HPHM='%s'",GWSJ[veh_num-1].c_str(),GWSJ[veh_num].c_str(),(*it_HPHM).c_str());
						vspdctomysql->UpdateData(SQL,Msg);

						if (timediff(GWSJ[veh_num],GWSJ[veh_num-1],vspdctomysql, split1)>3600){
							sprintf(SQL, "update after_vehicle_total set TZTP=2 where GWSJ='%s' and HPHM='%s'",GWSJ[veh_num].c_str(),(*it_HPHM).c_str());
							vspdctomysql->UpdateData(SQL,Msg);

						}else{
							SC_INTERVAL.push_back(SC_NO[veh_num-1]);
							SC_INTERVAL.push_back(SC_NO[veh_num]);

							GWSJ_INTERVAL.push_back(GWSJ[veh_num-1]);
							GWSJ_INTERVAL.push_back(GWSJ[veh_num]);

							FXBH_INTERVAL.push_back(FXBH[veh_num-1]);
							FXBH_INTERVAL.push_back(FXBH[veh_num]);
						}
					}
				}

				END_CROSS.swap(vector<string>());

			}          

			//轨迹数据的补齐
			for (int SC_num = 0; SC_num < SC_INTERVAL.size()/2; SC_num++){   

				//判断缺失交叉口的区间

				string DIRECTION_end=FXBHtoDIR(FXBH_INTERVAL[2*SC_num+1]);
				string SC_end_up = "",KKID = "";
				int length_up=0, length_down=0;

				sprintf(SQL, "select START_CROSS from tb_road where END_CROSS='%s' and END_CROSS_DIRECTION='%s'",SC_INTERVAL[2*SC_num+1].c_str(),DIRECTION_end.c_str());
				string SP = vspdctomysql->SelectData(SQL,1,Msg);

				if (SP.length()>0){
					char *str = (char *)SP.c_str();
					SC_end_up = strtok(str,split1);
				}else{
					break;        //区域外围路段
				}

				vector<string> END_CROSS,PRE_END_CROSS;         //轨迹点的下游交叉口集合 以及 上一个交叉口编号
				sprintf(SQL, "select END_CROSS from tb_road where START_CROSS='%s'",SC_INTERVAL[2*SC_num].c_str());

				SP = vspdctomysql->SelectData(SQL,1,Msg);

				if (SP.length() >0 ){
					char *str = (char *)SP.c_str();

					str = strtok(str, split1);
					while(str != NULL){
						END_CROSS.push_back(str);
						PRE_END_CROSS.push_back(SC_INTERVAL[2*SC_num]);
						str = strtok(NULL, split1);
					}

					int cross_num=0;
					for (cross_num=0; cross_num<END_CROSS.size(); cross_num++){   					

						if (SC_end_up == END_CROSS[cross_num]){

							//缺失一个点的补齐函数
                            missing_data_single((*it_HPHM),SC_INTERVAL[2*SC_num], SC_INTERVAL[2*SC_num+1], END_CROSS[cross_num],GWSJ_INTERVAL[2*SC_num],GWSJ_INTERVAL[2*SC_num+1],FXBH_INTERVAL[2*SC_num],FXBH_INTERVAL[2*SC_num+1], vspdctomysql,split1,split2);

							break;

						}else {

							sprintf(SQL, "select ROAD_CODE from TB_ROAD where START_CROSS='%s' and END_CROSS = '%s'",END_CROSS[cross_num].c_str(),SC_end_up.c_str());
							SP = vspdctomysql->SelectData(SQL,1,Msg);

							if (SP.length()>0){

								//缺失两个点的补齐
                               missing_data_double((*it_HPHM),SC_INTERVAL[2*SC_num], SC_INTERVAL[2*SC_num+1], END_CROSS[cross_num], SC_end_up, GWSJ_INTERVAL[2*SC_num],GWSJ_INTERVAL[2*SC_num+1],FXBH_INTERVAL[2*SC_num],FXBH_INTERVAL[2*SC_num+1], vspdctomysql,split1,split2);
							   break;
							}

						}  //else语句 

					}   //for (int cross_num=0; cross_num<END_CROSS.size(); cross_num++)

					if (cross_num == END_CROSS.size()){

						if (strcmp(SC_end_up.c_str(),"024")>0 ){        //路网搜索范围的限定

							cout <<"多异常点的轨迹补齐" <<endl;

							vector<string> START_CROSS,PRE_START_CROSS;
							START_CROSS.push_back(SC_end_up);
							PRE_START_CROSS.push_back(SC_INTERVAL[2*SC_num+1]);
							int PRE_END_NUM=0,PRE_START_NUM=0;

							//缺失多个点的补齐
							missing_data_more((*it_HPHM),SC_INTERVAL[2*SC_num], SC_INTERVAL[2*SC_num+1],END_CROSS,PRE_END_CROSS,PRE_END_NUM, START_CROSS,PRE_START_CROSS,PRE_START_NUM, GWSJ_INTERVAL[2*SC_num],GWSJ_INTERVAL[2*SC_num+1],vspdctomysql,split1,split2); 

						}else{
							cout << "车辆位于路网外围"<<endl;
						}

					}

				}      //if (SP.length() >0 )

				END_CROSS.swap(vector<string>());
				PRE_END_CROSS.swap(vector<string>());

			}			//for (int SC_num = 0; SC_num < SC_INTERVAL.size()/2; SC_num++){   

			GWSJ.swap(vector<string>());
			SC_NO.swap(vector<string>());
			FXBH.swap(vector<string>());  


		}    //if (SP.length() >0 )

		veh_info.swap(vector<string>());
		GWSJ_INTERVAL.swap(vector<string>());
		SC_INTERVAL.swap(vector<string>());
		FXBH_INTERVAL.swap(vector<string>());

	}         //for (it_HPHM=HPHM.begin(); it_HPHM != HPHM.end(); it_HPHM++){


	vspdctomysql->CloseMySQLConn();
	delete vspdctomysql;

	return 0;
}



bool missing_data_single(string HPHM,string SC_start, string SC_end, string END_CROSS,string date_start,string date_end,string FXBH_start,string FXBH_end, VspdCToMySQL *vspdctomysql,char *split1, char *split2)
{
	char SQL[1024];
	char *Msg = "";
	char *split3="@";

	float length_up=0,length_down=0;
	string FXBH="XX",KKID="0",SC_end_up="0";


	sprintf(SQL, "select KKID from tb_kk_sc where CROSS_CODE= '%s'",END_CROSS.c_str());
	string SP = vspdctomysql->SelectData(SQL,1,Msg);
	if (SP.length()>0){
		char *str = (char *)SP.c_str();
		KKID = strtok(str, split1);
	}else{
		KKID = END_CROSS;
	}


	int time_diff = timediff(date_start, date_end, vspdctomysql, split1);

	sprintf(SQL, "select LENGTH,END_CROSS_DIRECTION from tb_road where START_CROSS = '%s' and END_CROSS= '%s'",SC_start.c_str(),END_CROSS.c_str());
	SP = vspdctomysql->SelectData(SQL,2,Msg);

	char *str = (char *)SP.c_str();
	str = strtok(str, split2);
	length_up = atof(str);

	string DIR_end = strtok(NULL, split2);
	FXBH = DIRtoFXBH(DIR_end);

	sprintf(SQL, "select LENGTH from tb_road where START_CROSS= '%s' and END_CROSS= '%s'",END_CROSS.c_str(),SC_end.c_str());
	SP = vspdctomysql->SelectData(SQL,1,Msg);

	str = (char *)SP.c_str();
	str = strtok(str, split1);

	//sscanf(str, "%lf", &length_down);
	length_down = atof(str);

	
	string CDBH = RETURN_CDBH(END_CROSS,SC_end,DIR_end,vspdctomysql, split2, split3);
	int timeadd = (int) time_diff*length_up/(length_up + length_down);
	string GWSJ = time_add(date_start,timeadd, vspdctomysql, split1);

	sprintf(SQL, "insert into after_vehicle_total(ID,KKID,HPHM,GWSJ,FXBH,CDBH,TZTP) values(999,'%s','%s','%s','%s','%s',0)",KKID.c_str(),HPHM.c_str(),GWSJ.c_str(),FXBH.c_str(),CDBH.c_str());
	vspdctomysql->InsertData(SQL,Msg);

	return true;

}

bool missing_data_double(string HPHM, string SC_start, string SC_end,string END_CROSS, string SC_end_up, string date_start,string date_end,string FXBH_start,string FXBH_end, VspdCToMySQL *vspdctomysql,char *split1,char *split2)
{
	char SQL[1024];
	char *Msg="";
	char *split3 = "@";

	string KKID = "";
	string DIRECTION_end=FXBHtoDIR(FXBH_end);


		sprintf(SQL, "select KKID from tb_kk_sc where CROSS_CODE= '%s'",END_CROSS.c_str());
		string SP = vspdctomysql->SelectData(SQL,1,Msg);
		if (SP.length()>0){
			char *str = (char *)SP.c_str();
			KKID = strtok(str, split1);
		}else {
			KKID = END_CROSS;
		}

		int time_diff = timediff(date_start, date_end, vspdctomysql, split1);

		sprintf(SQL, "select LENGTH,END_CROSS_DIRECTION from tb_road where START_CROSS = '%s' and END_CROSS= '%s'",SC_start.c_str(),END_CROSS.c_str());
		SP = vspdctomysql->SelectData(SQL,2,Msg);

		char *str = (char *)SP.c_str();
		str = strtok(str, split2);
		float length_up1 = atof(str);	

		string DIR_end = strtok(NULL, split2);
		string FXBH_up1 = DIRtoFXBH(DIR_end);

		sprintf(SQL, "select LENGTH,END_CROSS_DIRECTION from tb_road where START_CROSS= '%s' and END_CROSS= '%s'",END_CROSS.c_str(),SC_end_up.c_str());
		SP = vspdctomysql->SelectData(SQL,2,Msg);

		str = (char *)SP.c_str();
		str = strtok(str, split2);
		float length_up2 = atof(str);

		//sscanf(str, "%lf", &length_up2);
		DIR_end = strtok(NULL, split2);
		string FXBH_up2 = DIRtoFXBH(DIR_end);


		sprintf(SQL, "select LENGTH from tb_road where START_CROSS= '%s' and END_CROSS= '%s'",SC_end_up.c_str(),SC_end.c_str());
		SP = vspdctomysql->SelectData(SQL,1,Msg);

		str = (char *)SP.c_str();
		str = strtok(str, split2);
		float length_up3 = atof(str);

		//sscanf(str, "%lf", &length_up3);

		int timeadd = (int) time_diff*length_up1/(length_up1 + length_up2 + length_up3);
		string GWSJ = time_add(date_start,timeadd, vspdctomysql, split1);

		string DIR = FXBHtoDIR(FXBH_up1);
		string CDBH = RETURN_CDBH(END_CROSS,SC_end_up,DIR,vspdctomysql, split2, split3);
		sprintf(SQL, "insert into after_vehicle_total(ID,KKID,HPHM,GWSJ,FXBH,CDBH,TZTP) values(999,'%s','%s','%s','%s','%s',0)",KKID.c_str(),HPHM.c_str(),GWSJ.c_str(),FXBH_up1.c_str(),CDBH.c_str());
		vspdctomysql->InsertData(SQL,Msg);

		sprintf(SQL, "select KKID from tb_kk_sc where CROSS_CODE= '%s'",SC_end_up.c_str());
		SP = vspdctomysql->SelectData(SQL,1,Msg);
		if (SP.length()>0){
			str = (char *)SP.c_str();
			KKID = strtok(str, split1);
		}else {
			KKID = SC_end_up;
		}

		DIR = FXBHtoDIR(FXBH_up2);
		CDBH = RETURN_CDBH(SC_end_up,SC_end,DIR,vspdctomysql, split2, split3);
		timeadd = time_diff*(length_up1+length_up2)/(length_up1 + length_up2 + length_up3);
		GWSJ = time_add(date_start,timeadd, vspdctomysql, split1);

		sprintf(SQL, "insert into after_vehicle_total(ID,KKID,HPHM,GWSJ,FXBH,CDBH,TZTP) values(999,'%s','%s','%s','%s','%s',0)",KKID.c_str(),HPHM.c_str(),GWSJ.c_str(),FXBH_up2.c_str(),CDBH.c_str());
		vspdctomysql->InsertData(SQL,Msg);


	return true;

}


bool missing_data_more(string HPHM, string SC_start, string SC_end,vector<string> &END_CROSS,vector<string> &PRE_END_CROSS,int PRE_END_NUM, vector<string> &START_CROSS,vector<string> &PRE_START_CROSS,int PRE_START_NUM, string date_start,string date_end,VspdCToMySQL *vspdctomysql,char *split1,char *split2)
{

    char SQL[1024];
	char *Msg="";
	string SP = "";
	char *split3="@";

	int END_NUM_temp=END_CROSS.size();
	int START_NUM_temp = START_CROSS.size();

	string str_end="0";
	for (int i=0;i < END_CROSS.size(); i++){
		str_end ="'"+ END_CROSS[i] +"'" + "," + str_end; 
	}
	str_end = str_end + "," + "'"+ SC_start + "'";

	string str_start="0";
	for (int j=0;j < START_CROSS.size(); j++){
		str_start = "'"+ START_CROSS[j] + "'" + "," + str_start; 
	}
	str_start = str_start + "," + "'"+ SC_end + "'";


	for (int i_end=PRE_END_NUM; i_end < END_NUM_temp; i_end++){

		sprintf(SQL, "select END_CROSS from TB_ROAD where START_CROSS = '%s' and END_CROSS not in (%s)",END_CROSS[i_end].c_str(),str_end.c_str());
		SP= vspdctomysql->SelectData(SQL,1,Msg);

		char *str = (char *)SP.c_str();
		str = strtok(str,split1);
		while (str != NULL){
			END_CROSS.push_back(str);
            PRE_END_CROSS.push_back(END_CROSS[i_end]);

			str = strtok(NULL, split1);
		}
	}


	for (int j_start=PRE_START_NUM; j_start < START_NUM_temp; j_start++){

		sprintf(SQL, "select START_CROSS from TB_ROAD where END_CROSS = '%s' and START_CROSS not in(%s)",START_CROSS[j_start].c_str(),str_start.c_str());
		SP= vspdctomysql->SelectData(SQL,1,Msg);

		char *str = (char *)SP.c_str();
		str = strtok(str,split1);
		while (str != NULL){
			START_CROSS.push_back(str);
			PRE_START_CROSS.push_back(START_CROSS[j_start]);

			str = strtok(NULL, split1);
		}

	}

	vector<string> UNION_POINT;
	vector<string> ROUTE_END_SERIES,ROUTE_START_SERIES;          //存储路径的次序
	vector<string> ROUTE;
	int route_num=0;
    
	for (int i_end=PRE_END_NUM; i_end < END_CROSS.size(); i_end++){
		for (int j_start= PRE_START_NUM; j_start < START_CROSS.size(); j_start++){

			if (END_CROSS[i_end] == START_CROSS[j_start]){
				UNION_POINT.push_back(END_CROSS[i_end]);
				route_num++;
			}
            
			if (route_num >6)      //可行路径存在的个数限制
				break;

		}
	}

	if (UNION_POINT.size() == 0){
        PRE_END_NUM = END_NUM_temp;
		PRE_START_NUM = START_NUM_temp;

        missing_data_more(HPHM, SC_start, SC_end,END_CROSS,PRE_END_CROSS,PRE_END_NUM, START_CROSS,PRE_START_CROSS,PRE_START_NUM, date_start,date_end,vspdctomysql,split1,split2);

	}else {

		vector<string>::iterator it_end;
		int it_pre=0;

		vector<string>::iterator it_start;	

		vector<vector<string>> ROUTE_ALL,ROUTE_VALID;
		//ROUTE_ALL.resize(UNION_POINT.size());

		for (int num=0; num < UNION_POINT.size(); num ++){

			ROUTE_END_SERIES.push_back(UNION_POINT[num]);
            ROUTE_START_SERIES.push_back(UNION_POINT[num]);

            it_end = find(END_CROSS.begin(),END_CROSS.end(),UNION_POINT[num]);
            it_pre = it_end - END_CROSS.begin();

			while(it_end != END_CROSS.end()){

				it_end = find(END_CROSS.begin(),END_CROSS.end(),PRE_END_CROSS[it_pre]);
                ROUTE_END_SERIES.insert(ROUTE_END_SERIES.begin(),PRE_END_CROSS[it_pre]);                          //在ROUTE_END_SERIES容器中，向前插入上游交叉口编号
				it_pre = it_end - END_CROSS.begin();
				
			}

			it_start = find(START_CROSS.begin(),START_CROSS.end(),UNION_POINT[num]);
			it_pre = it_start - START_CROSS.begin();

			while(it_start != START_CROSS.end()){

				it_start = find(START_CROSS.begin(),START_CROSS.end(),PRE_START_CROSS[it_pre]);
				ROUTE_START_SERIES.push_back(PRE_START_CROSS[it_pre]);                          //在ROUTE_START_SERIES容器中，向后插入下游交叉口编号
				it_pre = it_start - START_CROSS.begin();

			}
            
			//存储有效路径
			ROUTE.insert(ROUTE.end(),ROUTE_END_SERIES.begin(),ROUTE_END_SERIES.end()-1);
            ROUTE.insert(ROUTE.end(),ROUTE_START_SERIES.begin(),ROUTE_START_SERIES.end());
            
			ROUTE_ALL.push_back(ROUTE);

			ROUTE.swap(vector<string>());
			ROUTE_END_SERIES.swap(vector<string>());
			ROUTE_START_SERIES.swap(vector<string>());

		}


		vector<float> DISTANCE;
		vector<vector<float>> DISTANCE_ROUTE;
		vector<string> FXBH;
		vector<vector<string>> FXBH_ROUTE;

		float MIN_DIS=100000.0;
		int MIN_INDEX=-1;


		vector<string>::iterator it;
		vector<vector<string>>::iterator iter1;
		vector<string> vec_tmp1,vec_tmp2;


		   //删除重复路径
        ROUTE_VALID.push_back(ROUTE_ALL[0]);
		int loop1=0,loop_temp=0;

		while( loop1<ROUTE_ALL.size()-1 ){

			vec_tmp1 = ROUTE_ALL[loop1];
			for (int loop2 = loop1+1; loop2 < ROUTE_ALL.size(); loop2 ++){
				vec_tmp2 = ROUTE_ALL[loop2];

				if (vec_tmp1.size() != vec_tmp2.size()){
					ROUTE_VALID.push_back(vec_tmp2);
				} else{
					for (int vec_num=0; vec_num < vec_tmp1.size(); vec_num++){
						if (vec_tmp1[vec_num] != vec_tmp2[vec_num]){
							ROUTE_VALID.push_back(vec_tmp2);
							loop1 = loop2-1;
							break;
						}

					}
				}

			} 

			loop1++;
		}


		//计算最短路
		for (iter1 = ROUTE_VALID.begin(); iter1 != ROUTE_VALID.end(); iter1 ++){
			 vec_tmp1 = *iter1;

			for (it =vec_tmp1.begin(); it != vec_tmp1.end()-1; it ++){
                 sprintf(SQL, "select LENGTH,END_CROSS_DIRECTION from tb_road where START_CROSS= '%s' and END_CROSS= '%s'",(*it).c_str(),(*(it+1)).c_str());
                 SP = vspdctomysql->SelectData(SQL,2,Msg);

				 if (SP.length()>0){
					 char *str = (char *)SP.c_str();
					 str = strtok(str, split2);
					 float length = atof(str);

					 DISTANCE.push_back(length);

					 string DIR_end = strtok(NULL, split2);
					 string FXBH_temp = DIRtoFXBH(DIR_end);
					 FXBH.push_back(FXBH_temp);
				 }

			}

			float DIS = accumulate(DISTANCE.begin(),DISTANCE.end(),0.0);
			if (DIS < MIN_DIS){
				MIN_DIS = DIS;
				MIN_INDEX = iter1-ROUTE_VALID.begin();
			}

			DISTANCE_ROUTE.push_back(DISTANCE);
			FXBH_ROUTE.push_back(FXBH);

			DISTANCE.swap(vector<float>());
			FXBH.swap(vector<string>());

		}

		//计算过车时间
		int time_diff = timediff(date_start, date_end, vspdctomysql, split1);

		if (MIN_INDEX > -1){
                float sum_time =0.0;
			for (int point_num=0; point_num < DISTANCE_ROUTE[MIN_INDEX].size()-1; point_num ++){
				 sum_time = sum_time + DISTANCE_ROUTE[MIN_INDEX][point_num];

			     float timeadd = time_diff*sum_time/MIN_DIS;
			     string GWSJ = time_add(date_start,timeadd, vspdctomysql, split1);

				 sprintf(SQL, "select KKID from tb_kk_sc where CROSS_CODE= %s",ROUTE_VALID[MIN_INDEX][point_num+1].c_str());
				 SP = vspdctomysql->SelectData(SQL,1,Msg);
				 string KKID = "0";
				 if (SP.length()>0){
					 char *str = (char *)SP.c_str();
					 KKID = strtok(str, split1);
				 }else {
					 KKID = ROUTE_VALID[MIN_INDEX][point_num+1];
				 }
                  string DIR = FXBHtoDIR(FXBH_ROUTE[MIN_INDEX][point_num]);
				  string CDBH = RETURN_CDBH(ROUTE_VALID[MIN_INDEX][point_num+1],ROUTE_VALID[MIN_INDEX][point_num+2],DIR,vspdctomysql, split2, split3);

				 //写入数据库
				 sprintf(SQL, "insert into after_vehicle_total(ID,KKID,HPHM,GWSJ,FXBH,CDBH,TZTP) values(999,'%s','%s','%s','%s','%s',0)",KKID.c_str(),HPHM.c_str(),GWSJ.c_str(),FXBH_ROUTE[MIN_INDEX][point_num].c_str(),CDBH.c_str());
				 vspdctomysql->InsertData(SQL,Msg);
			}
		}

		ROUTE_ALL.swap(vector<vector<string>>());
		ROUTE_VALID.swap(vector<vector<string>>());

	}

	return true;
}


int timediff(string date_start, string date_end, VspdCToMySQL *vspdctomysql, char *split1)          //计算两个日期的时间差，返回秒数
{
	int second=0;
	char SQL[1024];
	char* Msg = "";

	sprintf(SQL, "select UNIX_TIMESTAMP('%s')-UNIX_TIMESTAMP('%s')",date_end.c_str(), date_start.c_str());
	string SP = vspdctomysql->SelectData(SQL,1,Msg);

	char *str = (char *)SP.c_str();
	str = strtok(str,split1);

	sscanf(str,"%d", &second);

	return second;

}

string time_add(string date_start,int time_diff, VspdCToMySQL *vspdctomysql, char *split1)          //计算一定时间间隔的日期值
{

	char SQL[1024];
	char* Msg = "";

	string date_end="";

	int MINU  = time_diff/60;
	int SECO = time_diff%60;
	sprintf(SQL, "select DATE_ADD('%s',INTERVAL %d MINUTE)", date_start.c_str(), MINU);
	string SP0 = vspdctomysql->SelectData(SQL, 1, Msg);

	char *str = (char *)SP0.c_str();
	date_end = strtok(str, split1);

	sprintf(SQL, "select DATE_ADD('%s', INTERVAL %d SECOND)", date_end.c_str(), SECO);
	SP0 = vspdctomysql->SelectData(SQL, 1, Msg);

	str = (char *)SP0.c_str();
	date_end = strtok(str, split1);

	return date_end;

}


string FXBHtoDIR(string FXBH_end)                         //将过车数据的进口道方向转化为VISSIM中的方向
{
	string DIRECTION_END="";

	int FXBH = atoi(FXBH_end.c_str());

	switch(FXBH) {
		case 1: DIRECTION_END="2";break;
		case 2: DIRECTION_END="6";break;
		case 3: DIRECTION_END="4";break;
		case 4: DIRECTION_END="0";break;
		default:
			DIRECTION_END="9";break;
	}

	return DIRECTION_END;

}

string DIRtoFXBH(string DIR_end)                        //将VISSIM中的方向转化为过车数据的进口道方向
{
	string FXBH_END="";

	int DIR = atoi(DIR_end.c_str());

	switch(DIR) {
		case 0: FXBH_END="04";break;
		case 2: FXBH_END="01";break;
		case 4: FXBH_END="03";break;
		case 6: FXBH_END="02";break;
		default:
			FXBH_END="XX";break;
	}

	return FXBH_END;
}

string RETURN_CDBH(string SC_start,string SC_end,string SC_start_dir,VspdCToMySQL *vspdctomysql, char *split2, char *split3)
{
	char SQL[1024] = {0};
	char *Msg="";
	int START_DIR=0,END_DIR=0;

    sprintf(SQL, "select START_CROSS_DIRECTION from TB_ROAD where START_CROSS='%s' and END_CROSS='%s'",SC_start.c_str(),SC_end.c_str());
	string SP = vspdctomysql->SelectData(SQL,1,Msg);

	if (SP.length()>0){
		char *str = (char *)SP.c_str();
		str = strtok(str,split2);


		sscanf(str,"%d",&END_DIR);

		sscanf(SC_start_dir.c_str(),"%d",&START_DIR);

		int LEFT_CODE = (START_DIR + 2)%8;
		int THROUGH_CODE = (START_DIR + 4)%8;
		int RIGHT_CODE = (START_DIR + 6)%8;

		sprintf(SQL,"select MOVEMENT,LANE_NO_REAL from tb_lane_tongxiang where OWNER_CROSS='%s' and DIRECTION = '%s'",SC_start.c_str(),SC_start_dir.c_str());
		SP = vspdctomysql->SelectData(SQL,2,Msg);

		vector<string> MOV_TEMP;

		str = (char *)SP.c_str();
		str = strtok(str,split3);
		while(str != NULL){
			MOV_TEMP.push_back(str);
			str = strtok(NULL,split3);

		}

		vector<string> MOVEMENT,CDBH;
		vector<string> LEFT_MOV,THROUGH_MOV,RIGHT_MOV;
		vector<string> LEFT_CDBH,THROUGH_CDBH,RIGHT_CDBH;

		vector<string>::iterator it;
		for(it=MOV_TEMP.begin(); it!=MOV_TEMP.end(); it++){
			str = (char *)(*it).c_str();
			str = strtok(str,split2);
			MOVEMENT.push_back(str);

			str = strtok(NULL,split2);
			CDBH.push_back(str);
		}

		for (int i=0; i<MOVEMENT.size(); i++){

			if (MOVEMENT[i]=="12" | MOVEMENT[i] == "21" | MOVEMENT[i] == "24" | MOVEMENT[i] == "23"){
				LEFT_MOV.push_back(MOVEMENT[i]);
				LEFT_CDBH.push_back(CDBH[i]);
			}
			if (MOVEMENT[i]=="11" | MOVEMENT[i] == "21" | MOVEMENT[i] == "22"| MOVEMENT[i] == "24"){
				THROUGH_MOV.push_back(MOVEMENT[i]);	
				THROUGH_CDBH.push_back(CDBH[i]);
			}
			if (MOVEMENT[i]=="13" | MOVEMENT[i] == "22" | MOVEMENT[i] == "23"| MOVEMENT[i] == "24"){
				RIGHT_MOV.push_back(MOVEMENT[i]);
				RIGHT_CDBH.push_back(CDBH[i]);
			}
		}

		if (END_DIR == LEFT_CODE){
			if (LEFT_MOV.size()==1){
				return LEFT_CDBH[0];
			}else if (LEFT_MOV.size()>1){
				int index = rand()%LEFT_MOV.size();
				return LEFT_CDBH[index];
			}
		}else if(END_DIR == THROUGH_CODE){
			if (THROUGH_MOV.size()==1){
				return THROUGH_CDBH[0];
			}else if (THROUGH_MOV.size()>1){
				int index = rand()%THROUGH_MOV.size();
				return THROUGH_CDBH[index];
			}
		}else if(END_DIR == RIGHT_CODE){
			if (RIGHT_MOV.size()==1){
				return RIGHT_CDBH[0];
			}else if (RIGHT_MOV.size()>1){
				int index = rand()%RIGHT_MOV.size();
				return RIGHT_CDBH[index];
			}

		}else
			return "XX";

	}else 
		return "XX";

}

