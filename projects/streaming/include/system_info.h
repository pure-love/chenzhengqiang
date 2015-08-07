/*
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2015/4/24
 * @modified-date:
 * @version:1.0
 * @desc:providing interface to calculate the cpu's occupy,memmory's occupy
 */


#ifndef _SYS_INFO_H_
#define _SYS_INFO_H_
#include<cstddef>
#include<string>
#include<ctime>
using std::string;

enum{HEADING_SIZE=20};
typedef struct _CPU_INFO
{
    char heading[HEADING_SIZE];
    double user_mod;
    double nice_mod;
    double system_mod;
    double idle_mod;
    double iowait_mod;
    double irq_mod;
    double softirq_mod;

}CPU_INFO;


typedef struct _MEM_INFO
{
    char heading_total[HEADING_SIZE];
    double mem_total;
    char heading_free[HEADING_SIZE];
    double mem_free;

}MEM_INFO;


typedef struct _NET_INFO
{
    unsigned long received_bytes;
    unsigned long received_packets;
    unsigned long transmited_bytes;
    unsigned long transmited_packets;

}NET_INFO;

//the base-class of system_info
class system
{
    public:
        virtual double get_cpu_occupy()=0;
        virtual double get_mem_occupy()=0;
        virtual bool get_net_occupy(NET_INFO &net_info)=0;

};

class system_info:public system
{
    private:
        double _cpu_occupy;
        double _mem_occupy;
        size_t _net_occupy;
        std::string _proc_cpu;
        std::string _proc_mem;
        std::string _proc_net;
        CPU_INFO  _initial_cpu_info;
        CPU_INFO  _current_cpu_info;
        MEM_INFO  _mem_info;
        NET_INFO _initial_net_info;
        NET_INFO  _current_net_info;
        bool _startup;
        clock_t _start;
        clock_t _finish;
    private:
        bool compute_cpu_occupy();
        bool compute_mem_occupy();
        bool compute_mem_occupy_through_proc();
        bool compute_mem_occupy_through_sysinfo();
        bool compute_net_occupy();

    public:
        system_info(const char *proc_cpu = "/proc/stat",const char *proc_mem="/proc/meminfo", const char *proc_net="/proc/net/dev");
        virtual double get_cpu_occupy();
        virtual double get_mem_occupy();
        virtual bool get_net_occupy(NET_INFO & net_info);
        virtual ~system_info(){}
};
#endif
