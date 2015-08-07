--author:chenzhengqiang
--start date:2015/7/22
--modified date:
--desc

local swwy_mysql = require "swwy_mysql"
local cjson = require "cjson"
local error_quit= require "error_quit"
local types={0,1,2,3,4}
local reply={}
reply.code=0
reply.content={}
reply.message="SUCCESS"

local type=ngx.var.arg_type
local version=ngx.var.arg_version

if not type or not version then
    error_quit.for_url_parameters()
end

local right_type = false
type=tonumber(type)
for i,v in pairs(types) do
    if v == type then
        right_type=true
        break
    end
end

if not right_type or string.len(version) == 0 then
    error_quit.for_url_parameters()
end


local mysql_handler=swwy_mysql.get_mysql_handler()
if not mysql_handler then
    error_quit.for_server()
end

local SQL=string.format("select upgrade_addr,force_update,version from swwy_modules_info where type=%d",type)
local results=swwy_mysql.exec_mysql_query(mysql_handler,SQL)

if not results then
    error_quit.for_server()
end

if next(results) == nil then
    error_quit.for_url_parameters()
end

reply.content.upgrade_addr=results[1].upgrade_addr
reply.content.version=results[1].version
reply.content.force_update=results[1].force_update

reply=cjson.encode(reply)
ngx.say(reply)
ngx.exit(ngx.HTTP_OK)
