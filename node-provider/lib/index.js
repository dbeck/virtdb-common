var Protocol, VirtDBConnector, VirtDBDataProvider,
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };

Protocol = require('./protocol');

VirtDBConnector = require('virtdb-connector');

VirtDBDataProvider = (function() {
  function VirtDBDataProvider(name, connectionString) {
    this.name = name;
    this.close = __bind(this.close, this);
    this.onQuery = __bind(this.onQuery, this);
    this.onMetaDataRequest = __bind(this.onMetaDataRequest, this);
    VirtDBConnector.connect(this.name, connectionString);
  }

  VirtDBDataProvider.prototype.onMetaDataRequest = function(callback) {
    VirtDBConnector.onIP((function(_this) {
      return function() {
        return VirtDBConnector.setupEndpoint(_this.name, Protocol.metaDataServer, callback);
      };
    })(this));
  };

  VirtDBDataProvider.prototype.onQuery = function(callback) {
    return VirtDBConnector.onIP((function(_this) {
      return function() {
        VirtDBConnector.setupEndpoint(_this.name, Protocol.queryServer, callback);
        return VirtDBConnector.setupEndpoint(_this.name, Protocol.columnServer);
      };
    })(this));
  };

  VirtDBDataProvider.prototype.sendMetaData = function(data) {
    return Protocol.sendMetaData(data);
  };

  VirtDBDataProvider.prototype.sendColumn = function(data) {
    return Protocol.sendColumn(data);
  };

  VirtDBDataProvider.prototype.close = function() {
    return Protocol.close();
  };

  VirtDBDataProvider.log = VirtDBConnector.log;

  return VirtDBDataProvider;

})();

module.exports = VirtDBDataProvider;

//# sourceMappingURL=index.js.map