var Connector, Diag, Protocol, VirtDB,
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };

Protocol = require('./protocol');

Diag = require('./diag');

Connector = require('./connector');

VirtDB = (function() {
  function VirtDB(name, connectionString) {
    var endpoint;
    this.name = name;
    this.close = __bind(this.close, this);
    this.onQuery = __bind(this.onQuery, this);
    this.onMetaDataRequest = __bind(this.onMetaDataRequest, this);
    Protocol.svcConfig(connectionString, Connector.onEndpoint);
    endpoint = {
      Endpoints: [
        {
          Name: this.name,
          SvcType: 'NONE'
        }
      ]
    };
    Protocol.sendEndpoint(endpoint);
  }

  VirtDB.prototype.onMetaDataRequest = function(callback) {
    Connector.onIP((function(_this) {
      return function() {
        return Connector.setupEndpoint(_this.name, Protocol.metaDataServer, callback);
      };
    })(this));
  };

  VirtDB.prototype.onQuery = function(callback) {
    return Connector.onIP((function(_this) {
      return function() {
        Connector.setupEndpoint(_this.name, Protocol.queryServer, callback);
        return Connector.setupEndpoint(_this.name, Protocol.columnServer);
      };
    })(this));
  };

  VirtDB.prototype.sendMetaData = function(data) {
    return Protocol.sendMetaData(data);
  };

  VirtDB.prototype.sendColumn = function(data) {
    return Protocol.sendColumn(data);
  };

  VirtDB.prototype.close = function() {
    return Protocol.close();
  };

  VirtDB.log = Diag;

  return VirtDB;

})();

module.exports = VirtDB;

//# sourceMappingURL=index.js.map