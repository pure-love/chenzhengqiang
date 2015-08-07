--author:chenzhengqiang
--start-date:2015/7/21

local _util={ _version="1.7.1" }
local configs = require "config"
local util_cjson=require "cjson"
local redis = require "resty.redis";
local mysql = require "resty.mysql"
local error_quit = require "error_quit"

function _util.content_type_ok(content_type)
    local cont_type= ngx.var.content_type
    if cont_type ~= content_type then
        return false
    end
    return true
end

function _util.do_check_method_and_content_type(method,content_type)
    local reply={}
    local request_method=ngx.var.request_method
    local cont_type = ngx.var.content_type
    if request_method ~= method then
        reply.code=configs['ERR_REQ_METHOD_ERR']
        reply.message=configs['ERR_DES_REQ_METHOD_ERR']
        ngx.say(util_cjson.encode(reply))
        ngx.exit(ngx.HTTP_OK)
    end
    if content_type ~= cont_type then
        reply.code=configs['ERR_CONTENT_TYPE']
        reply.message=configs['ERR_DES_CONTENT_TYPE']
        ngx.say(util_cjson.encode(reply))
        ngx.exit(ngx.HTTP_OK)  
    end
end

-- Methods of Mysql operations ------------------------------------------------
-- 连接数据库
function _util.connect_sql()
    local db, err = mysql:new()
    if not db then
        error_quit.for_server()
    return
    end

    db:set_timeout(1000) -- 1 sec

    local ok, err, errno, sqlstate = db:connect{
        host = configs["mysql_host"],
        port = 3306,
        database = "daemon",
        user = "swwy",
        password = "123321",
        max_packet_size = 1024 * 1024
    }

    if not ok then
        ngx.say(configs["mysql_host"])
        ngx.say(err)
        ngx.say("222222")
        error_quit.for_server()
    end

    return db
end

-- 执行更新操作
function _util.execute_sql_update(db,sql)
    local res, err, errno, sqlstate = db:query(sql)
    if not res then
        _util.close_sql(db)
        return false
    end

    return true
end

-- 数据入库
function _util.execute_sql_insert(db,sql)
    local res, err, errno, sqlstate =  db:query(sql)
    if errno then
        _util.close_sql(db)
        return errno
    end

    if not res then
        _util.close_sql(db)
        return false
    end

    return true;
end

-- 关闭连接,(此处不关闭连接，而是放入连接池，下次需要connect的时候可以直接从连接池取出来)
function _util.close_sql(db)
    -- put it into the connection pool of size 100,
    -- with 10 seconds max idle timeout
    local ok, err = db:set_keepalive(10000, 100)
    if not ok then
        --ngx.say("failed to set keepalive: ", err)
        return
    end
end

-------------------------------------------------------------------------------
-- 連接Redis
function _util.connect_redis()
    local red = redis:new()
    red:set_timeout(1000) -- 1 sec

    local ok,err = red:connect(configs["rds_host"], configs["rds_port"])
    if not ok then
        error_quit.for_server()
    end

    return red
end

-- 關閉連接,(此處不關閉連接,而是放入連接池,下次需要connect的時候可以直接從連接池中取出來)
function _util.close_redis(red)
    -- put it into the connection pool of size 100,
    -- with 10 seconds max idle timeout
    local ok,err = red:set_keepalive(10000, 100)
    if not ok then
        return
    end
end
    
function _util.decode_json(input, out)
    out[0] = util_cjson.decode(input)
end

return _util
