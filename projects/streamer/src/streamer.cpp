/***********************************************************
 * @File Name:streamer.cpp
 * @Author:chenzhengqiang
 * @company:swwy
 * @Date 2015/3/26
 * @Modified:
   2015/9/14:add function for supporting HLS
 * @Version:1.0
 * @Desc:the kernel of the entire streaming server
 **********************************************************/

#include "errors.h"
#include "common.h"
#include "flv.h"
#include "streamer.h"
#include "state_server.h"
#include "netutility.h"
#include "serverutility.h"
#include "logging.h"
#include <sys/resource.h>

using std::string;
using std::cout;
using std::endl;
using std::queue;


static const int  ADTS_HEADER_SIZE = 7;
static const int  MAX_FRAME_HEAD_SIZE = 1024;


//specify the max open files
static const size_t MAX_OPEN_FDS=65535;
//specify the buffer size of client's request
static const size_t BUFFER_SIZE= 1024;

//specify the size of work threads pool
static const size_t WORKTHREADS_SIZE=8;

//specify the fragment size
static const size_t FRAGMENT_SIZE = 1024;
//specify the size of cached flv tag in queue
static const size_t CACHED_TAGS_LIMIT = 79;
//notify workthread entry related
static struct ev_loop *NOTIFY_SERVER_LOOP=NULL;
static struct ev_async *NOTIFY_ASYNC_WATCHER=NULL;
static char NOTIFY_SERVER_CONFIG_FILE[50];
/*****************************WORKTHREAD  INFO  RELATED*******************************/
static const size_t AUDIO_TAG = 8;
static const size_t VIDEO_TAG = 9;
static const size_t SCRIPT_TAG = 18;
static const size_t TAG_DATA_SIZE=1024;
static const size_t VIEWERS_LIMIT = 2000;
static const size_t CHANNELS_LIMIT = 100;
static const size_t MAX_CHANNEL_SIZE=32;
static const int ONLINE = 0;
static const int OFFLINE = 1;


//the flv tag item
struct FLV_TAG_FRAME
{
    bool  is_key_frame;
    uint8_t   *TAG;
    size_t  data_size;
    size_t  sent_bytes;
};


//the viewer's queue structure
struct VIEWER_QUEUE
{
    FLV_TAG_FRAME *flv_tags[CACHED_TAGS_LIMIT];
    size_t front;
    size_t rear;
    bool empty;
};


//every signle viewer own a viewer queue
typedef struct
{
    ssize_t client_fd;
    bool send_first; //specify that if it's the first time to send tags to viewer
    bool register_viewer_callback_first;
    bool send_key_frame_first;	//specify that if it's the first time to send key-frame to viewer
    bool not_sent_flv_header_done;
    bool not_sent_http_response_done;
    bool not_sent_script_tag_done;
    bool not_sent_aac_sequence_header_done;
    bool not_sent_avc_sequence_header_done;
    struct ev_io * viewer_watcher;
    VIEWER_QUEUE viewer_queue;
}VIEWER_INFO,*VIEWER_INFO_PTR;


//the workthread's structure,attach the camera's channel to workthread's ID
//save the aac_sequence_header,avc_sequence_header for the media player's sake,
//you need to push them for the first time
//either push the flv tags to viewer through the viewer_queue
typedef std::string CHANNEL;
typedef ssize_t CLIENT_ID;
typedef ssize_t REQUEST_FD;
typedef struct _CAMERAS
{
    bool receive_first;
    bool script_tag_exists;
    char IP[INET_ADDRSTRLEN];
    struct ev_io *camera_watcher;
    struct ev_io *reply_watcher;
    // the flv header hold 9 bytes,and the first PreviousTagSize0 hold 4 bytes but always 0
    // so it's specified 9+4 bytes
    uint8_t   flv_header[FLV_HEADER_SIZE+PREVIOUS_TAG_SIZE];
    bool not_received_flv_header_done;
    size_t flv_header_sent_bytes;
    size_t flv_header_received_bytes;

    uint8_t tag_header[TAG_HEADER_SIZE];
    bool not_received_tag_header_done;
    size_t tag_header_received_bytes;

    uint8_t *tag_data;
    bool not_received_tag_data_done;
    size_t tag_data_size;
    size_t tag_data_received_bytes;

    //for flv script tag
    uint8_t  *flv_script_tag;
    size_t  script_tag_total_bytes;
    size_t  script_tag_sent_bytes;

    //for flv's aac sequence header tag
    uint8_t  *aac_sequence_header;
    size_t  aac_sequence_header_total_bytes;
    size_t  aac_sequence_header_sent_bytes;

    //for flv's avc sequence header tag
    uint8_t  *avc_sequence_header;
    size_t  avc_sequence_header_total_bytes;
    size_t  avc_sequence_header_sent_bytes;
    std::map<CLIENT_ID, VIEWER_INFO_PTR> viewer_info_pool;

}CAMERAS,*CAMERAS_PTR;



/*
 *@desc:every single thread owns a workthread_loop
 * to provide workthread's event loop
 * and the "async_watcher" is used to wake up our workthread event loop
 */
struct RESOURCE_INFO
{
     std::string IP;
     std::set<CLIENT_ID> socketfd_pool;
};


struct WORKTHREAD_INFO
{
    pthread_t thread_id;
    struct ev_loop *workthread_loop;
    struct ev_async *async_watcher;
    std::map<CHANNEL,CAMERAS_PTR> camera_info_pool;
    std::map<CHANNEL,RESOURCE_INFO> resource_info_pool;
};
static std::vector<WORKTHREAD_INFO> workthread_info_pool;



/*
#@desc:usefull aliases for c++ container's iterator
*/
typedef std::map<CHANNEL,CAMERAS_PTR>::iterator & camera_info_reference_iter;
typedef std::map<CHANNEL,CAMERAS_PTR>::iterator  camera_info_iter;

typedef std::map<CHANNEL,RESOURCE_INFO>::iterator & backing_resource_reference_iter;
typedef std::map<CHANNEL,RESOURCE_INFO>::iterator  backing_resource_iter;

typedef std::vector<WORKTHREAD_INFO>::iterator & workthread_info_reference_iter;
typedef std::vector<WORKTHREAD_INFO>::iterator workthread_info_iter;

typedef std::map<CLIENT_ID, VIEWER_INFO_PTR>::iterator & viewer_info_reference_iter;
typedef std::map<CLIENT_ID, VIEWER_INFO_PTR>::iterator  viewer_info_iter;


/*********************************END***********************************************/


struct EV_ASYNC_DATA
{
    size_t client_type;
    ssize_t client_fd;
    ssize_t request_fd;
    char *data;
    char channel[MAX_CHANNEL_SIZE];
};


static const size_t CAMERA = 0;
static const size_t VIEWER = 1;
static const size_t BACKER = 2;
static const size_t NOTIFY = 3;


/*********************************END*************************************/





/***************************the implementation of functions*********************/


/*
#@desc:usefull macros for logging
*/
#define LOG_START(X)  log_module(LOG_DEBUG,X,"###START###:%s",LOG_LOCATION)
#define LOG_DONE(X)   log_module(LOG_DEBUG,X,"###DONE### :%s",LOG_LOCATION)



/*
#@args:
#@returns:
#@desc:as the function name described
*/
static inline void free_workthread_info_pool(void)
{
    LOG_START("FREE_WORKTHREAD_INFO_POOL");
    std::vector<WORKTHREAD_INFO>::iterator wi_iter =  workthread_info_pool.begin();
    while( wi_iter != workthread_info_pool.end() )
    {
        if( wi_iter->workthread_loop != NULL )
        {
             free(wi_iter->workthread_loop);
             wi_iter->workthread_loop = NULL;
        }

        if( wi_iter->async_watcher != NULL )
        {
             free(wi_iter->async_watcher);
             wi_iter->async_watcher = NULL;
        }

        wi_iter->resource_info_pool.clear();
        camera_info_iter ci_iter = wi_iter->camera_info_pool.begin();
        while( ci_iter != wi_iter->camera_info_pool.end() )
        {
            viewer_info_iter vi_iter = ci_iter->second->viewer_info_pool.begin();
            while( vi_iter != ci_iter->second->viewer_info_pool.end() )
            {
                delete vi_iter->second;
                vi_iter->second = NULL;
                ++vi_iter;
            }
            ci_iter->second->viewer_info_pool.clear();
            delete ci_iter->second;
            ci_iter->second = NULL;
            ++ci_iter;
        }
        ++wi_iter;
    }
    workthread_info_pool.clear();
    LOG_DONE("FREE_WORKTHREAD_INFO_POOL");
}



/*
#@args:void
#@returns:CAMERAS_PTR
#@desc:new camera_ptr and initialize it
*/
CAMERAS_PTR new_cameras( struct ev_io * receive_request_watcher, const std::string & channel )
{
        if( receive_request_watcher == NULL )
        {
            log_module( LOG_ERROR, "NEW_CAMERAS", "RUNTIME ERROR OCCURRED: RECEIVE_REQUEST_WATCHER IS NOT VALID");
            return NULL;
        }
        
        CAMERAS_PTR  camera_ptr = new CAMERAS;
        if( camera_ptr == NULL )
        {
            log_module( LOG_ERROR,"NEW_CAMERAS","ALLOCATE MEMORY FAILED WHEN EXECUTE NEW CAMERAS:%s", strerror( errno ) );
            return NULL;
        }

        
        //CAMERAS_PTR INITIALIZATION START
        camera_ptr->camera_watcher = new struct ev_io;
        if( camera_ptr->camera_watcher == NULL )
        {
             log_module( LOG_ERROR, "NEW_CAMERAS", "ALLOCATE MEMORY FAILED WHEN EXECUTE NEW STRUCT EV_IO" );
             delete camera_ptr;
             camera_ptr = NULL;
             return NULL;
        }
        
        camera_ptr->reply_watcher = new struct ev_io;
        if( camera_ptr->reply_watcher == NULL )
        {
             log_module( LOG_ERROR, "NEW_CAMERAS", "ALLOCATE MEMORY FAILED:REPLY_WATCHER=NULL:%s", strerror( errno ) );
             delete camera_ptr->camera_watcher;
             camera_ptr->camera_watcher = NULL;
             delete camera_ptr;
             camera_ptr = NULL;
             return NULL;
        }

        char *cstr_channel = new char[channel.length()+1];
        if( cstr_channel == NULL )
        {
             log_module( LOG_ERROR,"NEW_CAMERAS","ALLOCATE MEMORY FAILED WHEN EXECUTE NEW CHAR[...]:%s", strerror(errno) );
             delete camera_ptr->camera_watcher;
             camera_ptr->camera_watcher = NULL;
             delete camera_ptr->reply_watcher;
             camera_ptr->reply_watcher = NULL;
             delete camera_ptr;
             camera_ptr = NULL;
             return NULL;
        }

        //you can not use the memset to initialize this struct
        //it will cause the segment default in run time
	std::string peer_ip = get_peer_info( receive_request_watcher->fd, 0 );
	if( peer_ip.empty() )
	{
	    strcpy( camera_ptr->IP, "unknown ip");
	}
	else
	{
	    strncpy( camera_ptr->IP, peer_ip.c_str(), INET_ADDRSTRLEN );
	}

        camera_ptr->camera_watcher->data = (void *)cstr_channel;
        camera_ptr->camera_watcher->fd = receive_request_watcher->fd;
        strcpy(cstr_channel,channel.c_str());
        camera_ptr->receive_first = true;
        camera_ptr->script_tag_exists = false;
        camera_ptr->not_received_flv_header_done = true;
        camera_ptr->not_received_tag_header_done = true;
        camera_ptr->not_received_tag_data_done = true;
        camera_ptr->script_tag_sent_bytes = 0;
        camera_ptr->flv_header_received_bytes = 0;
        camera_ptr->flv_header_sent_bytes = 0;
        camera_ptr->tag_header_received_bytes = 0;
        camera_ptr->tag_data_received_bytes = 0;
        camera_ptr->avc_sequence_header_total_bytes = 0;
        camera_ptr->aac_sequence_header_total_bytes = 0;
        camera_ptr->avc_sequence_header_sent_bytes = 0;
        camera_ptr->aac_sequence_header_sent_bytes = 0;
        camera_ptr->flv_script_tag = NULL;
        camera_ptr->aac_sequence_header = NULL;
        camera_ptr->avc_sequence_header = NULL;
        camera_ptr->tag_data = NULL;
        return camera_ptr;
}



/*
#@args:void
#@returns:VIEWER_INFO_PTR
#@desc:new viewer_info_ptr and initialize it
*/
VIEWER_INFO_PTR new_viewer_info( struct ev_io * receive_request_watcher )
{
       if( receive_request_watcher == NULL || receive_request_watcher->fd < 0 )
       {
            log_module( LOG_ERROR, "NEW_VIEWER_INFO", "RUNTIME ERROR OCCURRED:RECEIVE_REQUEST_WATCHER IS NOT VALID");
            return NULL;
       }
       
       VIEWER_INFO_PTR viewer_info_ptr = new VIEWER_INFO;
       if( viewer_info_ptr == NULL )
       {
            log_module( LOG_ERROR, "NEW_VIEWER_INFO","ALLOCATE MEMORY FAILED:%s", strerror( errno));
            return NULL;
       }

       viewer_info_ptr->viewer_watcher = new struct ev_io;
       if( viewer_info_ptr->viewer_watcher == NULL )
       {
            log_module(LOG_INFO,"NEW_VIEWER_INFO","ALLOCATE MEMORY FAILED:%s", strerror(errno) );
            delete viewer_info_ptr;
            viewer_info_ptr = NULL;
            return NULL;
       }
       
       viewer_info_ptr->send_first = true;
       viewer_info_ptr->register_viewer_callback_first = true;
       viewer_info_ptr->send_key_frame_first = true;
       viewer_info_ptr->client_fd = receive_request_watcher->fd;
          
       viewer_info_ptr->not_sent_http_response_done = true;
       viewer_info_ptr->not_sent_flv_header_done = true;
       viewer_info_ptr->not_sent_avc_sequence_header_done = true;
       viewer_info_ptr->not_sent_aac_sequence_header_done = true;
       viewer_info_ptr->not_sent_script_tag_done = true;
       viewer_info_ptr->viewer_queue.empty = true;
       viewer_info_ptr->viewer_queue.front = CACHED_TAGS_LIMIT;
       viewer_info_ptr->viewer_queue.rear = CACHED_TAGS_LIMIT;

       return viewer_info_ptr;
}



/*
#@args:channel[in]
#@returns:true inidicates channel exists,false otherwise
#@desc:as the function name described
*/
bool camera_already_in( const string & channel )
{
    workthread_info_iter wi_iter = workthread_info_pool.begin();
    while( wi_iter != workthread_info_pool.end() )
    {
        camera_info_iter ci_iter = wi_iter->camera_info_pool.find(channel);
        if( ci_iter != wi_iter->camera_info_pool.end() )
            return true;
        ++wi_iter;
    }
    return false;
}


