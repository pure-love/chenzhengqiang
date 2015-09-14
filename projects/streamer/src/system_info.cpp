/*
 * @file name:sys_info.cpp
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2015/4/24
 * @modified-date:
 * @version:1.0
 * @desc:implementation of class sys_info to calculate the cpu's occupy,memory's occupy
 */


#include "system_info.h"
#include<cstdio>
#include<cstring>
#include<stdexcept>
#include<sys/sysinfo.h>
using std::runtime_error;
system_info::system_info( const char * proc_cpu, const char * proc_mem, const char *proc_net ):
    _cpu_occupy(0),_mem_occupy(0),_net_occupy(0),_proc_cpu(proc_cpu),_proc_mem(proc_mem),
    _proc_net(proc_net),_startup(true),_start(0),_finish(0)

{
    compute_cpu_occupy();
    compute_net_occupy();
    _startup=false;
}



/*
 *@args:
 *@desc:compute the occupy of system's CPU
 * computational formula:
 CPU OCCUPY = 100 *(user + nice + system£©/(user+nice+system+idle)
 */
static const int LINE_SIZE=256;
bool system_info::compute_cpu_occupy()
{
    FILE *proc_cpu_handler;
    CPU_INFO cpu_info;
    char line[LINE_SIZE];

    proc_cpu_handler = fopen (_proc_cpu.c_str(), "r");
    if( proc_cpu_handler == NULL )
    {
        return false;
    }
    fgets (line, sizeof(line), proc_cpu_handler);
    sscanf (line, "%s %lf %lf %lf %lf %lf %lf %lf",
            cpu_info.heading, &cpu_info.user_mod, &cpu_info.nice_mod,&cpu_info.system_mod,
            &cpu_info.idle_mod,&cpu_info.iowait_mod,&cpu_info.irq_mod,&cpu_info.softirq_mod);
    if( _startup )
    {
        _initial_cpu_info.user_mod = cpu_info.user_mod;
        _initial_cpu_info.nice_mod = cpu_info.nice_mod;
        _initial_cpu_info.system_mod = cpu_info.system_mod;
        _initial_cpu_info.idle_mod = cpu_info.idle_mod;
        _initial_cpu_info.iowait_mod = cpu_info.iowait_mod;
        _initial_cpu_info.irq_mod = cpu_info.irq_mod;
        _initial_cpu_info.softirq_mod = cpu_info.softirq_mod;
    }
    else
    {
        _current_cpu_info.user_mod = cpu_info.user_mod;
        _current_cpu_info.nice_mod = cpu_info.nice_mod;
        _current_cpu_info.system_mod = cpu_info.system_mod;
        _current_cpu_info.idle_mod = cpu_info.idle_mod;
        _current_cpu_info.iowait_mod = cpu_info.iowait_mod;
        _current_cpu_info.irq_mod = cpu_info.irq_mod;
        _current_cpu_info.softirq_mod = cpu_info.softirq_mod;
    }
    fclose(proc_cpu_handler);
    return true;
}



/*
 *@args:
 *@desc:compute the occupy of system's memory
 * computational formula:
 MEMORY OCCUPY = 100 *(current_mem / total_mem)
 */
bool system_info::compute_mem_occupy_through_proc()
{
    FILE *proc_mem_handler;
    char line[LINE_SIZE];
    proc_mem_handler = fopen (_proc_mem.c_str(), "r");
    if( proc_mem_handler == NULL )
    {
        return false;
    }

    fgets (line, sizeof(line), proc_mem_handler);
    sscanf (line, "%s %lf", _mem_info.heading_total,&_mem_info.mem_total);
    fgets (line, sizeof(line), proc_mem_handler);
    sscanf (line, "%s %lf", _mem_info.heading_free,&_mem_info.mem_free);
    _mem_occupy = 100*((_mem_info.mem_total-_mem_info.mem_free)   / _mem_info.mem_total);
    fclose( proc_mem_handler);
    return true;
}



/*
 *@args:
 *@desc:compute the occupy of system's memory
 * by simply calling sysinfo
 */
