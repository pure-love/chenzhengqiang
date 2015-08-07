--author:chenzhengqiang
--start date:2015/7/28
--modified date:
--desc:providing the http interface for obtaining the specified streamer's stat info

local cjson = require "cjson"
local util = require "util"
local error_quit= require "error_quit"
local redis_gslb_key="gslbstat"
local redis = util.connect_redis()

if not redis then
    error_quit.for_server()
end

local reply='{"message":"SUCCESS","code":0,"content":['
local IP=ngx.var.arg_ip

if not IP or string.len(IP) == 0 then
    local results=redis:hkeys(redis_gslb_key)
    if not results then
        error_quit.for_server()
    end
    for index,key in pairs(results) do
       local result=redis:hget(redis_gslb_key,key)
       reply=reply..result..","
    end
else
   local result=redis:hget(redis_gslb_key,IP)
   if type(result)=="userdata" then
       error_quit.for_url_parameters()
   end
   reply=reply..result..","    
end

reply=string.sub(reply,1,-2)
reply=reply..']}'
ngx.say(reply)
ngx.exit(ngx.HTTP_OK)
