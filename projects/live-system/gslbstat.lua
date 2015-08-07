--author:laihuining
--start date:

local error_quit = require "error_quit"
local cjson = require "cjson"
local util = require "util"
local GSLB_KEY="gslbstat"

--firstly check if the request method and content type is valid
local request_method="POST"
local content_type="application/json;charset=UTF-8"
util.do_check_method_and_content_type(request_method,content_type)

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

--get parameters
local version = jsontable.version
local areaID = jsontable.areaID
local concurrency = jsontable.concurrency
local uptime = jsontable.uptime
local machineroom = jsontable.machineroom
local streamips = jsontable.streamips

if not version or not areaID or not concurrency or not uptime or not machineroom or not streamips then
    error_quit.for_json_format()
end
 
-- update redis --
local rds = util.connect_redis()
if type(rds) == "userdata" then
    error_quit.for_server()
end

local gslbIP = ngx.var.remote_addr
rds:hset(GSLB_KEY,gslbIP,body)
local reply={}
reply.code=0
reply.message="SUCCESS"
ngx.say(cjson.encode(reply))
ngx.exit(ngx.HTTP_OK)
