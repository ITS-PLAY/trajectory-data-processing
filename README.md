# trajectory-data-processing
Electronic police data from  structurized video picture



按时间顺序（升序），根据路网关联关系确定过车数据中相邻轨迹点之间是否存在连接关系；如果不存在说明轨迹数据存在缺失情况。进而对过车数据进行补全操作，步骤如下：
Step1：上下游相同交叉口集合的筛选。
      从起点和终点出发，相向进行搜索：
     在起点处，搜索所有的下游交叉口集合；在终点处，搜索所有的上游交叉口集合；比较与集合是否存在相同的交叉口，存在则记录交叉口编号。如果不存在，则递归调 用交叉口搜索函数，直到找到共同集合为止。
    当终点的上游交叉口为路网边缘交叉口时，停止调用跳出循环。
Step2：路径序列的逆向查询和重复路径的删除。
     在确定共同的交叉口集合后，根据上下游交叉口的集合，逆向重复步骤1）的过程，查找一条或多条可行的路径集合。
     得到的路径集合可能存在重复的路径，通过比较路径包含的交叉口个数以及每个交叉口的编号删除多余的路径。
Step3：基于距离的最短路径选择。
     根据路径中相邻的交叉口编号，查询TB_ROAD表获取路段的长度，累加求和计算路径的长度。对比路径集合中最短的路径，作为缺失轨迹的可行路径。
Step4：基于距离的过车时间的计算。
     根据与轨迹点的时间与，基于路段的长度分配的时间长度，并最终将时间差值叠加到上，作为每个补齐轨迹点的过车时间JGSK_FORMATE。
Step5：将补齐的过车数据写入数据库。
    根据路径集合中的交叉口编号，将车牌号HPHM，交叉口KKBH或CROSS_CODE，过车时间JGSK_FORMATE和车道编号CDBH等信息写入数据库中。
    
