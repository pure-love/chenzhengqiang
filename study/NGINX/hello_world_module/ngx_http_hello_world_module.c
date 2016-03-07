#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct 
{
    ngx_str_t output_words;
} ngx_http_hello_world_loc_conf_t;


// To process HelloWorld command arguments
static char* ngx_http_hello_world(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

// Allocate memory for HelloWorld command
static void* ngx_http_hello_world_create_loc_conf(ngx_conf_t* cf);

// Copy HelloWorld argument to another place
static char* ngx_http_hello_world_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);

// Structure for the HelloWorld command
static ngx_command_t ngx_http_hello_world_commands[] = {
    {
        ngx_string("hello_world"), // The command name
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,//the command type
        ngx_http_hello_world, // The command handler
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_hello_world_loc_conf_t, output_words),
        NULL
    },
    ngx_null_command
};


// Structure for the HelloWorld context
static ngx_http_module_t ngx_http_hello_world_module_ctx = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_hello_world_create_loc_conf,
    ngx_http_hello_world_merge_loc_conf
};


// Structure for the HelloWorld module, the most important thing
ngx_module_t ngx_http_hello_world_module = 
{
    NGX_MODULE_V1,
    &ngx_http_hello_world_module_ctx,
    ngx_http_hello_world_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};


static ngx_int_t ngx_http_hello_world_handler(ngx_http_request_t* r) 
{
  	//the http method must be GET OR HEAD,return 405 otherwise
  	if ( ! (r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)) )
  	{
		return NGX_HTTP_NOT_ALLOWED;
  	}

	//discard the http's request package
	ngx_int_t rc = ngx_http_discard_request_body(r);

	if ( rc != NGX_OK )
	{
		return rc;
	}
	
	//then set the Content-Type
	/*ngx_str_t type = ngx_string("text/html");
	//you can use the macro ngx_string for convinience's sake
	ngx_str_t response = ngx_string("Hello World");
	//set the http status's code
	r->headers_out.status = NGX_HTTP_OK;
	//set the content length
	r->headers_out.content_length_n = response.len;
	r->headers_out.content_type = type;

	//send the header first
	rc = ngx_http_send_header(r);
	if ( rc == NGX_ERROR || rc > NGX_OK || r->header_only )
	{
		return rc;
	}*/

	/*ngx_buf_t *b;
	b = ngx_create_temp_buf(r->pool, response.len);
	if ( b == NULL )
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	ngx_memcpy(b->pos, response.data, response.len);
	//notice:you must set the "last" pointer
	b->last = b->pos + response.len;
	//and set the last buf flag
	b->last_buf = 1;

	*/

	//the last important step calling the ngx_http_output_filter
	//to filish the request

	//send the disk file
	ngx_buf_t *b;
	b = ngx_palloc(r->pool, sizeof(ngx_buf_t));
	u_char *filename = (u_char*)"/tmp/test.html";
	b->in_file = 1;
	b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
	b->file->fd = ngx_open_file(filename, NGX_FILE_RDONLY | NGX_FILE_NONBLOCK,
									 NGX_FILE_OPEN, 0 );
	b->file->log = r->connection->log;
	b->file->name.data = filename;
	b->file->name.len = sizeof(filename)-1;

	if ( b->file->fd <= 0 )
	{
		return NGX_HTTP_NOT_FOUND;
	}

	if ( ngx_file_info(filename, &b->file->info) == NGX_FILE_ERROR )
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	//then set the Content-Type
	ngx_str_t type = ngx_string("text/html");
	r->headers_out.content_length_n = b->file->info.st_size;
	r->headers_out.status = NGX_HTTP_OK;
	//set the content length
	r->headers_out.content_type = type;
	//send the header first
	rc = ngx_http_send_header(r);
	if ( rc == NGX_ERROR || rc > NGX_OK || r->header_only )
	{
		return rc;
	}
	
	b->file_pos = 0;
	b->file_last = b->file->info.st_size;
	b->last = b->pos + b->file->info.st_size;
	//and set the last buf flag
	b->last_buf = 1;
	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;
	
	return ngx_http_output_filter(r, &out);
}

static void* ngx_http_hello_world_create_loc_conf(ngx_conf_t* cf) 
{
    ngx_http_hello_world_loc_conf_t* conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_hello_world_loc_conf_t));
    if (conf == NULL)
    {
        return NGX_CONF_ERROR;
    }
    conf->output_words.len = 0;
    conf->output_words.data = NULL;

    return conf;
}

static char* ngx_http_hello_world_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child) 
{
    ngx_http_hello_world_loc_conf_t* prev = parent;
    ngx_http_hello_world_loc_conf_t* conf = child;
    ngx_conf_merge_str_value(conf->output_words, prev->output_words, "Nginx");
    return NGX_CONF_OK;
}

static char* ngx_http_hello_world(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) 
{
    ngx_http_core_loc_conf_t* clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_hello_world_handler;
    ngx_conf_set_str_slot(cf, cmd, conf);
    return NGX_CONF_OK;
}
