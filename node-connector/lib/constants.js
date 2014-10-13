var Constants;

Constants = (function() {
  function Constants() {}

  Constants.ZMQ_REQ = "req";

  Constants.ZMQ_REP = "rep";

  Constants.ZMQ_PUSH = "push";

  Constants.ZMQ_PULL = "pull";

  Constants.ZMQ_PUB = "pub";

  Constants.ZMQ_SUB = "sub";

  Constants.SOCKET_TYPE = {
    PUSH_PULL: "PUSH_PULL",
    REQ_REP: "REQ_REP",
    PUB_SUB: "PUB_SUB"
  };

  Constants.ENDPOINT_TYPE = {
    NONE: "NONE",
    QUERY: "QUERY",
    COLUMN: "COLUMN",
    META_DATA: "META_DATA",
    DB_CONFIG: "DB_CONFIG",
    DB_CONFIG_QUERY: "DB_CONFIG_QUERY",
    LOG_RECORD: "LOG_RECORD",
    GET_LOGS: "GET_LOGS",
    CONFIG: "CONFIG",
    ENDPOINT: "ENDPOINT",
    IP_DISCOVERY: "IP_DISCOVERY",
    OTHER: "OTHER"
  };

  Constants.EVERY_CHANNEL = "";

  Constants.SERVER_CONFIG_TYPE = "42";

  return Constants;

})();

module.exports = Constants;

//# sourceMappingURL=constants.js.map