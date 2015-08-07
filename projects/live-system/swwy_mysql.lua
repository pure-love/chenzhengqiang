--author:chenzhengqiang
--start date:2015/7/20
--modified date:
--company:swwy
--desc:providing the connection to mysql


--global configuration for mysql
HOST="127.0.0.1"
PORT=3306
DATABASE="daemon"
USER="swwy"
PASSWD="123321"
MAX_PACKET_SIZE=1024*1024
--end
--
local _swwy_mysql={ _version="1.7.1" }

--get the mysql db connection handler
function _swwy_mysql.get_mysql_handler()
    local mysql = require "resty.mysql"
    local db,error = mysql:new()
    if not db then
        return nil
    end
    local ok,err,errno,sqlstate = db:connect{
                host=HOST,
                port=PORT,
                database=DATABASE,
                user=USER,
                password=PASSWD,
                max_packet_size=MAX_PACKET_SIZE  
    }
    if not ok then
        return nil
    end
    return db
    end

function _swwy_mysql.exec_mysql_query(mysql_handler,sql)
    results,error,errno,sqlstate = mysql_handler:query(sql)
    if not results then
        return nil
    end
    return results
end

function _swwy_mysql.close_mysql_handler(mysql_handler)
    mysql_handler:close()
end
return _swwy_mysql
