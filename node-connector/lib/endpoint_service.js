var Const, EndpointService, fs, log, protobuf, serviceConfigProto, zmq,
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __indexOf = [].indexOf || function(item) { for (var i = 0, l = this.length; i < l; i++) { if (i in this && this[i] === item) return i; } return -1; };

zmq = require("zmq");

fs = require("fs");

protobuf = require("node-protobuf");

log = require("loglevel");

Const = require("./constants");

require("source-map-support").install();

log.setLevel("debug");

serviceConfigProto = new protobuf(fs.readFileSync(__dirname + "/proto/svc_config.pb.desc"));

EndpointService = (function() {
  var EndpointServiceConnector;

  function EndpointService() {}

  EndpointService.prototype.instance = null;

  EndpointService.prototype.serviceConfigName = null;

  EndpointService.prototype.serviceConfigAddress = null;

  EndpointService.getInstance = function() {
    return this.instance != null ? this.instance : this.instance = new EndpointServiceConnector(this.serviceConfigName, this.serviceConfigAddress);
  };

  EndpointService.reset = function() {
    return this.instance = null;
  };

  EndpointService.setConnectionData = function(name, address) {
    this.serviceConfigName = name;
    return this.serviceConfigAddress = address;
  };

  EndpointServiceConnector = (function() {
    EndpointServiceConnector.prototype.reqrepSocket = null;

    EndpointServiceConnector.prototype.pubsubSocket = null;

    EndpointServiceConnector.prototype.endpoints = [];

    EndpointServiceConnector.prototype.serviceConfigConnections = [];

    EndpointServiceConnector.prototype.name = null;

    EndpointServiceConnector.prototype.address = null;

    function EndpointServiceConnector(name, address) {
      this.name = name;
      this.address = address;
      this._subscribeEndpoints = __bind(this._subscribeEndpoints, this);
      this._onPublishedMessage = __bind(this._onPublishedMessage, this);
      this._requestEndpoints = __bind(this._requestEndpoints, this);
      this._onMessage = __bind(this._onMessage, this);
      this.connect = __bind(this.connect, this);
      this.getComponents = __bind(this.getComponents, this);
      this.getEndpoints = __bind(this.getEndpoints, this);
      this.getOwnAddress = __bind(this.getOwnAddress, this);
      this.getComponentAddress = __bind(this.getComponentAddress, this);
      this.reqrepSocket = zmq.socket(Const.ZMQ_REQ);
      this.reqrepSocket.on("message", this._onMessage);
      this.connect();
      this._requestEndpoints();
    }

    EndpointServiceConnector.prototype.getComponentAddress = function(name) {
      var addresses, conn, endpoint, _i, _j, _len, _len1, _name, _ref, _ref1;
      addresses = {};
      _ref = this.endpoints;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        endpoint = _ref[_i];
        if (!(endpoint.Name === name)) {
          continue;
        }
        if (addresses[_name = endpoint.SvcType] == null) {
          addresses[_name] = {};
        }
        _ref1 = endpoint.Connections;
        for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
          conn = _ref1[_j];
          addresses[endpoint.SvcType][conn.Type] = conn.Address;
        }
      }
      return addresses;
    };

    EndpointServiceConnector.prototype.getOwnAddress = function() {
      return this.getComponentAddress(this.name);
    };

    EndpointServiceConnector.prototype.getEndpoints = function() {
      return this.endpoints;
    };

    EndpointServiceConnector.prototype.getComponents = function() {
      var components, endpoint, _i, _len, _ref, _ref1;
      components = [];
      _ref = this.endpoints;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        endpoint = _ref[_i];
        if (_ref1 = endpoint.Name, __indexOf.call(components, _ref1) < 0) {
          components.push(endpoint.Name);
        }
      }
      return components;
    };

    EndpointServiceConnector.prototype.connect = function() {
      var ex;
      try {
        this.reqrepSocket.connect(this.address);
        return log.debug("Connected to the endpoint service!");
      } catch (_error) {
        ex = _error;
        return log.error("Error during connecting to endpoint service!", ex);
      }
    };

    EndpointServiceConnector.prototype._onMessage = function(reply) {
      var endpoint, _i, _len, _ref;
      this.endpoints = (serviceConfigProto.parse(reply, "virtdb.interface.pb.Endpoint")).Endpoints;
      _ref = this.endpoints;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        endpoint = _ref[_i];
        if (endpoint.Name === this.name) {
          this.serviceConfigConnections = endpoint.Connections;
        }
      }
      if (!this.pubsubSocket) {
        this._subscribeEndpoints();
      }
    };

    EndpointServiceConnector.prototype._requestEndpoints = function() {
      var endpointMessage;
      endpointMessage = {
        Endpoints: [
          {
            Name: "",
            SvcType: Const.ENDPOINT_TYPE.NONE
          }
        ]
      };
      this.reqrepSocket.send(serviceConfigProto.serialize(endpointMessage, "virtdb.interface.pb.Endpoint"));
    };

    EndpointServiceConnector.prototype._onPublishedMessage = function(channelId, message) {
      var data, endpoint, newEndpoint, _i, _j, _len, _len1, _ref, _ref1, _results;
      data = serviceConfigProto.parse(message, "virtdb.interface.pb.Endpoint");
      _ref = data.Endpoints;
      _results = [];
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        newEndpoint = _ref[_i];
        _ref1 = this.endpoints;
        for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
          endpoint = _ref1[_j];
          if (endpoint.Name === newEndpoint.Name && endpoint.SvcType === newEndpoint.SvcType) {
            this.endpoints.splice(this.endpoints.indexOf(endpoint), 1);
            break;
          }
        }
        _results.push(this.endpoints = this.endpoints.concat(newEndpoint));
      }
      return _results;
    };

    EndpointServiceConnector.prototype._subscribeEndpoints = function() {
      var address, connection, ex, _i, _len, _ref, _results;
      this.pubsubSocket = zmq.socket(Const.ZMQ_SUB);
      this.pubsubSocket.on("message", this._onPublishedMessage);
      _ref = this.serviceConfigConnections;
      _results = [];
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        connection = _ref[_i];
        if (connection.Type === Const.SOCKET_TYPE.PUB_SUB) {
          _results.push((function() {
            var _j, _len1, _ref1, _results1;
            _ref1 = connection.Address;
            _results1 = [];
            for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
              address = _ref1[_j];
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

    return EndpointServiceConnector;

  })();

  return EndpointService;

})();

module.exports = EndpointService;

//# sourceMappingURL=endpoint_service.js.map