/*
#@args:channel[in]
#@returns:true inidicates channel exists,false otherwise
#@desc:as the function name described
*/
bool resource_already_in( const string & channel, workthread_info_reference_iter wir_iter )
{
    while( wir_iter != workthread_info_pool.end() )
    {
        backing_resource_iter br_iter = wir_iter->resource_info_pool.find(channel);
        if( br_iter != wir_iter->resource_info_pool.end() )
        return true;    
        ++wir_iter;
    }
    return false;
}



/*
#@args:
#@returns:
#@desc:as the function name described,find the channel info's iterator according to the channel
*/
static inline camera_info_iter get_channel_info_item(workthread_info_reference_iter wir_iter, const string & channel )
{
    return wir_iter->camera_info_pool.find(channel);
}



bool get_channel_info_item( const string & channel,camera_info_reference_iter cir_iter )
{
    workthread_info_iter wi_iter = workthread_info_pool.begin();
    while( wi_iter != workthread_info_pool.end() )
    {
        cir_iter = wi_iter->camera_info_pool.find(channel);
        if( cir_iter != wi_iter->camera_info_pool.end() )
            return true;
        ++wi_iter;
    }
    return false;
}


camera_info_iter get_channel_info_item(workthread_info_reference_iter wir_iter ,const CLIENT_ID &client_fd)
{
     camera_info_iter ci_iter = wir_iter->camera_info_pool.begin();
     while( ci_iter != wir_iter->camera_info_pool.end() )
     {
          if( ci_iter->second->camera_watcher->fd == client_fd )
          return ci_iter;
          ++ci_iter;
     }
     return ci_iter;
}


/*
#@args:
#@returns:
#@desc:as the function name described,find the workthread according to the thread_id
*/
workthread_info_iter get_workthread_info_item(pthread_t thread_id)
{
    workthread_info_iter wi_iter = workthread_info_pool.begin();
    while( wi_iter != workthread_info_pool.end() )
    {
        if(pthread_equal(wi_iter->thread_id, thread_id) )
            break;
        ++wi_iter;
    }
    return wi_iter;
}



/*
#@args:
#@returns:
#@desc:as the function name described,find the viewer info's iterator according to the client_id
# notice that,the client_id here is socket fd
*/

static inline viewer_info_iter get_viewer_info_item( camera_info_reference_iter cir_iter, const CLIENT_ID & client_id )
{
    return cir_iter->second->viewer_info_pool.find(client_id);
}



/*
#@desc:find the viewer info's iterator according to the client_id
# notice that,the client_id here is socket fd
*/
int get_viewer_info_item( workthread_info_reference_iter wir_iter, camera_info_reference_iter cir_iter,
							 viewer_info_reference_iter vir_iter, const CLIENT_ID & client_id )
{
    if( client_id < 0 )
    {
    	 log_module( LOG_ERROR, "GET_VIEWER_INFO_ITEM", "INVALID ARGUMENT RELATED CLIENT ID:%d", client_id );
	 return -1;		 
    }
	
    cir_iter = wir_iter->camera_info_pool.begin();
    while( cir_iter != wir_iter->camera_info_pool.end() )
    {
     	 vir_iter = cir_iter->second->viewer_info_pool.find( client_id );
        if( vir_iter != cir_iter->second->viewer_info_pool.end() )
        {
             return OK;
        }
        ++cir_iter;
    }

    log_module( LOG_INFO, "GET_VIEWER_INFO_ITEM", "GET VIEWER INFO ITEM FAILED RELATED TO CLIENT ID:%d", client_id );			
    return -1;		  
}


/*
*@args:
*@retuns:
*@desc:
*/
backing_resource_iter get_backing_resource_item( workthread_info_reference_iter wir_iter, const CLIENT_ID & client_id )
{
     
     backing_resource_iter br_iter;
     br_iter = wir_iter->resource_info_pool.begin();
     while( br_iter != wir_iter->resource_info_pool.end() )
     {
           if( br_iter->second.socketfd_pool.find(client_id) != br_iter->second.socketfd_pool.end())
           return br_iter;
           ++br_iter;
     }  
     return br_iter;
}



/*
#@args:VIEWER_QUEUE[IN],the data struct of our viewer's queue
#@returns:return a boolean type indicates if the viewer's queue is full
#@desc:if the viewer's queue is not empty and the "front" equals to "rear"
#   then we know that the queue is full
*/
static inline bool viewer_queue_full( VIEWER_QUEUE & viewer_queue )
{
    return (!viewer_queue.empty && (viewer_queue.front == viewer_queue.rear));
}


static inline bool viewer_queue_empty(const VIEWER_QUEUE & viewer_queue)
{
    return viewer_queue.empty;
}



/*
#@args:VIEWER_QUEUE[IN],FLV_TAG_FRAME[OUT]
#@returns:
#@desc:pop the tag in the queue into flv_tag
notice:it's a looping queue
*/

static void viewer_queue_pop( VIEWER_QUEUE & viewer_queue )
{
    if( viewer_queue_empty( viewer_queue ))
        return;
    
    delete [] (viewer_queue.flv_tags[viewer_queue.front])->TAG;
    (viewer_queue.flv_tags[viewer_queue.front])->TAG = NULL;

    delete viewer_queue.flv_tags[viewer_queue.front];
    viewer_queue.flv_tags[viewer_queue.front] = NULL;

    viewer_queue.front = (viewer_queue.front+1) % CACHED_TAGS_LIMIT;
    if( viewer_queue.front == viewer_queue.rear )
    viewer_queue.empty = true;
}


/*
#@args:VIEWER_QUEUE[IN],FLV_TAG_FRAME[OUT]
#@returns:
#@desc:obtain the flv_tag in the queue's head but not pop
*/
static void viewer_queue_top( VIEWER_QUEUE & viewer_queue, FLV_TAG_FRAME * &flv_tag)
{
    if( viewer_queue_empty(viewer_queue) )
        return;
    flv_tag = viewer_queue.flv_tags[viewer_queue.front];
}



/*
#@args:VIEWER_QUEUE[IN]
#@returns:
#@desc:as its name described,delete all the elements in viewer queue
*/
void delete_viewer_queue( VIEWER_QUEUE & viewer_queue )
{
    while( ! viewer_queue_empty(viewer_queue))
    {
         viewer_queue_pop(viewer_queue);
    }
}



/*
#@desc:as its name described,push the flv tag into the viewer's queue
*/
static void viewer_queue_push( VIEWER_QUEUE & viewer_queue, FLV_TAG_FRAME & flv_tag)
{
    viewer_queue.empty = false;
    if( viewer_queue.rear == CACHED_TAGS_LIMIT )
    {
        viewer_queue.rear = 0;
    }

    if( viewer_queue.front == CACHED_TAGS_LIMIT )
    viewer_queue.front = 0;
    
    viewer_queue.flv_tags[viewer_queue.rear] = new FLV_TAG_FRAME;
    if( viewer_queue.flv_tags[viewer_queue.rear] == NULL )
    {
        log_module(LOG_ERROR,"VIEWER_QUEUE_PUSH","Allocate Memory Failed:%s",LOG_LOCATION);
    }

    (viewer_queue.flv_tags[viewer_queue.rear])->is_key_frame= flv_tag.is_key_frame;
    (viewer_queue.flv_tags[viewer_queue.rear])->data_size = flv_tag.data_size;
    (viewer_queue.flv_tags[viewer_queue.rear])->TAG = new uint8_t[flv_tag.data_size];
    if(  ( viewer_queue.flv_tags[viewer_queue.rear])->TAG == NULL  )
    {
        log_module(LOG_ERROR,"VIEWER_QUEUE_PUSH","Allocate Memory Failed:%s",LOG_LOCATION);
        exit(EXIT_FAILURE);
    }
    memcpy((viewer_queue.flv_tags[viewer_queue.rear])->TAG, flv_tag.TAG, flv_tag.data_size);
    (viewer_queue.flv_tags[viewer_queue.rear])->sent_bytes = 0;
    viewer_queue.rear = ( viewer_queue.rear+1) % CACHED_TAGS_LIMIT;
}



/*
#@args:"workthreads_size" specify the size of work threads
#@returns:no returns
#@desc:create the work thread in a loop,meanwhile saving them to the pool
*/
struct WORKTHREAD_LOOP_INFO
{
    struct ev_loop    *workthread_loop;
    struct ev_async  *async_watcher;
    size_t type;
};

void get_notify_servers( const char * config_file,std::map<std::string, int> & notify_server_pool )
{
      std::ifstream ifile(config_file);
      if( !ifile )
     {
           log_module( LOG_ERROR,"GET_NOTIFY_SERVERS","FAILED TO OPEN FILE:%s--%s",config_file, strerror(errno) );
           return;
     }
	  
     std::string line;
     std::string IP;
     std::string PORT;
     while(getline(ifile,line))
     {
          IP="";
          PORT="";
          if(line.empty())
          continue;
          std::string::const_iterator line_iter = line.begin();
          while( line_iter != line.end() )
          {
               if( *line_iter == '#' )
               break;
               while( *line_iter == ' ' && line_iter != line.end() )
               {
                     ++line_iter;
               }
               if( line_iter == line.end() )
               break;
               while( *line_iter !=':' && line_iter != line.end() )
               {
                   IP.push_back(*line_iter);
                   ++line_iter;
               }
               if( line_iter == line.end() )
               break;
               ++line_iter;
               while( line_iter != line.end() && *line_iter == ' ')
               {
                   ++line_iter;
               }
               while( line_iter != line.end() )
               {
                    if( *line_iter == ' ' )
                    break;    
                    PORT.push_back(*line_iter);
                    ++line_iter;
               }
               notify_server_pool.insert( std::make_pair(IP,atol(PORT.c_str())) );
               break;
          }
     }
}

bool init_workthread_info_pool( size_t workthreads_size )
{
    LOG_START( "INIT_WORKTHREAD_INFO_POOL" );
    ssize_t ret;
    //create our work threads in a loop
    log_module(LOG_DEBUG,"INIT_WORKTHREAD_INFO_POOL","CAMERA STREAM RELATED--START" );
    pthread_attr_t thread_attr;
    pthread_t thread_id;
    WORKTHREAD_LOOP_INFO * workthread_loop_info = NULL;
    for( size_t index=0; index < workthreads_size; ++index )
    {

        //make the workthread detached to main thread
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate( &thread_attr, PTHREAD_CREATE_DETACHED );
        
        WORKTHREAD_INFO workthread_info;
        workthread_loop_info = new WORKTHREAD_LOOP_INFO;
        if( workthread_loop_info == NULL )
        {
            log_module(LOG_ERROR,"INIT_WORKTHREAD_INFO_POOL","MALLOC EV_LOOP_INFO FAILED");
            return false;
        }

        //init the workthread's event loop
        workthread_loop_info->workthread_loop = ev_loop_new( EVBACKEND_EPOLL );
        if( workthread_loop_info->workthread_loop == NULL )
        {
            delete workthread_loop_info;
            workthread_loop_info = NULL;
            log_module( LOG_ERROR,"INIT_WORKTHREAD_INFO_POOL","EV_LOOP_NEW FAILD");
            return false;
        }

        //init the workthread's async watcher
        workthread_loop_info->async_watcher = new ev_async;
        if(  workthread_loop_info->async_watcher == NULL )
        {
             free( workthread_loop_info->workthread_loop );
             workthread_loop_info->workthread_loop = NULL;
             delete workthread_loop_info;
             workthread_loop_info = NULL;
             log_module( LOG_ERROR,"INIT_WORKTHREAD_INFO_POOL","INIT THE SPECIFIED WORKTHREAD'S ASYNC WATCHER FAILED");
             return false;
        }
        
        workthread_info.workthread_loop = workthread_loop_info->workthread_loop;
        workthread_info.async_watcher = workthread_loop_info->async_watcher;
        workthread_loop_info->type = 99;
        ret = pthread_create( &thread_id,&thread_attr,workthread_entry, (void *)workthread_loop_info);
        if(  ret != 0 )
        {
            log_module( LOG_ERROR,"INIT_WORKTHREAD_POOL","PTHREAD_CREATE FAILED:%s", strerror(errno));
            return false;
        }
        workthread_info.thread_id = thread_id;
        workthread_info_pool.push_back( workthread_info );
    }
    
    log_module(LOG_DEBUG,"INIT_WORKTHREAD_INFO_POOL","WORKTHREADS RELATED TO CAMERA AND VIEWER CREATED DONE");
    //now startup the work thread for notifying camera's status to those servers of GSLB 
    
    log_module(LOG_DEBUG,"INIT_WORKTHREAD_INFO_POOL","INIT THE STREAMER'S NOTIFY SERVER ENTRY--START");
    pthread_attr_init( &thread_attr );
    pthread_attr_setdetachstate( &thread_attr, PTHREAD_CREATE_DETACHED );

    ret = pthread_create( &thread_id, &thread_attr, notify_server_entry, NULL );
    if(  ret != 0 )
    {
          log_module(LOG_ERROR,"INIT_WORKTHREAD_POOL","PTHREAD_CREATE FAILED:%s", strerror( errno ));
          return false;
    }
    
    log_module( LOG_DEBUG,"INIT_WORKTHREAD_INFO_POOL","INIT THE STREAMER'S NOTIFY SERVER ENTRY--DONE");
    LOG_DONE("INIT_WORKTHREAD_INFO_POOL");
    return true;
}



/*
#@desc:providing the idle callback function in workthread event loop,in which
# you can do the logging either collecting the statistics of streaming server
# and so on
*/
void workthread_idle_cb(struct ev_loop * workthread_loop, struct ev_idle *idle_watcher, int revents)
{
    (void)workthread_loop;
    (void)idle_watcher;
    (void)revents;
}




void viewer_queue_delete( VIEWER_QUEUE & viewer_queue, int pos )
{
    if( viewer_queue_empty( viewer_queue ))
    return;
    
    delete [] (viewer_queue.flv_tags[pos])->TAG;
    (viewer_queue.flv_tags[pos])->TAG = NULL;

    delete viewer_queue.flv_tags[pos];
    viewer_queue.flv_tags[pos] = NULL;

    //FLV_TAG_FRAME *flv_tags[CACHED_TAGS_LIMIT];
    
   do
   {
        viewer_queue.flv_tags[pos]=viewer_queue.flv_tags[(pos+1) % CACHED_TAGS_LIMIT ];
        pos = (pos+1) % CACHED_TAGS_LIMIT;
   }while( pos != (int)viewer_queue.rear );
   
   if( viewer_queue.rear == 0 )
   {
        viewer_queue.rear = CACHED_TAGS_LIMIT-1;
   }
   else
   {
        viewer_queue.rear = viewer_queue.rear-1;
   }

   viewer_queue.flv_tags[viewer_queue.rear] = NULL;
}

/*
#@args:viewer_queue[IN]
#@returns:
#@desc:just drop the tags in the viwer's queue,none-key frame first
#   and if there are full of key-frame in viewer's queue,then just drop the
#   tag in the queue's head
*/
void drop_frame( VIEWER_QUEUE & viewer_queue )
{
    //firstly find the none key frame in this viewer's queue, if failed
    //directly pop item from viewer's queue
    if( viewer_queue_empty(viewer_queue))
    return;

    //viewer_queue_pop(viewer_queue);
    int pointer = viewer_queue.front;
    do
    {
        //now go to find the none key frame from viewer's queue
        if( !( (viewer_queue.flv_tags[pointer])->is_key_frame ) )
        {
            viewer_queue_delete( viewer_queue,pointer );
            return;
        }
        pointer= ( pointer+1 ) % CACHED_TAGS_LIMIT;
    } while( pointer != (int)viewer_queue.rear );
   
    if( pointer == (int) viewer_queue.rear )
    {
        viewer_queue_pop(viewer_queue);
    }
   
}