bool system_info::compute_mem_occupy_through_sysinfo()
{
    struct sysinfo  sif;
    int ret = 0;
    ret = sysinfo(&sif);
    if ( ret == 0 )
    {
        _mem_occupy = 100*(sif.totalram-sif.freeram) /(sif.totalram);
    }
    else
    {
        return false;
    }
    return true;
}



bool system_info::compute_mem_occupy()
{
    return compute_mem_occupy_through_proc();
}



/*
 *@args:
 *@desc:compute the occupy of system's memory
 * computational formula:
 NET OCCUPY = (receive_packets + transmit_packets)/ 2
 */

bool system_info::compute_net_occupy()
{
    FILE *proc_net_handler;
    char line[LINE_SIZE];
    proc_net_handler = fopen (_proc_net.c_str(), "r");
    if( proc_net_handler == NULL )
    {
        return false;
    }

    //now filter the front three lines
    fgets (line, sizeof(line), proc_net_handler);
    fgets (line, sizeof(line), proc_net_handler);
    fgets (line, sizeof(line), proc_net_handler);
    fgets (line, sizeof(line), proc_net_handler);
    char heading[20];
    unsigned long occupy[10];
    sscanf (line, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
            heading, &occupy[0],&occupy[1],&occupy[2],&occupy[3],&occupy[4],
            &occupy[5],&occupy[6],&occupy[7],&occupy[8],&occupy[9]);
    if( _startup )
    {
        _initial_net_info.received_bytes = occupy[0];
        _initial_net_info.received_packets = occupy[1];
        _initial_net_info.transmited_bytes = occupy[8];
        _initial_net_info.transmited_packets = occupy[9];
        _start = clock();
    }
    else
    {
        _current_net_info.received_bytes = occupy[0];
        _current_net_info.received_packets = occupy[1];
        _current_net_info.transmited_bytes = occupy[8];
        _current_net_info.transmited_packets = occupy[9];
        _finish = clock();
    }
    return true;
}


double system_info::get_cpu_occupy()
{
    bool ok = compute_cpu_occupy();
    if( !ok )
    return -1;    
    double idle_differ = _initial_cpu_info.idle_mod - _current_cpu_info.idle_mod;

    double total_differ=(_initial_cpu_info.user_mod+_initial_cpu_info.system_mod+_initial_cpu_info.nice_mod
            +_initial_cpu_info.idle_mod+_initial_cpu_info.iowait_mod+_initial_cpu_info.irq_mod+_initial_cpu_info.softirq_mod)
        -(_current_cpu_info.user_mod+_current_cpu_info.system_mod+_current_cpu_info.nice_mod
                +_current_cpu_info.idle_mod+_current_cpu_info.iowait_mod+_current_cpu_info .irq_mod+_current_cpu_info.softirq_mod);

    _cpu_occupy = 100-idle_differ  / total_differ *100;
    return _cpu_occupy;
}


double system_info::get_mem_occupy()
{
    if(!compute_mem_occupy())
    return -1;    
    return _mem_occupy;
}



bool system_info::get_net_occupy(NET_INFO & net_info)
{
    if(!compute_net_occupy())
    return false;    
    net_info.received_bytes = _current_net_info.received_bytes-_initial_net_info.received_bytes;
    net_info.received_packets = _current_net_info.received_packets-_initial_net_info.received_packets;
    net_info.transmited_bytes = _current_net_info.transmited_bytes-_initial_net_info.transmited_bytes;
    net_info.transmited_packets = _current_net_info.transmited_packets-_initial_net_info.transmited_packets;
    int totaltime=(_finish -_start) /CLOCKS_PER_SEC;
    if( totaltime < 1 )
    totaltime = 1;
    memcpy(&_initial_net_info,&_current_net_info,sizeof(_current_net_info));
    _start = _finish;
    net_info.received_bytes /= totaltime;
    net_info.received_packets /= totaltime;
    net_info.transmited_bytes /= totaltime;
    net_info.transmited_packets /= totaltime;
    return true;
}

