--author:chenzhengqiang
--start date:2015/7/23

error_quit_cjson= require "cjson"
local reply={}
reply.code=0
reply.message="SUCCESS"

local _error_quit={ _version="1.7.1"}

ERROR_CODE={}
ERROR_CODE.ERR_HTTP_HEADER=10001
ERROR_CODE.ERR_URL_PARA=10002
ERROR_CODE.ERR_BODY_JSON=10003
ERROR_CODE.ERR_JSON_PARA=10004
ERROR_CODE.ERR_NODATA=10005
ERROR_CODE.ERR_SERVER=10006
ERROR_CODE.ERR_ACCOUNT_PASSWD=10007
ERROR_CODE.ERR_INVALID_REQ=10008
ERROR_CODE.ERR_NO_HANDLE_REQ=10009
ERROR_CODE.ERR_INVALID_TOKEN=10010
ERROR_CODE.ERR_CONTENT_TYPE=10011
ERROR_CODE.ERR_INVALID_DESMSGSRV_IP=10012
ERROR_CODE.ERR_SERVER_TOOBUSY=10013
ERROR_CODE.ERR_RDS_UNAVAILABLE=10014
ERROR_CODE.ERR_LOG_ERR=10015
ERROR_CODE.ERR_REQUEST_METHOD=10016

ERROR_DESC={}
ERROR_DESC.ERR_HTTP_HEADER = "Http header wrong"
ERROR_DESC.ERR_URL_PARA = "URL parameters wrong"
ERROR_DESC.ERR_BODY_JSON = "Wrong json format in http request body"
ERROR_DESC.ERR_JSON_PARA = "Lack or wrong body parameters"
ERROR_DESC.ERR_NODATA = "No data"
ERROR_DESC.ERR_SERVER = "Server system wrong"
ERROR_DESC.ERR_ACCOUNT_PASSWD = "No user or password wrong"
ERROR_DESC.ERR_INVALID_REQ = "Invalid request"
ERROR_DESC.ERR_NO_HANDLE_REQ = "No handle for request"
ERROR_DESC.ERR_INVALID_TOKEN = "Invalid token"
ERROR_DESC.ERR_CONTENT_TYPE = "Invalid content type"
ERROR_DESC.ERR_INVALID_DESMSGSRV_IP = "Invalid destination msg server ip"
ERROR_DESC.ERR_SERVER_TOOBUSY = "The server is too busy"
ERROR_DESC.ERR_RDS_UNAVAILABLE = "The redis server is unavailable"
ERROR_DESC.ERR_LOG_ERR = "The redis server is unavailable"
ERROR_DESC.ERR_REQ_METHOD = "request method error"


function _error_quit.for_http_header()
    reply.code=ERROR_CODE.ERR_HTTP_HEADER
    reply.message=ERROR_DESC.ERR_HTTP_HEADER
    ngx.say(error_quit_cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)     
end

function _error_quit.for_content_type()
    reply.code=ERROR_CODE.ERR_CONTENT_TYPE
    reply.message=ERROR_DESC.ERR_CONTENT_TYPE
    ngx.say(error_quit_cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)     
end

function _error_quit.for_url_parameters()
    reply.code=ERROR_CODE.ERR_URL_PARA
    reply.message=ERROR_DESC.ERR_URL_PARA
    ngx.say(error_quit_cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)     
end

function _error_quit.for_json_format()
    reply.code=ERROR_CODE.ERR_BODY_JSON
    reply.message=ERROR_DESC.ERR_BODY_JSON
    ngx.say(error_quit_cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)     
end

function _error_quit.for_json_para()
    reply.code=ERROR_CODE.ERR_JSON_PARA
    reply.message=ERROR_DESC.ERR_JSON_PARA
    ngx.say(error_quit_cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)     
end

function _error_quit.for_empty_data()
    reply.code=ERROR_CODE.ERR_NODATA
    reply.message=ERROR_DESC.ERR_NODATA
    ngx.say(error_quit_cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)     
end

function _error_quit.for_server()
    reply.code=ERROR_CODE.ERR_SERVER
    reply.message=ERROR_DESC.ERR_SERVER
    ngx.say(error_quit_cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)     
end

function _error_quit.for_request_method()
    reply.code=ERROR_CODE.ERR_REQUEST_METHOD
    reply.message=ERROR_DESC.ERR_REQ_METHOD
    ngx.say(error_quit_cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)     
end
return _error_quit
