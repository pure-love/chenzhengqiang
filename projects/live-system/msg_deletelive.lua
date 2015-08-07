
-- It provide access to mysql for message server

ngx.header.content_type = "text/plain";

local configs = require "config"
local error_quit = require "error_quit"
local os = require "os"
local util = require "util"
local str = require "resty.string"


-- 验证通过，生成并返回json数据
function print_resp(cont)
    local res_json = string.format('{ "memlist":[', fd, uid)
    local cnt = 0
    if (cont ~= nil and cont ~= ngx.null) then
        for i,v in pairs(cont) do
            if type(v) == "table" then
                if cnt >= 1 then
                    res_json = res_json .. ', '
                end

                res_json = res_json .. tostring(v["memberID"])
                cnt = cnt+1
            end
        end
    end

    res_json = res_json .. ' ] }'

    ngx.say(res_json)
end

-- 查找组成员ID
function execute_sql_query(db,sql)
    local res, err, errno, sqlstate = db:query(sql)
    if not res then
        util.close_sql(db)
        error_quit.for_server()
        return
    end

    local content = res
    if (content == nil or table.getn(content)== 0) then
        util.close_sql(db)
        ngx.say('{ "memlist":[ ] }')
        ngx.exit(ngx.HTTP_OK)
        return
    end

    return content
end


function msg_deletelive()
    local con_type = ngx.var.content_type

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

    local grpID = jsontable.grpID
    if not grpID then
        error_quit.for_json_para()
    end

    local sql = string.format('SELECT status FROM swwy_live_telecast WHERE ID=%d', grpID)

    -- 执行SQL语句
    local db = util.connect_sql();
    local cont = execute_sql_query(db,sql)
    cont = cont[1]
    if cont['status'] == 3 then
        local res_json = '{ "memlist":[] }'
        ngx.say(res_json)
    else
        sql = string.format('SELECT memberID FROM swwy_telecast_invitees WHERE grpID=%d', grpID)
        cont = execute_sql_query(db, sql)
        print_resp(cont)
    end

    -- 清除直播数据
    sql = string.format('DELETE FROM swwy_live_telecast WHERE ID=%d', grpID)
    db:query(sql)
    sql = string.format('DELETE FROM swwy_telecast_invitees WHERE grpID=%d', grpID)
    db:query(sql)

    util.close_sql(db)

    ngx.exit(ngx.HTTP_OK)
end


msg_deletelive()
