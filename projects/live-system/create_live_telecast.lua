--author:chenzhengqiang
--start date:2015/7/21
--modified date:
--company:swwy
--desc:providing the http interface for creating live telecast
--detailed information just seeing the api document


local swwy_mysql= require "swwy_mysql"
local error_quit= require "error_quit"
local util=require "util"
local cjson=require "cjson"

--firstly check if the request method and content type is valid
local request_method="POST"
local content_type="application/json;charset=UTF-8"
util.do_check_method_and_content_type(request_method,content_type)

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

--now get them one by one
local uid=jsontable.uid
local name=jsontable.name
local title=jsontable.title
local introduce=jsontable.introduce
local live_name=jsontable.live_name
local live_desc=jsontable.live_desc
local start_time=jsontable.start_time
local category=jsontable.category
local department=jsontable.department
local public=jsontable.public
local invitees=jsontable.invitees

if (not (uid and name and title and introduce and live_name and live_desc and start_time and
    category and department and public)) then
    error_quit.for_json_para()
end

local SQL=[[ INSERT INTO swwy_live_telecast(uid,name,title,introduce,live_name,live_desc,
                                            start_time,category,department,public) 
            VALUES (%d,'%s','%s','%s','%s','%s',%d,'%s','%s',%d)
          ]]
SQL=string.format(SQL,uid,name,title,introduce,live_name,live_desc,
                  start_time,category,department,public)

local mysql_handler = swwy_mysql.get_mysql_handler()
if not mysql_handler then
    error_quit.for_server()
end

local results = swwy_mysql.exec_mysql_query(mysql_handler,SQL)
if not results then
    swwy_mysql.close_mysql_handler(mysql_handler)
    error_quit.for_json_para()
end

--get the insert id for insert invitees into mysql table swwy_telecast_invitees 
SQL="SELECT LAST_INSERT_ID() as ID"
results=swwy_mysql.exec_mysql_query(mysql_handler,SQL)
local INSERT_ID=results[1].ID
swwy_mysql.close_mysql_handler(mysql_handler)

if (public==0) and invitees then
    local rds = util.connect_redis()
    if not rds then
        error_quit.for_server()
    end

    local hashkey = string.format('prilive_invitees_%d', INSERT_ID)
    for index,invitee_id in pairs(invitees) do
        rds:hmset(hashkey, invitee_id, string.format('V:1:%10d:255.255.255.255', invitee_id))
    end

    util.close_redis(rds)
end

--done

local reply={}
reply.code=0
reply.message="SUCCESS"
reply.content={}
reply.content.live_ID=tonumber(INSERT_ID)
reply.content.live_name=live_name
reply.content.live_desc=live_desc
reply.content.start_time=start_time
reply.content.category=category
reply.content.department=department

ngx.say(cjson.encode(reply))
ngx.exit(ngx.HTTP_OK)
--done 
