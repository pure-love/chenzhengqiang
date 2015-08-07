--author:chenzhengqiang
--start date:2015/7/22
--modified date:
--desc:


local swwy_mysql = require "swwy_mysql"
local cjson = require "cjson"
local util = require "util"

--firstly check if the request method and content type is valid
util.do_check_method_and_content_type(request_method,content_type)
--done

--then just get the http request body for inserting them into database
ngx.req.read_body()
local body = ngx.req.get_body_data()
if not body then
    error_quit.for_json_format()
end

local jsonout = {}
if not pcall(util.decode_json, body, jsonout) then
    error_quit.for_json_format()
end

local jsontable = jsonout[0]
if not jsontable then
    error_quit.for_json_format()
end

local liveID = jsontable.liveID
if not liveID then
    error_quit.for_json_para()
end

-- parameter list
local introduce = jsontable.introduce
local live_desc = jsontable.live_desc
local category = jsontable.category
local department = jsontable.department
local public = jsontable.public
local channel = jsontable.channel
local status = jsontable.status

local SQL = 'UPDATE swwy_live_telecast SET '
if introduce then
    SQL = SQL .. string.format([[introduce='%s',]], introduce)
end
if live_desc then
    SQL = SQL .. string.format([[live_desc='%s',]], live_desc)
end
if category then
    SQL = SQL .. string.format([[category=%d,]], category)
end
if department then
    SQL = SQL .. string.format([[department=%d,]], department)
end
if public then
    SQL = SQL .. string.format([[public=%d,]], public)
end
if channel then
    SQL = SQL .. string.format([[channel='%s',]], channel)
end
if status then
    SQL = SQL .. string.format([[status=%d,]], status)
end
SQL = string.sub(SQL, 1, -2)
SQL = SQL .. string.format(' WHERE ID=%d', liveID)

local mysql_handler = swwy_mysql.get_mysql_handler()
if not mysql_handler then
    error_quit.for_server()
end

local results = swwy_mysql.exec_mysql_query(mysql_handler,SQL)
if not results then
    swwy_mysql.close_mysql_handler()
    error_quit.for_server()
end

if next(results) == nil then
   reply.code=98
   reply.message="REUQEST VALUE ERROR"
   reply=cjson.encode(reply)
   ngx.say(reply)
   ngx.exit(ngx.HTTP_OK)
end

SQL=string.format("update swwy_live_telecast set status=2 where channel='%s'",channel)
results=swwy_mysql.exec_mysql_query(mysql_handler,SQL)

if next(results) == nil then
    reply.code=99
    reply.message="SERVER ERROR"
    reply=cjson.encode(reply)
    ngx.say(reply)
    ngx.exit(ngx.HTTP_INTERNAL_SERVER_ERROR)
end

reply=cjson.encode(reply)
ngx.say(reply)
ngx.exit(ngx.HTTP_OK)