void do_channel_notify( const std::string & channel , const int flag )
{
      log_module( LOG_DEBUG, "DO_CHANNEL_NOTIFY", "RELATED TO CHANNEL:%s", channel.c_str() );
      if( ev_async_pending( NOTIFY_ASYNC_WATCHER ) == 0 )
      {
             NOTIFY_DATA * notify_data = new NOTIFY_DATA;
             strcpy( notify_data->channel, channel.c_str() );
             notify_data->flag = flag;
             NOTIFY_ASYNC_WATCHER->data = (void *)notify_data;
             ev_async_send( NOTIFY_SERVER_LOOP, NOTIFY_ASYNC_WATCHER );
             log_module( LOG_DEBUG, "DO_CHANNEL_NOTIFY", "+++++SENT THE %s MESSAGE TO NOTIFY WORKTHREAD RELATED TO CHANNEL %s DONE+++++",
                                                        (flag==ONLINE)?"ONLINE":"OFFLINE",channel.c_str() );
      }
      else
      {
            log_module( LOG_ERROR, "DO_CHANNEL_NOTIFY", "+++++SENT THE ONLINE MESSAGE TO NOTIFY WORKTHREAD RELATED TO CHANNEL %s FAILED+++++",
                                                        channel.c_str() );
      }
      log_module( LOG_DEBUG, "DO_CHANNEL_NOTIFY", "RELATED TO CHANNEL:%s", channel.c_str() );
}



void delete_camera( struct ev_loop * workthread_loop, struct  ev_io *camera_watcher, 
                                    workthread_info_reference_iter wir_iter, camera_info_reference_iter cir_iter )
{
        if( workthread_loop == NULL || camera_watcher == NULL )
        {
            log_module( LOG_ERROR, "DELETE_CAMERA", "RUNTIME ERROR OCCURRED,WORKTHREADLOOP OR CAMERA WATCHER IS NULL");
            return;
        }
        
        log_module( LOG_DEBUG, "DELETE_CAMERA", "++++++++++START++++++++++");
        do_channel_notify( cir_iter->first, OFFLINE );
        
        if( !cir_iter->second->viewer_info_pool.empty() )
        {
            viewer_info_iter vi_iter = cir_iter->second->viewer_info_pool.begin();
            while( vi_iter != cir_iter->second->viewer_info_pool.end() )
            {
                if( vi_iter->second->viewer_watcher != NULL )
                {
                    ev_io_stop( workthread_loop, vi_iter->second->viewer_watcher );
                    close( vi_iter->second->viewer_watcher->fd  );
                    delete vi_iter->second->viewer_watcher;
                    vi_iter->second->viewer_watcher = NULL;
                }
                
                delete_viewer_queue( vi_iter->second->viewer_queue );
                delete vi_iter->second;
                vi_iter->second=NULL;
                ++vi_iter;
            }
            cir_iter->second->viewer_info_pool.clear();
        }

        if( loglevel_is_enabled( LOG_INFO ) )
        {
            if( camera_watcher->fd > 0 )
            {
                std::string peer_info = get_peer_info( camera_watcher->fd );
                log_module( LOG_INFO, "DELETE_CAMERA", "CAMERA %s OFFLINE RELATED TO CHANNEL:%s", peer_info.c_str(), 
                                                                              cir_iter->first.c_str() );
            }
        }
        
        if( camera_watcher->fd > 0 )
        {
            close( camera_watcher->fd );
        }
        else
        {
            log_module( LOG_ERROR, "DELETE_CAMERA", "RUNTIME ERROR OCCURRED,SOCKET FD IS NOT VALID RELATED TO CHANNEL:%s",
                                                                             cir_iter->first.c_str() );
        }
        
        ev_io_stop( workthread_loop, camera_watcher);
        delete camera_watcher;
        camera_watcher = NULL;
        if( cir_iter->second->tag_data != NULL )
        delete [] cir_iter->second->tag_data;
        cir_iter->second->tag_data = NULL;
        if( cir_iter->second->flv_script_tag != NULL )
        delete [] cir_iter->second->flv_script_tag;
        cir_iter->second->flv_script_tag = NULL;
        if( cir_iter->second->avc_sequence_header != NULL )
        delete [] cir_iter->second->avc_sequence_header;
        cir_iter->second->avc_sequence_header = NULL;
        if( cir_iter->second->aac_sequence_header != NULL )
        delete [] cir_iter->second->aac_sequence_header;
        cir_iter->second->aac_sequence_header = NULL;
        delete cir_iter->second;
        cir_iter->second = NULL;

        log_module( LOG_INFO, "DELETE_CAMERA", "ERASE THE CHANNEL %s FROM CAMERAS CONTAINER", cir_iter->first.c_str() );
        wir_iter->camera_info_pool.erase( cir_iter );
        log_module( LOG_INFO, "DELETE_CAMERA", "ERASE DONE, THERE IS(ARE) REMAIN %u CAMERA(S)", 
                                                                      wir_iter->camera_info_pool.size() );
        
        log_module( LOG_DEBUG, "DELETE_CAMERA", "++++++++++DONE++++++++++");
        return;
   
}


/*
#@desc:receiving the stream pushed from the camera
*/
void receive_stream_cb( struct ev_loop * workthread_loop, struct  ev_io *camera_watcher, int revents )
{
    //usefull macros for deleting channel when camera has stoped to push stream
    //meanwhile delete all the viewers
    (void)workthread_loop;
    uint8_t tag_type;
    bool is_key_frame = false;
    bool tag_is_sequence_header = false;
    
    workthread_info_iter wi_iter = get_workthread_info_item( pthread_self() );
    char * cstr_channel =(char *)camera_watcher->data;
    if( cstr_channel == NULL )
    {
        log_module( LOG_ERROR, "RECEIVE_STREAM_CB", "RUNTIME ERROR OCCURRED,CAMERA WATCHER'S DATA IS NULL" );
        rosehttp_reply_with_status( 500, camera_watcher->fd );
	close( camera_watcher->fd );
        return;
    }
    
    string channel( cstr_channel );
    camera_info_iter ci_iter = get_channel_info_item( wi_iter, channel );
    if( ci_iter == wi_iter->camera_info_pool.end())
    {
        log_module( LOG_ERROR, "RECEIVE_STREAM_CB", "RUNTIME ERROR OCCURRED RELATED TO CHANNEL:%s", cstr_channel );
        rosehttp_reply_with_status( 500, camera_watcher->fd );
        ev_io_stop( workthread_loop, camera_watcher );
        close( camera_watcher->fd );
        delete [] cstr_channel;
        delete camera_watcher;
        return;
    }

    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"RECEIVE_STREAM_CB","EV_ERROR OCCURRED" );
        rosehttp_reply_with_status( 500, camera_watcher->fd );
        delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
        return;
    }



    //just initialize once for the capability's sake
   
    size_t received_bytes = 0;
    size_t total_bytes = 0;
    if(ci_iter->second == NULL )
    return;    
        
    if( ci_iter->second->receive_first )
    {
        //now parse the flv header and the first previous tag
        if( ci_iter->second->not_received_flv_header_done )
        {
            total_bytes = FLV_HEADER_SIZE+PREVIOUS_TAG_SIZE-ci_iter->second->flv_header_received_bytes;
            received_bytes = read_specify_size(camera_watcher->fd,
                    ci_iter->second->flv_header+ci_iter->second->flv_header_received_bytes,
                    total_bytes);
            if( received_bytes == 0 )
            {
                delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
                return;
            }
            ci_iter->second->flv_header_received_bytes += received_bytes;
            if( received_bytes != total_bytes )
            {
                return;
            }
            ci_iter->second->not_received_flv_header_done = false;
        }

        log_module( LOG_DEBUG,"RECEIVE_STREAM_CB","SIGNATURE:%c%c%c VERSION:%d",
                ci_iter->second->flv_header[0],ci_iter->second->flv_header[1],
                ci_iter->second->flv_header[2],ci_iter->second->flv_header[3]);

        if(( ci_iter->second->flv_header[4] & 0x04 ) == 4 )
        {
            log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","VIDEO ARE PRESENT");
        }
        if(( ci_iter->second->flv_header[4] & 0x01 ) == 1 )
        {
            log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","AUDIO ARE PRESENT");
        }
        ci_iter->second->receive_first = false;
        log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","NOW ALWAYS RECEIVE THE STREAM PUSHED FROM THE CAMERA UNLESS STOPPED");
    }


    if( ci_iter->second->not_received_tag_header_done )
    {
        log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","NOW PARSE THE FLV TAG HEADER"); 
        total_bytes = TAG_HEADER_SIZE-ci_iter->second->tag_header_received_bytes;
        received_bytes = read_specify_size(camera_watcher->fd,
                ci_iter->second->tag_header+ci_iter->second->tag_header_received_bytes , total_bytes);

        if( received_bytes == 0 )
        {
              delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
              return;
        }
        
        ci_iter->second->tag_header_received_bytes += received_bytes;
	 if( received_bytes != total_bytes )
	 return;
       
	ci_iter->second->not_received_tag_header_done = false;
       ci_iter->second->tag_header_received_bytes = 0;

       //compute the data size
       ci_iter->second->tag_data_size = ci_iter->second->tag_header[1] * 65536+ci_iter->second->tag_header[2]*256
            +ci_iter->second->tag_header[3]+PREVIOUS_TAG_SIZE;
       //detailed information about kinds of field of flv media file,see the document pulished in adobe
        log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","PARSE THE FLV TAG HEADER OK--THE TOTAL TAG  SIZE IS %d BYTES",
        ci_iter->second->tag_data_size+TAG_HEADER_SIZE-PREVIOUS_TAG_SIZE);
       
        ci_iter->second->tag_data = new uint8_t[ci_iter->second->tag_data_size];
        if(  ci_iter->second->tag_data == NULL  )
        {
            log_module(LOG_ERROR,"RECEIVE_STREAM_CB","FAILED TO ALLOCATE MEMORY:%s",strerror(errno));
            delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
            return;
        }
        
    }

    
    if( ci_iter->second->not_received_tag_data_done )
    {
        
        log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "NOT RECEIVE THE TAG DATA DONE--TAG TOTAL BYTES:%d--RECEIVED BYTES:%d",
                                                                               ci_iter->second->tag_data_size,
                                                                               ci_iter->second->tag_data_received_bytes );
        
        total_bytes = ci_iter->second->tag_data_size-ci_iter->second->tag_data_received_bytes;
        received_bytes = read_specify_size( camera_watcher->fd,
                ci_iter->second->tag_data+ci_iter->second->tag_data_received_bytes,total_bytes );

        if( received_bytes == 0 )
        {
             delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
             return;
        }
        ci_iter->second->tag_data_received_bytes += received_bytes;
        if( received_bytes != total_bytes )
        {
            return;
        }
        
        ci_iter->second->not_received_tag_data_done = false;
        ci_iter->second->tag_data_received_bytes = 0;
    }

    
    size_t packet_size = TAG_HEADER_SIZE  + ci_iter->second->tag_data_size;
    uint8_t *packet = new uint8_t[packet_size];
    memset( packet, 0, packet_size);
    //std::vector<uint8_t> packet(packet_size);
    if( packet == NULL )
    {
        log_module( LOG_ERROR,"RECEIVE_STREAM_CB","ALLOCATE MEMORY FAILED:uint8_t *packet= new uint8_t[packet_size]");
        delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
        return;
    }

    memcpy( packet,ci_iter->second->tag_header,TAG_HEADER_SIZE);
    memcpy( packet+TAG_HEADER_SIZE, ci_iter->second->tag_data, ci_iter->second->tag_data_size);

    tag_type =  ci_iter->second->tag_header[0] & 0x1F;
    //log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","NOW PARSE THE FLV TAG :VIDEO TAG--AUDIO TAG--SCRIPT TAG--");

    switch(  tag_type )
    {
        case AUDIO_TAG:
            //now parse the flv audio tag
            log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","NOW PARSE THE FLV AUDIO TAG");
            if( ( ci_iter->second->tag_data[0] & 0xF0 ) >> 4 == 10 )
            {
                //log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","this is the AAC data");
                uint8_t aac_packet_type = ci_iter->second->tag_data[1];
                if( aac_packet_type == 0 )
                {
                    log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","FIND THE FLV AAC SEQUENCE HEADER");
                    tag_is_sequence_header = true;
                    ci_iter->second->aac_sequence_header = new uint8_t[packet_size];
                    if( ci_iter->second->aac_sequence_header == NULL )
                    {
                         log_module(LOG_INFO,"RECEIVE_STREAM_CB","ALLOCATE MEMORY FAILED:%s",strerror(errno));
                         delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
                    }
                    memcpy(ci_iter->second->aac_sequence_header,packet,packet_size);
                    ci_iter->second->aac_sequence_header_total_bytes = packet_size;
                    ci_iter->second->aac_sequence_header_sent_bytes = 0;
                    log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","SAVE THE FLV AAC SEQUENCE HEADER INTO CHANNEL POOL OK");
                }
                else if(  aac_packet_type == 1 )
                {
                    //log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","FIND THE FLV AAC AUDIO PAYLOAD");
                }
            }
            else
            {
                log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","ERROR:THIS ISN't THE AAC DATA");
            }
            break;
        case VIDEO_TAG:
            //handle the h264 first
            log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","NOW PARSE THE FLV VIDEO TAG");
            if ( (ci_iter->second->tag_data[0] & 0xF0) >> 4 == 1 )
            {
                is_key_frame = true;
                log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","FIND THE VIDEO'S KEY FRAME");
            }
            else
            {
                //log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","This Isn't Video's Key Frame");
            }

            if( ( ci_iter->second->tag_data[0]  & 0x0F ) == 7 )
            {
                log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","FIND THE AVC TAG");
                uint8_t avc_packet_type = ci_iter->second->tag_data[1];
                if( avc_packet_type == 0 )
                {
                    log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","FIND THE AVC SEQUENCE HEADER");
                    tag_is_sequence_header = true;
                    ci_iter->second->avc_sequence_header_total_bytes = packet_size;
                    ci_iter->second->avc_sequence_header_sent_bytes = 0;
                    ci_iter->second->avc_sequence_header = new uint8_t[packet_size];
                    if(  ci_iter->second->avc_sequence_header == NULL )
                    {
                         log_module(LOG_ERROR,"RECEIVE_STREAM_CB","ALLOCATE MEMORY FAILED:%s",strerror(errno));
                         delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
                         return;
                    }
                    memcpy(ci_iter->second->avc_sequence_header,packet,packet_size);
                    log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","SAVE THE AVC SEQUENCE HEADER INTO CHANNEL POOL OK");
                }
                else if( avc_packet_type == 1 )
                {
                    log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","RECEIVED THE AVC'S NALU DATA");
                }
                else 
                {
                    log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","ERROR:FLV FORMAT ERROR FOR VIDEO TAG");
                }
            }
            else
            {
                log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","ERROR:THIS ISN'T THE AVC TAG");
            }
            break;
        case SCRIPT_TAG:
            //now parse the flv script tag
            log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","FIND THE FLV SCRIPT TAG");
            tag_is_sequence_header = true;
            ci_iter->second->script_tag_exists = true;
            ci_iter->second->script_tag_total_bytes = packet_size;
            ci_iter->second->script_tag_sent_bytes = 0;
            ci_iter->second->flv_script_tag = new uint8_t[packet_size];
            if( ci_iter->second->flv_script_tag == NULL )
            {
                  log_module( LOG_ERROR,"RECEIVE_STREAM_CB","ALLOCATE MEMORY FAILED:%s",strerror(errno));
                  delete_camera( workthread_loop, camera_watcher, wi_iter, ci_iter );
                  return;
            }
            memcpy( ci_iter->second->flv_script_tag, packet,packet_size );
            log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","SAVE THE FLV SCRIPT TAG INTO CHANNEL POOL OK");
            break;
    }

    delete [] ci_iter->second->tag_data;
    ci_iter->second->tag_data = NULL;

    if( ! tag_is_sequence_header )
    {
        //if the flv tag is none of aac sequence header tag, svc sequence header tag or script tag
        //then merely push them into the viewer's queue
        log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","NOW BEGIN TO PUSH THE TAG FRAME INTO ALL VIEWERS' VIEWER QUEUE");
        FLV_TAG_FRAME flv_tag;
        flv_tag.TAG = packet;
        flv_tag.data_size = packet_size;
        flv_tag.sent_bytes = 0;
        flv_tag.is_key_frame = is_key_frame;

        //now parse tags ok and send the tags to all viewers
        bool need_start_viewer_callback = false;
        if( !( ci_iter->second->viewer_info_pool.empty() ) )
        {
            viewer_info_iter vi_iter = ci_iter->second->viewer_info_pool.begin();
            while(vi_iter != ci_iter->second->viewer_info_pool.end())
            {
                 if( vi_iter->second->register_viewer_callback_first )
                 {
                      vi_iter->second->register_viewer_callback_first = false;
                      ev_io_init(vi_iter->second->viewer_watcher,send_tags_cb,
                      vi_iter->second->client_fd,EV_WRITE);
                 }
                 
                 if( viewer_queue_empty( vi_iter->second->viewer_queue ))
                 {
                      need_start_viewer_callback = true;
                 }
                 
                 if( viewer_queue_full(vi_iter->second->viewer_queue) )
                 {
                      //network congestion might happened,now drop the none-key frame first
		      if( loglevel_is_enabled( LOG_DEBUG ))
		      {
			  std::string peer_info = get_peer_info( vi_iter->second->client_fd );  
			log_module( LOG_DEBUG,"RECEIVE_STREAM_CB","THE %s'S VIEWER QUEUE IS FULL--NETWORK CONGESTION HAPPENED",
				                                  peer_info.c_str());
		      }

                      drop_frame( vi_iter->second->viewer_queue );
                 }
                 
                 viewer_queue_push( vi_iter->second->viewer_queue, flv_tag );
                 if( need_start_viewer_callback )
                 {
                      need_start_viewer_callback = false;
                      ev_io_start( workthread_loop, vi_iter->second->viewer_watcher );
                 }
                 
		 if( loglevel_is_enabled( LOG_DEBUG ))
		 {
		     std::string peer_info = get_peer_info( vi_iter->second->client_fd ); 
                     log_module( LOG_DEBUG,"RECEIVE_STREAM_CB","ALREADY PUSH THE FLV TAG FRAME INTO %s'S VIEWER QUEUE",
                                                                peer_info.c_str() );
		 }
                 ++vi_iter;
            }
        }
        else
        {
           ;
        }
    }

    delete [] packet;
    packet = NULL;
    tag_is_sequence_header = false;
    is_key_frame = false;
    memset( ci_iter->second->tag_header,0,TAG_HEADER_SIZE );
    ci_iter->second->not_received_tag_header_done = true;
    ci_iter->second->not_received_tag_data_done = true;
}




