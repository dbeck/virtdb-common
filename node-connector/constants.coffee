class Constants

    @ZMQ_REQ = "req"
    @ZMQ_REP = "rep"
    @ZMQ_PUSH = "push"
    @ZMQ_PULL = "pull"
    @ZMQ_PUB = "pub"
    @ZMQ_SUB = "sub"

    @SOCKET_TYPE =
        PUSH_PULL: "PUSH_PULL"
        REQ_REP: "REQ_REP"
        PUB_SUB: "PUB_SUB"

    @ENDPOINT_TYPE =
        NONE: "NONE"
        QUERY: "QUERY"
        COLUMN: "COLUMN"
        META_DATA: "META_DATA"
        DB_CONFIG: "DB_CONFIG"
        DB_CONFIG_QUERY: "DB_CONFIG_QUERY"
        LOG_RECORD: "LOG_RECORD"
        GET_LOGS: "GET_LOGS"
        CONFIG: "CONFIG"
        ENDPOINT: "ENDPOINT"
        IP_DISCOVERY: "IP_DISCOVERY"
        OTHER: "OTHER"

    @EVERY_CHANNEL = ""

    @SERVER_CONFIG_TYPE = "42"

module.exports = Constants
