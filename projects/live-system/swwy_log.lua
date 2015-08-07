--author:chenzhengqiang
--start date:2015/7/21
--modified date:
--company:swwy
--desc:providing the logging function

LOG_FILE="/var/log/swwy.daemon.log"
LOG_ERROR=0
LOG_INFO=1
LOG_DEBUG=2
LOG_LEVEL=nil
FILE_MOD="w+"
LOG_FILE_HANDLER=nil

local _SWWY_LOG={ _version="1.7.1" }

function _SWWY_LOG.open_logfile(loglevel)
    LOG_LEVEL=loglevel
    LOG_FILE_HANDLER=io.open(LOG_FILE,FILE_MOD)
end

function _SWWY_LOG.log_module(loglevel,module,desc)
    if loglevel <= LOG_LEVEL then
            local curr_time=os.date("%Y-%m-%d %H:%M:%S")
            local output=string.format("%s : %s : %d : %s",curr_time,module,loglevel,desc)
            LOG_FILE_HANDLER:write(output) 
    end
end

function _SWWY_LOG.close_logfile()
    LOG_FILE_HANDLER:close()
end

return _SWWY_LOG