void delete_viewer( struct ev_loop * workthread_loop, struct  ev_io *viewer_watcher,
                                   viewer_info_reference_iter vir_iter, camera_info_reference_iter cir_iter )
{
    if( workthread_loop == NULL || viewer_watcher == NULL )
    {
        log_module( LOG_ERROR, "DELETE_VIEWER", "RUNTIME ERROR OCCURRED:WORKTHREADLOOP OR VIEWER_WATCHER IS NULL");
        return;
    }
    
    log_module( LOG_DEBUG, "DELETE_VIEWER","++++++++++START++++++++++");
    if( viewer_watcher->fd > 0 )
    {
        if( loglevel_is_enabled( LOG_DEBUG ) )
        {
            std::string peer_info = get_peer_info( viewer_watcher->fd );
            log_module( LOG_DEBUG, "DELETE_VIEWER", "VIEWER %s OFFLINE", peer_info.c_str() );
        }
        
        close( viewer_watcher->fd );
        ev_io_stop( workthread_loop, viewer_watcher );
        delete viewer_watcher;
        viewer_watcher = NULL;
        
        log_module( LOG_DEBUG, "DELETE_VIEWER", "DELETE THE VIEWER'S VIEWER QUEUE++++++++++START++++++++++");
        delete_viewer_queue( vir_iter->second->viewer_queue );
        log_module( LOG_DEBUG, "DELETE_VIEWER", "DELETE THE VIEWER'S VIEWER QUEUE++++++++++DONE++++++++++");
        delete vir_iter->second;
        vir_iter->second = NULL;
        log_module( LOG_DEBUG, "DELETE_VIEWER", "ERASE VIEWER FROM VIEWER CONTAINER RELATED TO CHANNEL:%s START", cir_iter->first.c_str() );
        cir_iter->second->viewer_info_pool.erase( vir_iter );
        log_module( LOG_DEBUG, "DELETE_VIEWER", "ERASE VIEWER DONE RELATED TO CHANNEL:%s, THERE ARE(IS) REMAIN %u VIEWER(S)", 
            cir_iter->first.c_str(), cir_iter->second->viewer_info_pool.size() );
    }
    else
    {
        log_module( LOG_ERROR, "DELETE_VIEWER", "RUNTIME ERROR OCCURRED:SOCKET FD IS NOT VALID IN VIEWER WATCHER");
    }
    log_module( LOG_DEBUG, "DELETE_VIEWER","++++++++++DONE++++++++++");
}



/*
#@args:
#@desc:parse the flv stream and send the cached tags to the viwer
*/
void send_tags_cb( struct ev_loop * workthread_loop, struct  ev_io *viewer_watcher, int revents )
{
    #define STOP_SEND_TAGS() \
    close( viewer_watcher->fd );\
    ev_io_stop( workthread_loop, viewer_watcher );\
    delete viewer_watcher;\
    viewer_watcher = NULL;\
    return;
		
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR, "SEND_TAGS_CB", "EV_ERROR %s", LOG_LOCATION );
	 STOP_SEND_TAGS();
    }
    	
    int sent_bytes = -1;
    size_t total_bytes = -1;
    workthread_info_iter wi_iter = get_workthread_info_item( pthread_self() );
	
    if( wi_iter == workthread_info_pool.end() )
    {
        log_module( LOG_ERROR,"SEND_TAGS_CB","REACH THE END OF WORKTHREAD_INFO_POOL UNKNOWN ERROR OCCURRED");
	 STOP_SEND_TAGS();
    }

    viewer_info_iter vi_iter;
    camera_info_iter ci_iter;
	
    int ret = get_viewer_info_item( wi_iter, ci_iter, vi_iter , viewer_watcher->fd );
    if( ret != OK )
    {
    	log_module( LOG_ERROR, "SEND_TAGS_CB", "GET VIEWER INFO ITEM THROUGH SOCKET FD:%d FAILED", viewer_watcher->fd );
	STOP_SEND_TAGS();	
    }
	
    if(  vi_iter->second->send_first )
    {
        //send_first indicates send the flv header,avc tag,aac tag,script tag for once
        if( vi_iter->second->not_sent_http_response_done )
        {
            log_module( LOG_DEBUG,"SEND_TAGS_CB","START SEND HTTP 200 RESPONSE TO VIEWER");
            static const char *http_response="HTTP/1.1 200 OK\r\nContent-type:video/x-flv\r\n\r\n";
            static int sent_http_response_bytes = 0;
            total_bytes = strlen(http_response)-sent_http_response_bytes;
            sent_bytes = write_specify_size2( viewer_watcher->fd, http_response+sent_http_response_bytes,
                    total_bytes);
            if( sent_bytes == -1 )
            {
                delete_viewer( workthread_loop, viewer_watcher, vi_iter, ci_iter );
                return;
            }
            if( (size_t) sent_bytes != total_bytes )
            {
                sent_http_response_bytes += sent_bytes;
                return;
            }
            
            sent_http_response_bytes = 0;
            vi_iter->second->not_sent_http_response_done = false;
            log_module(LOG_DEBUG,"SEND_TAGS_CB","SEND HTTP 200 RESPONSE DONE");
        }


        if( vi_iter->second->not_sent_flv_header_done )
        {
            log_module(LOG_DEBUG,"SEND_TAGS_CB","NOW SEND THE FLV HEADER");
            total_bytes = sizeof(ci_iter->second->flv_header)-ci_iter->second->flv_header_sent_bytes;
            sent_bytes= write_specify_size(viewer_watcher->fd,
                    ci_iter->second->flv_header+ci_iter->second->flv_header_sent_bytes,
                    total_bytes);

            if( sent_bytes == -1 )
            {
                delete_viewer( workthread_loop, viewer_watcher, vi_iter, ci_iter );
                return;
            }
            
            if( (size_t)sent_bytes != total_bytes )
            {
                ci_iter->second->flv_header_sent_bytes += sent_bytes;
                return;
            }
            vi_iter->second->not_sent_flv_header_done = false;
            log_module( LOG_DEBUG,"SEND_TAGS_CB","THE FLV SIGNATURE:%c%c%c VERSION:%d",
                    ci_iter->second->flv_header[0],ci_iter->second->flv_header[1],
                    ci_iter->second->flv_header[2],ci_iter->second->flv_header[3]);
            log_module( LOG_DEBUG,"SEND_TAGS_CB","SEND THE FLV HEADER DONE");
        }


        //if there is script tag in flv file then send it to the viewer
        if( ci_iter->second->script_tag_exists )
        {
            if(  vi_iter->second->not_sent_script_tag_done )
            {
                log_module(LOG_DEBUG,"SEND_TAGS_CB","NOW SEND THE FLV SCRIPT TAG DATA");
                total_bytes = ci_iter->second->script_tag_total_bytes-ci_iter->second->script_tag_sent_bytes;
                sent_bytes = write_specify_size(viewer_watcher->fd,
                        ci_iter->second->flv_script_tag+ci_iter->second->script_tag_sent_bytes,
                        total_bytes);

                if( sent_bytes == -1 )
                {
                    delete_viewer( workthread_loop, viewer_watcher, vi_iter, ci_iter );
                    return;
                }
                
                if( (size_t)sent_bytes != total_bytes )
                {
                    ci_iter->second->script_tag_sent_bytes += sent_bytes;
                    return;
                }
                log_module(LOG_DEBUG,"SEND_TAGS_CB","SEND THE FLV SCRIPT TAG DATA DONE");
            }
            vi_iter->second->not_sent_script_tag_done = false;
        }

        //avc_sequence_header_total_bytes is not equal to 0,
        //it indiates that there is avc sequence header tag in flv file
        if( ci_iter->second->avc_sequence_header_total_bytes != 0 )
        {
            if( vi_iter->second->not_sent_avc_sequence_header_done )
            {
                log_module(LOG_DEBUG,"SEND_TAGS_CB","NOW SEND THE FLV AVC SEQUENCE HEADER");
                total_bytes = ci_iter->second->avc_sequence_header_total_bytes-ci_iter->second->avc_sequence_header_sent_bytes;
                sent_bytes = write_specify_size(viewer_watcher->fd,
                        ci_iter->second->avc_sequence_header+ci_iter->second->avc_sequence_header_sent_bytes,
                        total_bytes);
                if( sent_bytes == -1 )
                {
                    delete_viewer( workthread_loop, viewer_watcher, vi_iter, ci_iter );
                    return;
                }
                
                if( (size_t)sent_bytes != total_bytes )
                {
                    ci_iter->second->avc_sequence_header_sent_bytes += sent_bytes;
                    return;
                }
                vi_iter->second->not_sent_avc_sequence_header_done = false;
                log_module(LOG_DEBUG,"SEND_TAGS_CB","SEND THE FLV AVC SEQUENCE HEADER  DONE");
            }
        }

        //aac_sequence_header_total_bytes is not equal to 0,
        //it indiates that there is aac sequence header tag in flv file
        if( ci_iter->second->aac_sequence_header_total_bytes != 0 )
        {
            if( vi_iter->second->not_sent_aac_sequence_header_done )
            {
                log_module(LOG_DEBUG,"SEND_TAGS_CB","NOW SEND THE FLV AAC SEQUENCE HEADER");
                total_bytes = ci_iter->second->aac_sequence_header_total_bytes-ci_iter->second->aac_sequence_header_sent_bytes;
                sent_bytes = write_specify_size(viewer_watcher->fd,
                        ci_iter->second->aac_sequence_header+ci_iter->second->aac_sequence_header_sent_bytes,
                        total_bytes);

                if( sent_bytes == -1 )
                {
                    delete_viewer( workthread_loop, viewer_watcher, vi_iter, ci_iter );
                    return;
                }
                
                if( (size_t)sent_bytes != total_bytes )
                {
                    ci_iter->second->aac_sequence_header_sent_bytes += sent_bytes;
                    return;
                }
                log_module(LOG_DEBUG,"SEND_TAGS_CB","SEND THE FLV AAC SEQUENCE HEADER DONE");
                vi_iter->second->not_sent_aac_sequence_header_done = false;
            }
        }
        vi_iter->second->send_first = false;
        log_module(LOG_DEBUG,"SEND_TAGS_CB","SEND FLV SEQUENCE HEADER--DONE ID:%u",pthread_self());
    }


    //now always send the tags to viewer
    FLV_TAG_FRAME *flv_tag=NULL;
    if( viewer_queue_empty( vi_iter->second->viewer_queue ) )
    {
        log_module(LOG_DEBUG,"SEND_TAGS_CB","THE VIEWER QUEUE IS EMPTY,JUST STOP THE LIBEV IO EVENT FOR VIEWER");
        ev_io_stop(workthread_loop,viewer_watcher);
        return;
    }
    else
    {
        
        viewer_queue_top( vi_iter->second->viewer_queue, flv_tag );
        //you must send the key frame to viewer the first time
        if( vi_iter->second->send_key_frame_first && !flv_tag->is_key_frame )
        {
            log_module(LOG_DEBUG,"SEND_TAGS_CB","JUST DROP THE NON-KEYFRAME FOR THE FIRST SENT");
            viewer_queue_pop(vi_iter->second->viewer_queue);
            return;
        }
        else
        {
            log_module( LOG_DEBUG,"SEND_TAGS_CB","NOW JUST SEND THE FLV TAG FROM THE VIEWER QUEUE");
            //write_specify_size2(viewer_watcher->fd,flv_tag->TAG,flv_tag->data_size);
            if( ( flv_tag->TAG != NULL ) && ( flv_tag->data_size != flv_tag->sent_bytes ) )
            {
                /*if( (flv_tag->data_size-flv_tag->sent_bytes) >= FRAGMENT_SIZE )
                {
                    sent_bytes = write_specify_size2( viewer_watcher->fd,
                                                               flv_tag->TAG+flv_tag->sent_bytes,
                                                               FRAGMENT_SIZE);
                    DELETE_VIEWER_IF(sent_bytes,-1);
                    flv_tag->sent_bytes+=sent_bytes;
                    return;
                }*/
                /*else*/ if( (flv_tag->data_size-flv_tag->sent_bytes) > 0 )
                {
                    sent_bytes = write_specify_size( viewer_watcher->fd, 
                                                                   flv_tag->TAG+flv_tag->sent_bytes,
                                                                   flv_tag->data_size-flv_tag->sent_bytes);
                    if( sent_bytes == -1 )
                    {
                        delete_viewer( workthread_loop, viewer_watcher, vi_iter, ci_iter );
                        return;
                    }
                    
                    if( (size_t)sent_bytes != (flv_tag->data_size-flv_tag->sent_bytes) )
                    {
                        flv_tag->sent_bytes += sent_bytes;
                        return;
                    }
               }
            }
            
            flv_tag->sent_bytes = 0;
            flv_tag->data_size = 0;
            viewer_queue_pop(vi_iter->second->viewer_queue);
        }
        vi_iter->second->send_key_frame_first= false;
    }
}



