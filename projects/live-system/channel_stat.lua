--author:chenzhengqiang
--start date:2015/7/22
--modifed date:
--desc:

local swwy_mysql = require "swwy_mysql"
local error_quit = require "error_quit"
local cjson = require "cjson"

local IS_CHANNEL_OVER=2
local IS_CHANNEL_FAILURE=3
local ONLINE=0
local OFFLINE=1

local channel=ngx.var.arg_channel
local status=ngx.var.arg_status

if not (channel and status) then
    error_quit.for_url_parameters()
else
    status = tonumber(status)
end

local mysql_handler=swwy_mysql.get_mysql_handler()
if not mysql_handler then
    error_quit.for_server()
end

local SQL = string.format("SELECT status FROM swwy_live_telecast WHERE channel='%s'", channel)
local results = swwy_mysql.exec_mysql_query(mysql_handler,SQL)

if not results then
    error_quit.for_server()
end

if next(results) == nil then
    error_quit.for_url_parameters()
end

local channel_status=results[1].status

if channel_status  ~= IS_CHANNEL_OVER then
    if status == ONLINE then
        SQL = string.format("UPDATE swwy_live_telecast SET status=1 WHERE channel='%s'",channel)
    elseif status == OFFLINE then
        if channel_status == IS_CHANNEL_FAILURE then
            ngx.exit(ngx.HTTP_OK)
        end
        SQL = string.format("UPDATE swwy_live_telecast SET status=3 WHERE channel='%s'",channel)
    end
end

results = swwy_mysql.exec_mysql_query(mysql_handler,SQL)
if not results or next(results) == nil then
    error_quit.for_server()
end

swwy_mysql.close_mysql_handler(mysql_handler)

--流服接口,不需要返回
ngx.exit(ngx.HTTP_OK)
