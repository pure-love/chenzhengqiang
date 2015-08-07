--author:chenzhengqiang
--start date:2015/7/20
--modified date:
--desc:providing the http interface for obtaining the streamer's information

local swwy_mysql=require("swwy_mysql")
local configs = require "config"
local cjson = require("cjson")
local error_quit = require("error_quit")

local reply = {}
if ngx.var.request_method ~= "GET" then
    error_quit.for_request_method()
end

local areaID = ngx.var.arg_area_ID
if areaID then
    areaID = tonumber(areaID)
end

local file,err = io.open("/usr/local/openresty/nginx/conf/daemon/streamsrv.conf", "r")

if not file then
    error_quit.for_server()
end

local content = {}

-- read line
local json = file:read("*a")
local jsontable = cjson.decode(json)

local streamsrvlist = jsontable.streamsrvlist
for i,v in pairs(streamsrvlist) do
    if not areaID or (areaID and areaID==v.areaID) then
        content[i] = {}
        content[i].IP = v.IP
        content[i].ISP = v.ISP
        content[i].region = v.region
        content[i].concurrency = v.concurrency
        content[i].bandwidth = v.bandwidth
    end
end

ngx.say(string.format('{ "code":0, "message":"SUCCESS", "content":%s }', cjson.encode(content)))
ngx.exit(ngx.HTTP_OK)