/*
*args:
*returns:void
*desc:this callback was called asynchronously 
*     when main event loop send signal to workthread's event loop
*/
void async_read_cb( struct ev_loop *workthread_loop, struct ev_async *async_watcher, int revents )
{
    #define ERASE_CAMERA() \
    if( ci_iter->second->flv_script_tag != NULL )\
    delete [] ci_iter->second->flv_script_tag;\
    if( ci_iter->second->avc_sequence_header != NULL )\
    delete [] ci_iter->second->avc_sequence_header;\
    if( ci_iter->second->aac_sequence_header != NULL )\
    delete [] ci_iter->second->aac_sequence_header;\
    delete ci_iter->second;\
    ci_iter->second = NULL;\
    wi_iter->camera_info_pool.erase( ci_iter );

    LOG_START("ASYNC_READ_CB");
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"ASYNC_READ_CB","EV_ERROR %s",LOG_LOCATION);
        return;
    }

    if( workthread_loop == NULL || async_watcher == NULL )
    {
        log_module( LOG_ERROR, "ASYNC_READ_CB", "RUNTIME ERROR:ARGS NOT VALID");
        return;
    }
    
    EV_ASYNC_DATA *async_data = static_cast<EV_ASYNC_DATA *>( async_watcher->data );
    if( async_data == NULL )
    {
         log_module( LOG_ERROR, "ASYNC_READ_CB", "ASYNC DATA IS NULL");
         return;
    }
    
    if( async_data->client_type == CAMERA )
    {
        //now handle camera's request
        log_module(LOG_DEBUG,"ASYNC_READ_CB","CAMERA'S REQUEST--ID:%u",pthread_self());
        string channel( async_data->channel );
        workthread_info_iter wi_iter = get_workthread_info_item( pthread_self() );
        camera_info_iter ci_iter = get_channel_info_item( wi_iter, channel);
       
	if( async_data->client_fd > 0 )
	{    
	    int ret = rosehttp_reply_with_status( 200, async_data->client_fd );
	    if( ret == -1 )
	    {
		log_module( LOG_ERROR, "ASYNC_READ_CB","WRITE_SPECIFY_SIZE2 ERROR:FAILED TO SEND http 200 OK TO CAMERA RELATED TO CHANNEL %s", channel.c_str() );
		delete async_data;
		ERASE_CAMERA();
		return;
	    }

	    log_module( LOG_DEBUG, "ASYNC_READ_CB", "SEND 200 OK TO CAMERA RELATED TO SOCKET FD:%d", async_data->client_fd );
	    //ev_io_init( ci_iter->second->reply_watcher,reply_200OK_cb,async_data->client_fd, EV_WRITE );
	    ev_io_init( ci_iter->second->camera_watcher, receive_stream_cb, async_data->client_fd, EV_READ );
	    //ev_io_start( workthread_loop, ci_iter->second->reply_watcher );
	    ev_io_start( workthread_loop, ci_iter->second->camera_watcher );
	}
	else
	{
           log_module( LOG_ERROR, "ASYNC_READ_CB", "RUNTIME ERROR OCCURRED:SOCKET FD IS NOT VALID");
	    delete async_data;
	    ERASE_CAMERA();
	    return;
	}
    }
    else if( async_data->client_type == BACKER )
    {
        //now handle backing-resource's request
        log_module(LOG_DEBUG,"ASYNC_READ_CB","VIEWER'S BACKING RESOURCE REQUEST--ID:%u",pthread_self());
        struct ev_io *pull_stream_watcher = new struct ev_io;
        if( pull_stream_watcher == NULL )
        {
              log_module( LOG_ERROR,"ASYNC_READ_CB","ALLOCATE MEMORY FAILED:%s", strerror(errno) );
              delete [] ((static_cast<EV_ASYNC_DATA*>(async_watcher->data))->data);
              delete static_cast<EV_ASYNC_DATA*>(async_watcher->data);
              async_watcher->data = NULL;
              return;
        }

        struct ev_io *send_backing_resource_watcher = new struct ev_io;
        if( send_backing_resource_watcher == NULL )
        {
              log_module(LOG_ERROR,"ASYNC_READ_CB","ALLOCATE MEMORY FAILED:%s", strerror(errno) );
              delete pull_stream_watcher;
              pull_stream_watcher = NULL;
              delete [] ((static_cast<EV_ASYNC_DATA*>(async_watcher->data))->data);
              delete static_cast<EV_ASYNC_DATA*>(async_watcher->data);
              async_watcher->data = NULL;
              return;
        }

        struct ev_io *push_stream_watcher = new struct ev_io;
        if( push_stream_watcher == NULL )
        {
              log_module(LOG_INFO,"ASYNC_READ_CB","ALLOCATE MEMORY FAILED:%s",LOG_LOCATION);
              delete pull_stream_watcher;
              pull_stream_watcher = NULL;
              delete send_backing_resource_watcher;
              send_backing_resource_watcher = NULL;
              delete [] ((static_cast<EV_ASYNC_DATA*>(async_watcher->data))->data);
              delete static_cast<EV_ASYNC_DATA*>(async_watcher->data);
              async_watcher->data = NULL;
              return;
        }

        char *imitated_request = new char[strlen(async_data->data)+1];
        if( imitated_request == NULL )
        {
             log_module(LOG_INFO,"ASYNC_READ_CB","ALLOCATE MEMORY FAILED:%s",LOG_LOCATION);
             delete pull_stream_watcher;
             pull_stream_watcher = NULL;
             delete send_backing_resource_watcher;
             send_backing_resource_watcher = NULL;
             delete push_stream_watcher;
             push_stream_watcher = NULL;
             
             delete [] ((static_cast<EV_ASYNC_DATA*>(async_watcher->data))->data);
             delete static_cast<EV_ASYNC_DATA*>(async_watcher->data);
             async_watcher->data = NULL;
             return;
        }
        
        strcpy( imitated_request, (static_cast<EV_ASYNC_DATA*>(async_watcher->data))->data);
        delete [] ((static_cast<EV_ASYNC_DATA*>(async_watcher->data))->data);
        
        send_backing_resource_watcher->data =(void *)imitated_request;
        pull_stream_watcher->data = (void *)push_stream_watcher;
       
        ev_io_init( send_backing_resource_watcher,send_backing_resource_cb,async_data->client_fd,EV_WRITE );
        ev_io_init( pull_stream_watcher,pull_stream_cb,async_data->client_fd,EV_READ );
        ev_io_init( push_stream_watcher,push_stream_cb,async_data->request_fd,EV_WRITE );
        ev_io_start( workthread_loop, send_backing_resource_watcher );
        ev_io_start( workthread_loop, pull_stream_watcher );
            
    }
    
    delete static_cast<EV_ASYNC_DATA*>(async_watcher->data);
    async_watcher->data = NULL;
    LOG_DONE( "ASYNC_READ_CB" );
}



/*
*args:
*returns:
*desc:just send http 200 OK to camera
*/
void reply_200OK_cb(struct ev_loop * workthread_loop, struct  ev_io *reply_watcher, int revents )
{
      if( EV_ERROR & revents )
     {
          log_module(LOG_INFO,"reply_200OK_CB","EV_ERROR %s",LOG_LOCATION);
          return;
     }
     const char *http_200_ok = "HTTP/1.1 200 OK\r\n\r\n";
     write_specify_size2( reply_watcher->fd,http_200_ok, strlen(http_200_ok) );
     ev_io_stop(workthread_loop,reply_watcher);
     delete reply_watcher;
     reply_watcher = NULL;
}



/*
 *@args:
 *@returns:
 *@desc:
 */
struct STREAM_DATA
{
    uint8_t stream[TAG_DATA_SIZE];
    size_t received_bytes;
};

/*
*@args:
*@returns:void
*@desc:pull the "stream" from another streaming server
*/
void pull_stream_cb(struct ev_loop * workthread_loop, struct  ev_io *pull_stream_watcher, int revents )
{
    #define DELETE_RESOURCE(X,Y) \
    std::set<CLIENT_ID>::iterator it = (Y)->second.socketfd_pool.begin();\
    while( it != br_iter->second.socketfd_pool.end())\
    {\
         close(*it);\
         ++it;\
    }\
    (Y)->second.socketfd_pool.clear();\
    (X)->resource_info_pool.erase(Y);\
    ev_io_stop(workthread_loop,pull_stream_watcher);\
    ev_io_stop(workthread_loop,push_stream_watcher);\
    delete static_cast<struct ev_io *>(pull_stream_watcher->data);\
    pull_stream_watcher->data = NULL;\
    delete pull_stream_watcher;\
    pull_stream_watcher = NULL;\
    return;

        
    (void)workthread_loop;
    if( EV_ERROR & revents )
    {
        log_module(LOG_INFO,"PULL_STREAM_CB","EV_ERROR:%s",LOG_LOCATION);
        return;
    }
    
    struct ev_io *push_stream_watcher = static_cast<struct ev_io *>(pull_stream_watcher->data);
    if( push_stream_watcher->fd == -1 )
    {
         workthread_info_iter wi_iter = get_workthread_info_item(pthread_self());
         backing_resource_iter br_iter = get_backing_resource_item(wi_iter,pull_stream_watcher->fd);
         br_iter->second.socketfd_pool.erase(pull_stream_watcher->fd);
         close(pull_stream_watcher->fd);
         if( br_iter->second.socketfd_pool.empty() )
         {
              log_module(LOG_DEBUG,"PULL_STREAM_CB","SOCKET FD POOL WAS EMPTY--CHANNEL DELETING...");
              wi_iter->resource_info_pool.erase(br_iter);
              log_module(LOG_DEBUG,"PULL_STREAM_CB","SOCKET FD POOL WAS EMPTY--CHANNEL DELETED.");
         }
         ev_io_stop(workthread_loop,pull_stream_watcher);
         delete static_cast<struct ev_io *>(pull_stream_watcher->data);
         pull_stream_watcher->data = NULL;
         delete pull_stream_watcher;
         pull_stream_watcher = NULL;
         return;
    }
   
    STREAM_DATA * stream_data = new STREAM_DATA;
    if( stream_data == NULL )
    {
         workthread_info_iter wi_iter = get_workthread_info_item( pthread_self() );
         backing_resource_iter br_iter = get_backing_resource_item( wi_iter,pull_stream_watcher->fd );
         log_module(LOG_INFO,"PULL_STREAM_CB","ALLOCATE MEMORY FAILED:%s",LOG_LOCATION);
         DELETE_RESOURCE( wi_iter,br_iter );
    }
    stream_data->received_bytes=read_specify_size( pull_stream_watcher->fd, stream_data->stream, sizeof(stream_data->stream));
    if( stream_data->received_bytes == 0 )
    {
         workthread_info_iter wi_iter = get_workthread_info_item( pthread_self() );
         backing_resource_iter br_iter = get_backing_resource_item( wi_iter,pull_stream_watcher->fd );
         char MSG[99];
         snprintf(MSG,99,"RESOURCE %s HAS STOPED TO PUSH STREAM",br_iter->second.IP.c_str());
         log_module(LOG_INFO,"PULL_STREAM_CB",MSG);
         delete stream_data;
         stream_data = NULL;
         DELETE_RESOURCE(wi_iter,br_iter);
    }
    push_stream_watcher->data = (void *)stream_data;
    ev_io_start( workthread_loop,push_stream_watcher );
}



/*
*@args:
*@returns:void
*@desc:send the stream received from streaming server to viewer
*/
void push_stream_cb(struct ev_loop * workthread_loop, struct  ev_io *push_stream_watcher, int revents )
{
     if( EV_ERROR & revents )
    {
        log_module(LOG_INFO,"PUSH_STREAM_CB","EV_ERROR:%s",LOG_LOCATION);
        return;
    }
    static size_t total_bytes = 0;
    STREAM_DATA *stream_data = static_cast<STREAM_DATA *>(push_stream_watcher->data);
    if( total_bytes != stream_data->received_bytes )
    {
         ssize_t sent_bytes = write_specify_size(push_stream_watcher->fd,
         stream_data->stream+total_bytes,stream_data->received_bytes-total_bytes);
         if( sent_bytes == -1 )
         {
             ev_io_stop(workthread_loop,push_stream_watcher);
             workthread_info_iter wi_iter = get_workthread_info_item(pthread_self());
             backing_resource_iter br_iter = get_backing_resource_item(wi_iter,push_stream_watcher->fd);
             char MSG[99];
             snprintf(MSG,99,"VIEWER %s OFF LINE",br_iter->second.IP.c_str());
             log_module(LOG_INFO,"PUSH_STREAM_CB",MSG);
             br_iter->second.socketfd_pool.erase(push_stream_watcher->fd);
             close(push_stream_watcher->fd);
             delete static_cast<STREAM_DATA*>(push_stream_watcher->data);
             push_stream_watcher->data = NULL;
             push_stream_watcher->fd = -1;
             return;
         }
         total_bytes+=(size_t)sent_bytes;
         if( total_bytes != stream_data->received_bytes )
         return;
         
    }
    total_bytes = 0;
    delete static_cast<STREAM_DATA*>(push_stream_watcher->data);
    push_stream_watcher->data = NULL;
    ev_io_stop(workthread_loop,push_stream_watcher);
}


