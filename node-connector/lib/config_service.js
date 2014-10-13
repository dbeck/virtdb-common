var ConfigService, Const, EndpointService, fs, log, protobuf, serviceConfigProto, zmq,
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };

zmq = require("zmq");

fs = require("fs");

protobuf = require("node-protobuf");

log = require("loglevel");

Const = require("./constants");

EndpointService = require("./endpoint_service");

require("source-map-support").install();

log.setLevel("debug");

serviceConfigProto = new protobuf(fs.readFileSync(__dirname + "/proto/svc_config.pb.desc"));

ConfigService = (function() {
  var ConfigServiceConnector;

  function ConfigService() {}

  ConfigService.prototype.instance = null;

  ConfigService.getInstance = function() {
    return this.instance != null ? this.instance : this.instance = new ConfigServiceConnector;
  };

  ConfigService.reset = function() {
    return this.instance = null;
  };

  ConfigServiceConnector = (function() {
    ConfigServiceConnector.prototype.reqRepSocket = null;

    ConfigServiceConnector.prototype.configs = null;

    function ConfigServiceConnector() {
      this._subscribeConfigs = __bind(this._subscribeConfigs, this);
      this._onPublishedMessage = __bind(this._onPublishedMessage, this);
      this._requestConfigs = __bind(this._requestConfigs, this);
      this._onMessage = __bind(this._onMessage, this);
      this.sendConfig = __bind(this.sendConfig, this);
      this.getConfigs = __bind(this.getConfigs, this);
      this.connect = __bind(this.connect, this);
      this.configs = {};
      this.reqRepSocket = zmq.socket(Const.ZMQ_REQ);
      this.reqRepSocket.on("message", this._onMessage);
      this.connect();
      this._requestConfigs();
    }

    ConfigServiceConnector.prototype.connect = function() {
      var addresses, ex;
      try {
        addresses = EndpointService.getInstance().getOwnAddress();
        this.reqRepSocket.connect(addresses[Const.ENDPOINT_TYPE.CONFIG][Const.SOCKET_TYPE.REQ_REP][0]);
        return log.debug("Connected to the config service!");
      } catch (_error) {
        ex = _error;
        return log.error("Error during connecting to config service!", ex);
      }
    };

    ConfigServiceConnector.prototype.getConfigs = function() {
      return this.configs;
    };

    ConfigServiceConnector.prototype.sendConfig = function(config) {
      var ex;
      try {
        return this.reqRepSocket.send(serviceConfigProto.serialize(config, "virtdb.interface.pb.Config"));
      } catch (_error) {
        ex = _error;
        return log.error("Error during sending config!", ex);
      }
    };

    ConfigServiceConnector.prototype._onMessage = function(message) {
      var configMessage;
      configMessage = serviceConfigProto.parse(message, "virtdb.interface.pb.Config");
      log.debug("Got config message: ", configMessage);
      this.configs[configMessage.Name] = configMessage;
      if (!this.pubsubSocket) {
        this._subscribeConfigs();
      }
    };

    ConfigServiceConnector.prototype._requestConfigs = function() {
      var component, components, configMessage, _i, _len;
      components = EndpointService.getInstance().getComponents();
      for (_i = 0, _len = components.length; _i < _len; _i++) {
        component = components[_i];
        configMessage = {
          Name: component
        };
        this.reqRepSocket.send(serviceConfigProto.serialize(configMessage, "virtdb.interface.pb.Config"));
      }
    };

    ConfigServiceConnector.prototype._onPublishedMessage = function(channelId, message) {
      var configMessage;
      configMessage = serviceConfigProto.parse(message, "virtdb.interface.pb.Config");
      return this.configs[configMessage.Name] = configMessage;
    };

    ConfigServiceConnector.prototype._subscribeConfigs = function() {
      var address, addresses, connection, ex, _i, _len, _results;
      addresses = EndpointService.getInstance().getOwnAddress();
      this.pubsubSocket = zmq.socket(Const.ZMQ_SUB);
      this.pubsubSocket.on("message", this._onPublishedMessage);
      _results = [];
      for (_i = 0, _len = addresses.length; _i < _len; _i++) {
        connection = addresses[_i];
        if (connection.Type === Const.SOCKET_TYPE.PUB_SUB) {
          _results.push((function() {
            var _j, _len1, _ref, _results1;
            _ref = addresses[Const.ENDPOINT_TYPE.CONFIG][Const.SOCKET_TYPE.PUB_SUB];
            _results1 = [];
            for (_j = 0, _len1 = _ref.length; _j < _len1; _j++) {
              address = _ref[_j];
              try {
                this.pubsubSocket.connect(address);
              } catch (_error) {
                ex = _error;
                continue;
              }
              this.pubsubSocket.subscribe(Const.EVERY_CHANNEL);
              break;
            }
            return _results1;
          }).call(this));
        }
      }
      return _results;
    };

    return ConfigServiceConnector;

  })();

  return ConfigService;

})();

module.exports = ConfigService;

//# sourceMappingURL=config_service.js.map