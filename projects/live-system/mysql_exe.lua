----test auth-------
-- It provide access to mysql for message server

local mysql = require "resty.mysql"

ngx.header.content_type = "text/plain";

local configs = require "config"

-- 验证通过，生成并返回json数据
function print_resp(cont)
    local res_json = '{ "code":0, "message":"SUCCESS" }'

    ngx.say(res_json)
    ngx.exit(ngx.HTTP_OK)
end

function connect_sql(cmdtype)
    local db, err = mysql:new()
    if not db then
        ngx.say(string.format('{"cmd":%d, "code":%d, "message":"%s","content":""}',
            cmdtype, configs["ERR_SERVER"],configs["ERR_DES_SERVER"]))
        ngx.exit(ngx.HTTP_OK)
    return
    end

    db:set_timeout(1000) -- 1 sec

    local ok, err, errno, sqlstate = db:connect{
        host = configs["mysql_host"],
        port = configs["mysql_port"],
        database = configs["mysql_db"],
        user = configs["mysql_user"],
        password = configs["mysql_password"],
        max_packet_size = 1024 * 1024
    }

    if not ok then
        ngx.say(string.format('{"cmd":%d, "code":%d, "message":"%s","content":""}',
            cmdtype, configs["ERR_SERVER"],configs["ERR_DES_SERVER"]))
        ngx.exit(ngx.HTTP_OK)
    end

    return db
end

function close_sql(db)
    -- put it into the connection pool of size 100,
    -- with 10 seconds max idle timeout
    local ok, err = db:set_keepalive(10000, 100)
    if not ok then
        --ngx.say("failed to set keepalive: ", err)
        return
    end
end

function mysql_exe()
    local con_type = ngx.var.content_type

    if con_type ~= "application/json;charset=UTF-8" then
        ngx.say(string.format('{"cmd":%d, "code":%d, "message":"%s","content":""}',
            cmdtype, configs["ERR_CONTENT_TYPE"],configs["ERR_DES_CONTENT_TYPE"]))
        ngx.exit(ngx.HTTP_OK)
        return
    else
        ngx.req.read_body()
        local sql = ngx.req.get_body_data()
        if not sql then
            ngx.say(string.format('{"cmd":%d, "code":%d, "message":"%s","content":""}',
                cmdtype, configs["ERR_BODY_JSON"],configs["ERR_DES_BODY_JSON"]))
            ngx.exit(ngx.HTTP_OK)
        end

        -- 执行SQL语句
        local db = connect_sql(configs["CMD_INVALID"])
        db:query(sql)
        close_sql(db)

        -- 输出成功信息
        print_resp(cont)
    end
end


mysql_exe()