/*
*@args:
*@returns:void
*@desc:just send the imitated request to another streaming server
*/
void send_backing_resource_cb(struct ev_loop * workthread_loop, struct  ev_io *send_backing_watcher, int revents )
{
    if( EV_ERROR & revents )
    {
        log_module(LOG_INFO,"SEND_BACKING_RESOURCE_CB","EV_ERROR %s",LOG_LOCATION);
        return;
    }
    char * imitated_request = (char*)send_backing_watcher->data;
    log_module(LOG_DEBUG,"SEND_BACKING_RESOURCE_CB","IMITATED REQUEST:%s",imitated_request);
    static size_t total_bytes = 0;
    if( imitated_request != NULL && total_bytes != strlen(imitated_request) )
    {
        int sent_bytes = write_specify_size(send_backing_watcher->fd,
        imitated_request+total_bytes,strlen(imitated_request)-total_bytes);
        total_bytes+=sent_bytes;
        if( sent_bytes != -1 )
       {
           if( total_bytes != strlen(imitated_request))
           {
                return;
           }    
       }
       else
       {
           log_module(LOG_INFO,"SEND_BACKING_RESOURCE_CB","SEND BACKING_RESOURCE REQUEST FAILED");
           workthread_info_iter wi_iter= get_workthread_info_item(pthread_self());
           backing_resource_iter br_iter = get_backing_resource_item(wi_iter,send_backing_watcher->fd);
           br_iter->second.socketfd_pool.clear();
           wi_iter->resource_info_pool.erase(br_iter);
       }
    }
    total_bytes = 0;
    ev_io_stop(workthread_loop, send_backing_watcher);
    delete [] static_cast<char *>(send_backing_watcher->data);
    send_backing_watcher->data = NULL;
    delete send_backing_watcher;
    send_backing_watcher = NULL;
}


void read_channel_status_cb( struct ev_loop *workthread_loop, struct ev_async *async_watcher, int revents )
{
    (void) workthread_loop;
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"READ_CHANNEL_STATUS_CB","EV_ERROR %s",LOG_LOCATION);
        return;
    }

    std::map<std::string, int> notify_servers;
    //obtain the servers' address wrtten in config file
    
    get_notify_servers( NOTIFY_SERVER_CONFIG_FILE, notify_servers );
    std::map<std::string, int>::iterator it = notify_servers.begin();
    int conn_fd;

    NOTIFY_DATA * notify_data =  static_cast<NOTIFY_DATA *>( async_watcher->data );
    if( notify_data == NULL )
    {
         log_module( LOG_ERROR, "READ_CHANNEL_STATUS_CB","ERROR:NOTIFY_DATA IS NULL");
    }
    else
    {
            while( it != notify_servers.end() )
            {
                log_module( LOG_DEBUG,"READ_CHANNEL_STATUS_CB","NOTIFY TO SERVER:%s PORT:%d START",it->first.c_str(),it->second );
                conn_fd= tcp_connect( it->first.c_str(),it->second );
                if( conn_fd == -1 )
                {
                    log_module( LOG_ERROR,"READ_CHANNEL_STATUS_CB","TCP_CONNECT FAILED RELATED TO SERVER %s:%d TRY ANOTHER SERVER",
                                                        it->first.c_str(),it->second);
                    ++it;
                    continue;
                }
                
                sdk_set_tcpnodelay( conn_fd );
                sdk_set_sndbuf( conn_fd, 65535 );
                NOTIFY_DATA *notify_data_dump = new NOTIFY_DATA;
                memcpy( notify_data_dump, notify_data, sizeof( NOTIFY_DATA ) );
                struct ev_io *notify_watcher = new ev_io;
                notify_watcher->data =(void *)notify_data_dump;
                notify_watcher->fd = conn_fd;
                ev_io_init( notify_watcher , do_notify_cb, conn_fd, EV_WRITE );
                ev_io_start( workthread_loop, notify_watcher );
                log_module( LOG_DEBUG,"READ_CHANNEL_STATUS_CB","NOTIFY TO SERVER:%s PORT:%d DONE",it->first.c_str(),it->second );
                ++it;
            }
     }

     if( notify_data )
     delete static_cast<NOTIFY_DATA *>( async_watcher->data );
     async_watcher->data = NULL;
}


void get_src_info( const std::string & src, string & IP, int & PORT)
{
    std::string host;
    std::string port="";
    char sep = ':';
    std::string::const_iterator str_iter = src.begin();
    while( str_iter != src.end() && *str_iter != sep )
    {
        host.push_back(*str_iter);
        ++str_iter;
    }
    if( str_iter != src.end() )
    {
        ++str_iter;
        while( str_iter != src.end() )
        {
            port.push_back(*str_iter);
            ++str_iter;
        }
    }
    if(port.empty())
    {
        PORT=80;
    }
    else
    {
        PORT = atol(port.c_str());
    }
    IP=host;
}


void print_help( void )
{
    printf("\rUsage:streamer [OPTION]...[OPTION]...\n");
    printf("\rStartup the streaming server or list information about it:version,help info etc.\n");
    printf("\rMandatory arguments to long options are mandatory for short options too.\n");
    printf("\r%-20s%s","  -f, --config-file","specify the config file\n");
    printf("\r%-20s%s","  -h, --help","display help and exit\n");
    printf("\r%-20s%s","  -v, --version","display version and exit\n");
    printf("\r\nExample:\n");
    printf("\rstreamer  //startup this streaming server through default config file\n");
    printf("\rstreamer -h\n");
    printf("\rstreamer -v\n");
    printf("\rstreamer --config-file /etc/streamer/streamer.conf\n");
    exit( EXIT_SUCCESS );
}


void print_error( void )
{
    std::cerr<<"\r\nShort for options,you must enter at least one like the following:"<<std::endl;
    std::cerr<<"\r:streamer -h  or  ./streamer -v"<<std::endl;
    std::cerr<<"\r:streamer -f <config_file>";
    exit(EXIT_FAILURE);
}


void print_version( struct SERVER_CONFIG & server_config )
{
    printf("\rStreamer Version:%s\n", server_config.meta["version"].c_str() );
    printf("\rCopyright (C) %s.\n", server_config.meta["copyright"].c_str() );
    printf("Started on %s  with gcc 4.47 in CentOS 6.6.Written By %s.\n",
		    server_config.meta["start-date"].c_str(),
		    server_config.meta["author"].c_str() );
    exit( EXIT_SUCCESS );
}

void print_welcome( int port )
{
    char heading[1024];
    snprintf(heading,1024,"\n" \
                          "\r    *\n"\
                          "\r  *   *  *      *     *  *     *     *  *       *\n" \
                          "\r *        *     *    *    *    *    *   *       *\n"\
                          "\r   *       *    *   *      *   *   *     *      *\n"\
                          "\r      *      * * * *        * * * *       * *  *\n"\
                          "\r  *   *       *   *          *   *             *\n"\
                          "\r    *                                         *\n"\
                          "\r                                             *"\
                          "\n\nsoftware version 1.7.1, Copyleft (c) 2015 SWWY\n" \
                          "this streaming server started on March 26 2015  with gcc 4.4.7 in CentOS\n" \
                          "programed by chenzhengqiang,based on http protocol, which registered at port:%d\n\n" \
                          "now streaming server start listening :......\n\n\n\n\n", port );

    std::cout<<heading<<std::endl;
}

/*
*@args:
*@returns:
*@desc:used to notify camera's status to those server or GSLB
*/
void * notify_server_entry( void * args )
{
     
     NOTIFY_SERVER_LOOP = ev_loop_new( EVBACKEND_SELECT );
     NOTIFY_ASYNC_WATCHER = new ev_async;

     if( NOTIFY_SERVER_LOOP == NULL || NOTIFY_ASYNC_WATCHER == NULL )
     {
        log_module( LOG_ERROR,"NOTIFY_SERVER_ENTRY", "ALLOCATE MEMORY FAILED WHEN EXECUTE EV_LOOP_NEW OR NEW EV_ASYNC");
        abort();
     }

     ev_async_init( NOTIFY_ASYNC_WATCHER , read_channel_status_cb );
     ev_async_start( NOTIFY_SERVER_LOOP , NOTIFY_ASYNC_WATCHER );
     ev_run( NOTIFY_SERVER_LOOP, 0 );
     return args;
}



/*
#@desc:just providing the workthread's event loop in work thread routine
*/
void * workthread_entry ( void * args )
{
    log_module(LOG_DEBUG,"WORKTHREAD_ROUTINE START","ID:%u", pthread_self() );
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    sa.sa_flags = 0;
    if( sigemptyset(&sa.sa_mask) == -1 || sigaction( SIGPIPE, &sa, 0 ) == -1 )
    { 
        log_module( LOG_ERROR,"WORKTHREAD_ENTRY","FAILED TO IGNORE SIGPIPE SIGNAL");
        abort();
    }
    WORKTHREAD_LOOP_INFO * workthread_loop_info = static_cast<WORKTHREAD_LOOP_INFO * >(args);
    if( workthread_loop_info == NULL )
    {
        log_module( LOG_ERROR, "WORKTHREAD_ENTRY", "ARGS IS NULL RELATED TO WORK THREAD ID:%u", pthread_self() );
        return NULL;
    }
    
    //register the libev async callback
    //used to receive signal from the main event loop
    if( workthread_loop_info->async_watcher == NULL || workthread_loop_info->workthread_loop == NULL )
    {
        log_module( LOG_ERROR, "WORKTHREAD_ENTRY", "ARGS IN WORKTHREAD_LOOP_INFO INVALID RELATED TO WORKTHREAD ID:%d", pthread_self() );
        return NULL;
    }
    
    ev_async_init( workthread_loop_info->async_watcher, async_read_cb );
    ev_async_start( workthread_loop_info->workthread_loop, workthread_loop_info->async_watcher );
    ev_run( workthread_loop_info->workthread_loop, 0 );
    free( workthread_loop_info->workthread_loop );
    workthread_loop_info->workthread_loop = NULL;
    delete workthread_loop_info->async_watcher;
    workthread_loop_info->async_watcher = NULL;
    delete workthread_loop_info;
    log_module( LOG_DEBUG,"WORKTHREAD_ROUTINE DONE","ID:%u",pthread_self() );
    return NULL;
}



/*
#@args:seeing the detailed information from libev api document
#@returns:void
#@desc:this is the asynchronous callback API defined in libev;
#this function is called asynchronously when streaming server received the client's request
# for establising connection
*/
void accept_cb( struct ev_loop * main_event_loop, struct ev_io * listen_watcher, int revents )
{
    LOG_START("ACCEPT_CB");
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"ACCEPT_CB","LIBEV ERROR FOR EV_ERROR:%d--%s",EV_ERROR,LOG_LOCATION);
        return;
    }

    ssize_t client_fd;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof( struct sockaddr_in );
    client_fd = accept( listen_watcher->fd, (struct sockaddr *)&client_addr, &len );
    if( client_fd < 0 )
    {
        log_module( LOG_ERROR,"ACCEPT_CB","ACCEPT ERROR:%s",strerror(errno));   
        return;
    }
    
    struct ev_io * receive_request_watcher = new struct ev_io;
    if( receive_request_watcher == NULL )
    {
        log_module( LOG_ERROR,"ACCEPT_CB","ALLOCATE MEMORY FAILED:%s", strerror( errno ));
        rosehttp_reply_with_status( 500, client_fd );
        close( client_fd );
        return;
    }
    
    receive_request_watcher->data = NULL;   
    if( loglevel_is_enabled( LOG_INFO ))
    {
        std::string peer_info = get_peer_info( client_fd );
        log_module( LOG_INFO, "ACCEPT_CB", "CLIENT %s CONNECTED SOCK FD IS:%d",
                                                             peer_info.c_str(), client_fd );
    }
    
    sdk_set_nonblocking( client_fd );
    sdk_set_tcpnodelay( client_fd );
    sdk_set_keepalive( client_fd );

    if( loglevel_is_enabled( LOG_DEBUG ) )
    {
        uint32_t DEFAULT_RECV_BUF_SIZE;
        uint32_t DEFAULT_SEND_BUF_SIZE;
        socklen_t recv_len = sizeof( DEFAULT_RECV_BUF_SIZE );
        socklen_t send_len = sizeof( DEFAULT_SEND_BUF_SIZE );
        getsockopt( client_fd, SOL_SOCKET, SO_RCVBUF, &DEFAULT_RECV_BUF_SIZE, &recv_len );
        getsockopt( client_fd, SOL_SOCKET, SO_SNDBUF, &DEFAULT_SEND_BUF_SIZE, &send_len );
        log_module( LOG_DEBUG, "ACCEPT_CB", "DEFAULT RECV BUFFER SIZE IS :%u SEND BUFFER SIZE:%u", DEFAULT_RECV_BUF_SIZE, DEFAULT_SEND_BUF_SIZE );
    }
    
    //register the socket io callback for reading client's request    
    ev_io_init(  receive_request_watcher, receive_request_cb, client_fd, EV_READ );
    ev_io_start( main_event_loop,receive_request_watcher );
    LOG_DONE( "ACCEPT_CB" );
}



/*
#@args:
#@returns:
#@desc:dispatch workthread through simple round-robin
*/
workthread_info_iter get_workthread_through_round_robin()
{
    static int round_robin_val =0;
    workthread_info_iter wi_iter = workthread_info_pool.begin();
    int count = 0;
    while( wi_iter != workthread_info_pool.end() )
    {
        if( count == round_robin_val )
            break;
        ++count;
        ++wi_iter;
    }
    
    round_robin_val = (round_robin_val+1) % WORKTHREADS_SIZE;
    return wi_iter;
}



