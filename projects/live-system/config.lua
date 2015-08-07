all_config = {
    mysql_host="192.168.1.11",
    mysql_port=3306,
    mysql_user="swwy",
    mysql_password="123321",
    mysql_db="daemon",
    mysql_pool_connections=10,

    -- redis
    rds_host="192.168.1.11",
    rds_port=6379,
    rds_db=2,
    -- err code --
    ERR_HTTP_HEADER = 10001,
    ERR_URL_PARA = 10002,
    ERR_BODY_JSON = 10003,
    ERR_JSON_PARA = 10004,
    ERR_NODATA = 10005,
    ERR_SERVER = 10006,
    ERR_ACCOUNT_PASSWD = 10007,
    ERR_INVALID_REQ = 10008,
    ERR_NO_HANDLE_REQ = 10009,
    ERR_INVALID_TOKEN = 10010,
    ERR_CONTENT_TYPE = 10011,
    ERR_INVALID_DESMSGSRV_IP = 10012,
    ERR_SERVER_TOOBUSY = 10013,
    ERR_RDS_UNAVAILABLE = 10014,
    ERR_LOG_ERR = 10015,
    -- used in file server --
    ERR_NO_INPUTFILENAME = 10016,
    ERR_CANNOT_OPENFILE = 10017,

    ERR_DES_HTTP_HEADER = "Http header wrong",
    ERR_DES_URL_PARA = "URL parameters wrong",
    ERR_DES_BODY_JSON = "Wrong json format in http request body",
    ERR_DES_JSON_PARA = "Lack or wrong body parameters",
    ERR_DES_NODATA = "No data",
    ERR_DES_SERVER = "Server system wrong",
    ERR_DES_ACCOUNT_PASSWD = "No user or password wrong",
    ERR_DES_INVALID_REQ = "Invalid request",
    ERR_DES_NO_HANDLE_REQ = "No handle for request",
    ERR_DES_INVALID_TOKEN = "Invalid token",
    ERR_DES_CONTENT_TYPE = "Invalid content type",
    ERR_DES_INVALID_DESMSGSRV_IP = "Invalid destination msg server ip",
    ERR_DES_SERVER_TOOBUSY = "The server is too busy",
    ERR_DES_RDS_UNAVAILABLE = "The redis server is unavailable",
    ERR_DES_LOG_ERR = "The redis server is unavailable",
    
    -- used in file server --
    ERR_DES_NO_INPUTFILENAME = "No input filename",
    ERR_DES_CANNOT_OPENFILE = "Failed to open file",
    -- err code --

    -- command type
    CMD_INVALID=10000,
    -- command type
    
}
return all_config;
