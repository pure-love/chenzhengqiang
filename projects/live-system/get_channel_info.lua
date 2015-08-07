--author:chenzhengqiang
--start date:2015/7/22
--modified date:
--company:swwy
--desc:providing the HTTP interface for obtaining those live telecast info

local cjson = require "cjson"
local swwy_mysql = require "swwy_mysql"
local error_quit = require "error_quit"
local request_method="GET"
local content_type="application/json;charset=UTF-8"
local util=require "util"
 
--firstly check if the request method and content type is valid
--util.do_check_method_and_content_type(request_method,content_type)
--done

local uid = ngx.var.arg_uid
if uid then
    uid = tonumber(uid)
end

local invitee = ngx.var.arg_invitee
if invitee then
    invitee = tonumber(invitee)
end


--read body
ngx.req.read_body()
local request_body = ngx.req.get_body_data()
if not request_body then
    error_quit.for_json_format()
end

--get the input value and check it one by one
local jsonout = {}
if not pcall(util.decode_json, request_body, jsonout) then
    error_quit.for_json_format()
end

local jsontable = jsonout[0]
if not jsontable then
    error_quit.for_json_format()
end

index=jsontable.index
count=jsontable.count
sort_type=jsontable.sort_type
status=jsontable.status
category=jsontable.category
department=jsontable.department
if not (index and count and sort_type) then
    error_quit.for_json_para()
end  
--done

local mysql_handler = swwy_mysql.get_mysql_handler()
if not mysql_handler then
    error_quit.for_server()
end

-- if user get his invited live list
if (invitee == 1) and uid then
    local rds = util.connect_redis()
    if not rds then
        error_quit.for_server()
    end

    local invitedlives = rds:smembers(string.format('invitedlive_%d', uid))
    local SQL = [[ SELECT ID , live_name, live_desc, name, title, uid, start_time, end_time, category, department,
                    hot, praises, hits from swwy_live_telecast WHERE ]]
    if #invitedlives==0 then
        error_quit.for_empty_data()
    end

    for i=1,#invitedlives,1 do
        local live_ID = tonumber(invitedlives[i])
        SQL = SQL .. string.format('ID=%d or ', live_ID)
    end
    SQL = string.sub(SQL, 1, -4)

    local channels=swwy_mysql.exec_mysql_query(mysql_handler,SQL)
    if not channels then
        error_quit.for_json_format()
    end

    if next(channels)==nil then
        error_quit.for_empty_data()
    end

    local reply={}
    reply.code=0
    reply.message="SUCCESS"
    reply.content = channels
    ngx.say(cjson.encode(reply))
    ngx.exit(ngx.HTTP_OK)
end

---now do the sql
local SQL = [[SELECT ID as live_ID,live_name,live_desc,name,title,uid,start_time,end_time,category,department,
               hot,praises,hits from swwy_live_telecast ]]
local SORT_FIELD = "hot" --默认按热度排序
if sort_type == 1 then --点击率
    SORT_FIELD="hits"
elseif sort_type == 2 then --热度
    SORT_FIELD="hot"
elseif sort_type == 3 then --创建时间
    SORT_FIELD="create_time"
end

-- compose sql condition
local CONDITION = ''
if uid or status or category or department then
    CONDITION = CONDITION .. [[ WHERE]]
end

if uid then
    CONDITION = CONDITION .. string.format([[ uid=%d and]], uid)
end

if status then 
    CONDITION = CONDITION .. string.format([[ status=%d and]], status)
end

if category then
    CONDITION = CONDITION .. string.format([[ category='%s' and]], category)
end

if department then
    CONDITION = CONDITION .. string.format([[ department='%s' and]], department)
end

CONDITION = string.sub(CONDITION, 1, -4)
CONDITION = CONDITION .. string.format([[ ORDER BY %s limit %d,%d]], SORT_FIELD, index, count)
SQL = SQL .. CONDITION

-- query sql
local channels=swwy_mysql.exec_mysql_query(mysql_handler,SQL)
if not channels then
    error_quit.for_json_format()
end

if next(channels)==nil then
    error_quit.for_empty_data()
end

swwy_mysql.close_mysql_handler(mysql_handler)
local reply={}
reply.code=0
reply.message="SUCCESS"
reply.content = channels
ngx.say(cjson.encode(reply))
ngx.exit(ngx.HTTP_OK)