/*
#@args:also seeing the detailed information from the libev api document
#@returns:
#@desc:
#called asynchronously when streaming server received the data written by client
*/
void receive_request_cb( struct ev_loop * main_event_loop, struct  ev_io *receive_request_watcher, int revents )
{
    #define DO_RECEIVE_REQUEST_CB_CLEAN() \
    ev_io_stop( main_event_loop, receive_request_watcher );\
    delete receive_request_watcher;\
    receive_request_watcher = NULL;\
    return;
        
    LOG_START("RECEIVE_REQUEST_CB");
    if( EV_ERROR & revents )
    {
        log_module ( LOG_ERROR,"RECEIVE_REQUEST_CB","ERROR FOR EV_ERROR,JUST STOP THIS IO WATCHER" );
        rosehttp_reply_with_status( 500, receive_request_watcher->fd );
        close( receive_request_watcher->fd );
        DO_RECEIVE_REQUEST_CB_CLEAN();
    }

    int received_bytes;
    //used to store those key-values parsed from http request
    char request[BUFFER_SIZE];
    //read the http header and then parse it
    received_bytes = read_rosehttp_header( receive_request_watcher->fd, request, BUFFER_SIZE );
    if(  received_bytes <= 0  )
    {     
          if(  received_bytes == 0  )
          {
              if( loglevel_is_enabled( LOG_INFO ) )
              {
                    std::string peer_info = get_peer_info( receive_request_watcher->fd );
                    log_module( LOG_INFO, "RECEIVE_REQUEST_CB"," READ 0 BYTE FROM %s CLIENT DISCONNECTED ALREADY",
                                                                                           peer_info.c_str() );
              }
          }
          else if(  received_bytes == LENGTH_OVERFLOW  )
          {
              if( loglevel_is_enabled( LOG_INFO ) )
              {
                    std::string peer_info = get_peer_info( receive_request_watcher->fd );
                    log_module( LOG_INFO, "RECEIVE_REQUEST_CB","CLIENT %s'S HTTP REQUEST LINE IS TOO LONG",
                                                                                          peer_info.c_str() );
              }
          }
          
          rosehttp_reply_with_status( 400, receive_request_watcher->fd );
          close( receive_request_watcher->fd );
          DO_RECEIVE_REQUEST_CB_CLEAN();
    }
    
    request[received_bytes]='\0';
    log_module( LOG_DEBUG,"RECEIVE_REQUEST_CB","HTTP REQUEST HEADER:%s", request );

    SIMPLE_ROSEHTTP_HEADER simple_rosehttp_header;
    int ret = parse_simple_rosehttp_header( request, strlen( request ), simple_rosehttp_header );
    if( ret == STREAM_FORMAT_ERROR )
    {
        log_module( LOG_INFO, "RECEIVE_REQUEST_CB", "HTTP REQUEST LINE FORMAT ERROR");
        rosehttp_reply_with_status( 400, receive_request_watcher->fd );
        DO_RECEIVE_REQUEST_CB_CLEAN();
    }

    std::string channel = simple_rosehttp_header.url_args["channel"];
    std::string method = simple_rosehttp_header.method;
    //varify the channel and the http method
    if( channel.empty() || channel.length() > MAX_CHANNEL_SIZE 
        || (  method != "POST" &&  method != "GET" ) 
      )
      
    {
        log_module( LOG_INFO, "RECEIVE_REQUEST_CB", "BAD HTTP REQUEST:%s", request );
        rosehttp_reply_with_status( 400, receive_request_watcher->fd );
        close( receive_request_watcher->fd );
        DO_RECEIVE_REQUEST_CB_CLEAN();
    }
    
    if( method == "POST" )
    {
         //http "POST" method indicates camera's request
        //check if the channels'amounts overtop the limit before do the camera's request
        if( get_channel_list().size() > CHANNELS_LIMIT )
        {
             log_module( LOG_INFO,"RECEIVE_REQUEST_CB","CAMERA'S REQUEST:OVERTOP THE CHANNNELS' LIMIT,SEND 503 TO CAMERA");
             rosehttp_reply_with_status( 503, receive_request_watcher->fd );
             close( receive_request_watcher->fd );
        }
        else
        {
             
             log_module( LOG_INFO,"RECEIVE_REQUEST_CB","RECEIVE THE CAMERA'S REQUEST--CHANNEL:%s", channel.c_str());
             bool ok = do_camera_request( main_event_loop, receive_request_watcher, channel );
             if( ! ok )
             {
                  rosehttp_reply_with_status( 500, receive_request_watcher->fd );
                  close( receive_request_watcher->fd );
                  log_module( LOG_ERROR,"RECEIVE_REQUEST_CB","DO_CAMERA_REQUEST FAILED" );
             }
             else
             {
                  log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB", "DO_CAMERA_REQUEST SUCCEEDED");
                  log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB", "+++++SEND THE ONLINE MESSAGE TO NOTIFY WORKTHREAD RELATED TO CHANNEL %s START+++++",
                                                        channel.c_str() );
                  
                  do_channel_notify( channel, ONLINE );
                  
            }
            
        }    
    }
    else if( method == "GET" )
    {
        //http "GET" method indicates viewer's request
        log_module(LOG_INFO,"RECEIVE_REQUEST_CB","RECEIVE THE VIEWER'S REQUEST--CHANNEL:%s", channel.c_str());
        bool ok = do_viewer_request( main_event_loop, receive_request_watcher, simple_rosehttp_header );
        if( ! ok )
        {
             log_module( LOG_ERROR, "RECEIVE_REQUEST_CB","DO_VIEWER_REQUEST FAILED" );
        }
        else
        {
             log_module( LOG_INFO,"RECEIVE_REQUEST_CB","DO_VIEWER_REQUEST SUCCEEDED");
        }
    }
    
    DO_RECEIVE_REQUEST_CB_CLEAN();
}



/*
 *@args:
 *@returns:true indicates do camera's request ok,failed the otherwise
 *@desc:as the function name described,do the camera's request
 */
bool do_camera_request( struct ev_loop* main_event_loop, struct ev_io *receive_request_watcher, const std::string & channel )
{
        (void)main_event_loop;
        if( camera_already_in( channel ) )
        {
            //if the "channel" exists then just reply http 400,otherwise ok
            log_module(LOG_DEBUG,"RECEIVE_REQUEST_CB","CAMERA BAD REQUEST:CHANNEL %s ALREADY EXISTS", channel.c_str());
            rosehttp_reply_with_status( 400, receive_request_watcher->fd );
            close( receive_request_watcher->fd );
            return false;
        }

        //now choose a work thread through round robin
        workthread_info_iter wi_iter = get_workthread_through_round_robin();
        log_module( LOG_DEBUG,"DO_CAMERA_REQUEST","GET_WORKTHREAD_THROUGH_ROUND_ROBIN--OK--ID:%u",wi_iter->thread_id);
        
	 if( ev_async_pending( wi_iter->async_watcher ) == 0 )
        {
             //now allocate EV_ASYNC_DATA for async_read_cb's sake
             EV_ASYNC_DATA *async_data = new EV_ASYNC_DATA;
             if( async_data == NULL )
             {
                    log_module( LOG_INFO,"DO_CAMERA_REQUEST","ALLOCATE EV_ASYNC_DATA FAILED:%s",LOG_LOCATION );
                    return false;
             }
             
             async_data->client_type = CAMERA;
             async_data->client_fd = receive_request_watcher->fd;
             strcpy( async_data->channel, channel.c_str() );

             CAMERAS_PTR camera_ptr = new_cameras( receive_request_watcher, channel );
             if( camera_ptr == NULL )
             {
                   log_module( LOG_INFO,"DO_CAMERA_REQUEST","ALLOCATE MEMORY FAILED:CAMERAS_PTR:%s",LOG_LOCATION );
                   delete async_data;
                   async_data = NULL;
                   return false;
             }

             std::pair<std::map<CHANNEL,CAMERAS_PTR>::iterator,bool> ret = 
             wi_iter->camera_info_pool.insert( std::make_pair( channel, camera_ptr ) );
             if( ret.second )
             {
                    wi_iter->async_watcher->data=(void *)async_data;
                    ev_async_send( wi_iter->workthread_loop, wi_iter->async_watcher );
             }
             else
             {
                   log_module( LOG_ERROR, "DO_CAMERA_REQUEST", "RUNTIME ERROR OCCURRED, MAP INSERT FAILED");
                   if( camera_ptr->camera_watcher != NULL )
                   {
                        delete camera_ptr->camera_watcher;
                        camera_ptr->camera_watcher = NULL;
                   }

                   if( camera_ptr->reply_watcher != NULL )
                   {
                        delete camera_ptr->reply_watcher;
                        camera_ptr->reply_watcher = NULL;
                   }

                   if( camera_ptr->tag_data != NULL )
                   {
                        delete [] camera_ptr->tag_data;
                        camera_ptr->tag_data = NULL;
                   }

                   if( camera_ptr->flv_script_tag != NULL )
                   {
                        delete [] camera_ptr->flv_script_tag;
                        camera_ptr->flv_script_tag = NULL;
                   }

                   if( camera_ptr->aac_sequence_header != NULL )
                   {
                        delete [] camera_ptr->aac_sequence_header;
                        camera_ptr->aac_sequence_header = NULL;
                   }

                   if( camera_ptr->avc_sequence_header != NULL )
                   {
                        delete [] camera_ptr->avc_sequence_header;
                        camera_ptr->avc_sequence_header = NULL;
                   }
                   
                   delete async_data;
                   async_data = NULL;
                   return false;      
             }
        }     
        else
        {
            log_module( LOG_DEBUG,"DO_CAMERA_REQUEST","EV_ASYNC_PENDING ERROR CAMERA MIGHT HAVE TO WAIT FOR A SECOND" );
            return false;
        }
        return true;
}




/*
 *@args:
 *@returns:true indicates do viewer's request ok,failed the otherwise
 *@desc:as the function name described,do the viewer's request
 */
bool do_viewer_request( struct ev_loop * main_event_loop, struct ev_io * receive_request_watcher, 
                                                          SIMPLE_ROSEHTTP_HEADER & simple_rosehttp_header )
{
        //firstly find the channel
        workthread_info_iter wi_iter = workthread_info_pool.begin();
        camera_info_iter ci_iter;
	 std::string channel = simple_rosehttp_header.url_args["channel"];
	 if( channel.empty() )
	 {
            log_module( LOG_ERROR, "DO_VIEWER_REQUEST", "CHANNEL REQUESTED IN SIMPLE ROSEHTTP HEADER IS EMPTY");
	     return false;
	 }
     
        while( wi_iter != workthread_info_pool.end() )
        {
            ci_iter = wi_iter->camera_info_pool.find( channel );
            if( ci_iter != wi_iter->camera_info_pool.end())
                break;
            ++wi_iter;
        }

        std::string src = simple_rosehttp_header.url_args["src"];
        //wi_ter != workthread_info_pool.end() indicates that the channel requested exists
        if( wi_iter != workthread_info_pool.end() )
        {
            //firstly check the limit of viewers
            if( ci_iter->second->viewer_info_pool.size() > VIEWERS_LIMIT )
            {
                log_module( LOG_INFO,"DO_VIEWER_REQUEST","OVERTOP THE VIEWERS' LIMIT,SEND 503 TO VIEWER" );
                rosehttp_reply_with_status( 503, receive_request_watcher->fd );
                close( receive_request_watcher->fd );
                return false;
            }
            
            log_module( LOG_DEBUG,"DO_VIEWER_REQUEST","FIND THE CHANNEL:%s--ID:%u",ci_iter->first.c_str(),wi_iter->thread_id );
            viewer_info_iter vi_iter = ci_iter->second->viewer_info_pool.find( receive_request_watcher->fd );
            if( vi_iter == ci_iter->second->viewer_info_pool.end() )
            {
                VIEWER_INFO_PTR viewer_info_ptr = new_viewer_info( receive_request_watcher );
                if( viewer_info_ptr == NULL )
                {
                    log_module( LOG_ERROR, "DO_VIEWER_REQUEST","ALLOCATE MEMORY FAILED:VIEWER_INFO_PTR:%s",LOG_LOCATION );
                    return false;
                }
                
                log_module( LOG_DEBUG,"DO_VIEWER_REQUEST","ADD CLIENT_ID:%d START", receive_request_watcher->fd );
                std::pair<std::map<CLIENT_ID, VIEWER_INFO_PTR>::iterator,bool> ret = 
                ci_iter->second->viewer_info_pool.insert( std::make_pair( receive_request_watcher->fd, viewer_info_ptr ) );

                if( ! ret.second )
                {
                    log_module( LOG_ERROR, "DO_VIEWER_REQUEST", "RUNTIME ERROR OCCURRED,MAP INSERT FAILED" );
                    rosehttp_reply_with_status( 500, receive_request_watcher->fd );
                    close( receive_request_watcher->fd );
                    if( viewer_info_ptr->viewer_watcher != NULL )
                    {
                        delete viewer_info_ptr->viewer_watcher;
                        viewer_info_ptr->viewer_watcher = NULL;
                    }
                    delete viewer_info_ptr;
                    return false;
                }
                else
                {
                    log_module( LOG_DEBUG,"DO_VIEWER_REQUEST","ADD CLIENT_ID:%d DONE",receive_request_watcher->fd );
                }
            }
            else
            {
                 log_module( LOG_ERROR, "DO_VIEWER_REQUEST","SYSTEM ERROR:SYSTEM GENERATE ThE SOCKET FD TAHT WAS ALREADY IN USE");
                 rosehttp_reply_with_status( 500, receive_request_watcher->fd );
                 close( receive_request_watcher->fd );
                 return false;
            }

        }
        else if( src.empty() )  
        //if there is no channel requested in streaming server,
        //either "src" is empty then simply reply http 400 bad request
        {
                log_module( LOG_DEBUG,"DO_VIEWER_REQUEST","FIND THE CHANNEL FAILED, EITHER SRC IS EMPTY,JUST SEND 404 TO CLIENT" );
                rosehttp_reply_with_status( 404, receive_request_watcher->fd );
                close( receive_request_watcher->fd );
                return false;
        }
        else 
        //imitate the behaviour of viwer,sending the backing-source request
        {
            //if the src is not empty then do the backing-source request
            //it means imitate the request of client

            //also check if the channels' amount overtop the limit
            if( get_channel_list().size() > CHANNELS_LIMIT )
            {
                  log_module( LOG_INFO, "DO_VIEWER_REQUEST", "BACKING RESOURCE'S REQUEST:OVERTOP THE CHANNNELS' LIMIT,SEND 503 TO VIEWER" );
                  rosehttp_reply_with_status( 503, receive_request_watcher->fd  );
                  close( receive_request_watcher->fd );
                  return false;
            }
            
            log_module( LOG_DEBUG,"DO_VIEWER_REQUEST","SENDING THE BACKING-SOURCE REQUEST:SRC=%s",src.c_str() );
            //now do the backing resource request
            bool ok = do_the_backing_source_request( main_event_loop,receive_request_watcher->fd, simple_rosehttp_header );
            if( ! ok )
            {
                 log_module( LOG_ERROR, "DO_VIEWER_REQUEST", "DO THE BACKING RESOURCE REQUEST FAILED" );
                 rosehttp_reply_with_status( 500, receive_request_watcher->fd );
                 close( receive_request_watcher->fd );
                 log_module( LOG_DEBUG,"DO_VIEWER_REQUEST","SENDING THE BACKING-SOURCE REQUEST:FAILED" );
            }
            else
            {
                  log_module( LOG_DEBUG, "DO_VIEWER_REQUEST", "SENDING THE BACKING-SOURCE REQUEST:SUCCEEDED" );
            }
            return ok;
        }
        return true;
}




/*
*@srgs:conn_fd[IN],notify_data[IN]
*@returns:void
*@desc:just send the camera's status to those servers configured in notify_servers.conf 
*/
void do_notify_cb( struct ev_loop *workthread_loop, struct ev_io *notify_watcher, int revents )
{

     char notify_message[99];
     NOTIFY_DATA *notify_data = static_cast<NOTIFY_DATA *>( notify_watcher->data );
     if( notify_data == NULL )
     {
        log_module( LOG_ERROR, "DO_NOTIFY_CB","ERROR:NOTIFY_DATA IS NULL");
        goto EXIT;
     }
    
     if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR, "DO_NOTIFY_CB", "ERROR FOR LIBEV ERROR");
        goto EXIT;
    }

    
    if ( notify_data->flag != ONLINE && notify_data->flag != OFFLINE )
    {
        log_module( LOG_ERROR,"DO_NOTIFY_CB","ERROR:NOTIFY_DATA.FLAG IS %d",notify_data->flag );
        goto EXIT;
    }
   
    snprintf( notify_message,sizeof(notify_message),"POST /daemon/channel_stat.do?channel=%s"\
                                                                         "&status=%d HTTP/1.1\r\n\r\n",notify_data->channel,
                                                                         notify_data->flag);
    log_module( LOG_DEBUG, "DO_NOTIFY_CB", "NOTIFY MESSAGE:%s BYTES:%d", notify_message, strlen(notify_message) );
    write_specify_size2( notify_watcher->fd, notify_message,strlen(notify_message) );
    EXIT:
    close( notify_watcher->fd );
    delete static_cast<NOTIFY_DATA *>( notify_watcher->data );
    ev_io_stop( workthread_loop, notify_watcher );
    delete notify_watcher;
}



/*
 *@args:
 *@returns:vector<CHANEEL>
 *@desc:return our streaming server's channel list stored in vector<channel>
*/
std::vector<CHANNEL_POOL> get_channel_list()
{
    std::vector<CHANNEL_POOL> channel_pool;
    workthread_info_iter wi_iter = workthread_info_pool.begin();
    while( wi_iter != workthread_info_pool.end() )
    {
        if( !wi_iter->camera_info_pool.empty() )
        {
            camera_info_iter ci_iter = wi_iter->camera_info_pool.begin();
            while( ci_iter != wi_iter->camera_info_pool.end() )
            {
                CHANNEL_POOL channel;
                channel.channel = ci_iter->first;
                channel.is_camera = true;
                memcpy(channel.IP,ci_iter->second->IP,INET_ADDRSTRLEN);
                channel_pool.push_back(channel);
                ++ci_iter;
            }
        }

        if( !wi_iter->resource_info_pool.empty() )
        {
             backing_resource_iter br_iter = wi_iter->resource_info_pool.begin();
             while( br_iter != wi_iter->resource_info_pool.end() )
             {
                  CHANNEL_POOL channel;
                  channel.channel = br_iter->first;
                  strcpy(channel.IP,br_iter->second.IP.c_str());
                  channel.is_camera = false;
                  channel_pool.push_back(channel);
                  ++br_iter;
             }
        }
        
        ++wi_iter;
    }
    return channel_pool;
}



/*
 *@args:channel[in]
 *@returns:int[out]
 *@desc:obtain the total amount of viewers that related to the camera's channel
 */
size_t  get_channel_viewers( const string & channel )
{
    workthread_info_iter wi_iter = workthread_info_pool.begin();
    while( wi_iter != workthread_info_pool.end() )
    {
         camera_info_iter ci_iter = wi_iter->camera_info_pool.find(channel);
         if( ci_iter != wi_iter->camera_info_pool.end() )
         return ci_iter->second->viewer_info_pool.size();

         backing_resource_iter br_iter = wi_iter->resource_info_pool.find(channel);
         if( br_iter != wi_iter->resource_info_pool.end())
         return br_iter->second.socketfd_pool.size()/2;
         
         ++wi_iter;
    }
    return 0;
}



/*
 *@desc:as the function name described:obtain all viewers online
 */
size_t get_all_online_viewers()
{
    workthread_info_iter wi_iter = workthread_info_pool.begin();
    size_t total_viewers = 0;
    while( wi_iter != workthread_info_pool.end() )
    {
        if( !wi_iter->camera_info_pool.empty() )
        {
            camera_info_iter ci_iter = wi_iter->camera_info_pool.begin();
            while( ci_iter != wi_iter->camera_info_pool.end() )
            {
                total_viewers += ci_iter->second->viewer_info_pool.size();
                ++ci_iter;
            }
        }
        if( !wi_iter->resource_info_pool.empty() )
        {
            backing_resource_iter br_iter = wi_iter->resource_info_pool.begin();
            while( br_iter != wi_iter->resource_info_pool.end() )
            {
                  total_viewers += br_iter->second.socketfd_pool.size() /2;
                  ++br_iter;
            }
        }
        ++wi_iter;
    }
    return total_viewers;
}



/*
#@args: "src" stands for the address of another streaming server,
#            and the "http_request"
#@returns:
#@desc:imitate the behaviour of viewer,sending the backing-source request
#   to the streaming server that the "src" specified
*/
//suppose that the default port is 80
//src=IP:PORT

bool do_the_backing_source_request( struct ev_loop * main_event_loop,ssize_t request_fd, 
                                                                SIMPLE_ROSEHTTP_HEADER & simple_rosehttp_header )
{
    #define DELETE_ASYNC_DATA() \
    if( async_data->data != NULL )\
    {\
                delete [] static_cast<char *>(async_data->data);\
                async_data->data = NULL;\
    }\
    if( async_data != NULL )\
    {\
          delete async_data;\
          async_data = NULL;\
    }\

            
    if( main_event_loop == NULL || request_fd < 0 )
    {
        log_module( LOG_ERROR, "DO_THE_BACKING_SOURCE_REQUEST", "RUNTIME ERROR OCCURRED,EVENT LOOP OR SOCKET FD IS NOT VALID");
        return false;
    }
    
    string IP;
    int port = 80;
    
    std::string src= simple_rosehttp_header.url_args["src"];
    std::string channel = simple_rosehttp_header.url_args["channel"];
    if( src.empty() || channel.empty() )
    {
	log_module( LOG_ERROR, "DO_THE_BACKING_SOURCE_REQUEST", "SRC OR CHANNEL IN HTTP REQUEST LINE IS EMPTY");
	return false;
    }

    get_src_info( src, IP, port );
    log_module( LOG_DEBUG, "DO_THE_BACKING_SOURCE_REQUEST", "IP:%s--PORT:%d", IP.c_str(), port );
    int local_fd = tcp_connect( IP.c_str(),port );
    if( local_fd == -1 )
    {
        log_module( LOG_ERROR,"DO_THE_BACKING_SOURCE_REQUEST","TCP_CONNECT FAILED ");
        return false;
    }
    
    //now choose a work thread through round robin
    EV_ASYNC_DATA *async_data = new EV_ASYNC_DATA;
    if(  async_data == NULL )
    {
         log_module( LOG_ERROR,"DO_THE_BACKING_SOURCE_REQUEST","ALLOCATE MEMORY FAILED");
         return false;
    }

    std::ostringstream OSS_imitated_request;
    OSS_imitated_request<<"GET "<<simple_rosehttp_header.server_path<<"/"
                           <<"channel="<<channel
                           <<"&type="<<simple_rosehttp_header.url_args["type"]
                           <<"&token="<<simple_rosehttp_header.url_args["token"]<<" HTTP/1.1\r\n\r\n";
    
    string str_imitated_request = OSS_imitated_request.str();
    async_data->data = new char[str_imitated_request.length()+1];
    if( async_data->data == NULL )
    {
         delete async_data;
         log_module( LOG_ERROR,"DO_THE_BACKING_SOURCE_REQUEST","ALLOCATE MEMORY FAILED:%s", strerror(errno) );
         return false;
    }
    
    strcpy( async_data->data, str_imitated_request.c_str() );
    log_module( LOG_DEBUG,"DO_THE_BACKING_SOURCE_REQUEST","IMITATED HTTP REQUEST:%s",async_data->data);

    std::pair<std::set<CLIENT_ID>::iterator,bool> ret1;
    std::pair<std::set<CLIENT_ID>::iterator,bool> ret2;
    workthread_info_iter wi_iter = workthread_info_pool.begin();   
    if( ! resource_already_in( channel, wi_iter ))
    {
         log_module(LOG_DEBUG,"DO_THE_BACKING_SOURCE_REQUEST","RESOURCE CHANNEL:%s NOT EXIST", channel.c_str());
         wi_iter = get_workthread_through_round_robin();
         while( ev_async_pending( wi_iter->async_watcher ) != 0 )
         {
              wi_iter = get_workthread_through_round_robin();
         }
         
         RESOURCE_INFO resource_info;
         resource_info.IP=IP;
         ret1 = resource_info.socketfd_pool.insert( request_fd );
         ret2 = resource_info.socketfd_pool.insert( local_fd );
         if( ! ret1.second || ! ret2.second )
         {
            log_module( LOG_ERROR, "DO_THE_BACKING_SOURCE_REQUEST", "RUNTIME ERROR OCCURRED:SET INSERT FAILED" );
            DELETE_ASYNC_DATA();
            return false;
         }
         std::pair<std::map<CHANNEL,RESOURCE_INFO>::iterator,bool> ret = 
         wi_iter->resource_info_pool.insert( std::make_pair( channel, resource_info ) );
         if( ! ret.second )
         {
            log_module( LOG_ERROR, "DO_THE_BACKING_SOURCE_REQUEST", "RUNTIME ERROR OCCURRED:MAP INSERT FAILED" );
            DELETE_ASYNC_DATA();
            return false;
         }
    }
    else
    {
         log_module(LOG_DEBUG,"DO_THE_BACKING_SOURCE_REQUEST","RESOURCE CHANNEL:%s EXIST", channel.c_str());
         backing_resource_iter br_iter = wi_iter->resource_info_pool.find( channel );
         if( br_iter == wi_iter->resource_info_pool.end() )
         {
            log_module( LOG_ERROR, "DO_THE_BACKING_SOURCE_REQUEST", "CHANNEL %s DOESN'T EXIST", channel.c_str() );
            DELETE_ASYNC_DATA();
            return false;
         }
         
         ret1 = br_iter->second.socketfd_pool.insert( request_fd );
         ret2 = br_iter->second.socketfd_pool.insert( local_fd );
         if( ! ret1.second || ! ret2.second )
         {
            log_module( LOG_ERROR, "DO_THE_BACKING_SOURCE_REQUEST", "RUNTIME ERROR OCCURRED:SET INSERT FAILED" );
            DELETE_ASYNC_DATA();
            return false;
         }
    }

    if( ev_async_pending( wi_iter->async_watcher ) == 0 )
    {
        sdk_set_nonblocking( local_fd );
        sdk_set_keepalive( local_fd );
        sdk_set_tcpnodelay( local_fd );    
        async_data->client_fd = local_fd;
        async_data->request_fd = request_fd;
        async_data->client_type = BACKER;
        wi_iter->async_watcher->data = (void *) async_data;
        log_module( LOG_DEBUG,"DO_THE_BACKING_RESOURCE_REQUEST","EV_ASYNC_SEND START" );
        ev_async_send( wi_iter->workthread_loop, wi_iter->async_watcher );
        log_module( LOG_DEBUG,"DO_THE_BACKING_RESOURCE_REQUEST","EV_ASYNC_SEND DONE" );
    }
    else
    {
        log_module( LOG_ERROR, "DO_THE_BACKING_SOURCE_REQUEST", "EV_ASYNC_PENDING ERROR");
        DELETE_ASYNC_DATA();
        return false;
    }
    return true;
}




/*
#@args: listen_fd:the socket file descriptor for listening
#@returns:
#@desc:
providing one asynchronous event loop for handling clients's connection request
*/
void serve_forever( ssize_t streamer_listen_fd, ssize_t state_server_fd, SERVER_CONFIG & server_config )
{
    //if run_as_daemon is true,then make this streaming server run as daemon
    
    if( server_config.server["daemon"] == "yes" )
    {
        daemon(0,0);
    }
    else
    {
	    print_welcome( atoi( server_config.server["bind-port"].c_str() ) );
    }	

    struct ev_loop *main_event_loop = EV_DEFAULT;
    struct ev_io *listen_watcher = NULL;
    
    logging_init( server_config.server["log-file"].c_str(), atoi(server_config.server["log-level"].c_str()));
    log_module( LOG_DEBUG, "SERVE_FOREVER",  "LISTEN SOCKET FD:%d", streamer_listen_fd );
    
    //you have to ignore the PIPE's signal when client close the socket
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    sa.sa_flags = 0;
    if( sigemptyset( &sa.sa_mask ) == -1 ||sigaction( SIGPIPE, &sa, 0 ) == -1 )
    { 
        log_module(LOG_ERROR,"SERVE_FOREVER","FAILED TO IGNORE SIGPIPE SIGNAL");
        goto STREAMER_EXIT;
    }

    //initialization on main event-loop
    listen_watcher= new ev_io;
    if( listen_watcher == NULL )
    {
        log_module( LOG_ERROR,"SERVE_FOREVER","ALLOCATE MEMORY FAILED:%s WHEN EXECUTE NEW EV_IO",LOG_LOCATION);
        goto STREAMER_EXIT;
    }
    else
    {
        if( server_config.server["notify-server-file"].empty() || server_config.server["notify-server-file"].length() > 50 )
        {
            log_module( LOG_ERROR, "SERVE_FOREVER","NOTIFY SERVER CONFIG FILE IS EMPTY OR THE NAME IS TOO LONG" );
            goto STREAMER_EXIT;
        }
        
        strncpy( NOTIFY_SERVER_CONFIG_FILE,server_config.server["notify-server-file"].c_str(),sizeof( NOTIFY_SERVER_CONFIG_FILE ) );
        //and now initialize the workthread pool.pool size is specified with global "WORKTHREADS_SIZE"
        if( ! init_workthread_info_pool( WORKTHREADS_SIZE ) )
        {
            log_module( LOG_ERROR,"SERVE_FOREVER","INIT_WORKTHREAD_INFO_POOL FAILED");
            goto STREAMER_EXIT;
        }
        
        
        //meanwhile startup the state server
        bool state_ok = startup_state_server( state_server_fd );
        if( state_ok )
        {
             ev_io_init( listen_watcher, accept_cb, streamer_listen_fd, EV_READ );
             ev_io_start( main_event_loop, listen_watcher );
             
            if( loglevel_is_enabled( LOG_DEBUG ) )
            {
                uint32_t DEFAULT_RECV_BUF_SIZE;
                uint32_t DEFAULT_SEND_BUF_SIZE;
                socklen_t recv_len = sizeof( DEFAULT_RECV_BUF_SIZE );
                socklen_t send_len = sizeof( DEFAULT_SEND_BUF_SIZE );
                getsockopt( streamer_listen_fd , SOL_SOCKET, SO_RCVBUF, &DEFAULT_RECV_BUF_SIZE, &recv_len );
                getsockopt( streamer_listen_fd , SOL_SOCKET, SO_SNDBUF, &DEFAULT_SEND_BUF_SIZE, &send_len );
                log_module( LOG_DEBUG, "SERVE_FOREVER", "DEFAULT RECV BUFFER SIZE IS :%u SEND BUFFER SIZE:%u", DEFAULT_RECV_BUF_SIZE, DEFAULT_SEND_BUF_SIZE );
            }
            ev_run( main_event_loop,0 );
        }
        else
        {
             log_module( LOG_INFO,"SERVE_FOREVER","STARTUP_STATE_SERVER:FAILED" );
        }
    }

    STREAMER_EXIT:
    close( streamer_listen_fd );
    close( state_server_fd );
    logging_deinit();

    if( main_event_loop )
    {
        free(main_event_loop);
        main_event_loop = NULL;
    }

    if( listen_watcher )
    {
        delete listen_watcher;
        listen_watcher = NULL;
    }
    
    free_workthread_info_pool();
    LOG_DONE( "SERVE_FOREVER" );
}
