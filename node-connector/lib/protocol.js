var Protocol, async, fs, proto_diag, proto_service_config, protobuf, zmq;

fs = require('fs');

zmq = require('zmq');

protobuf = require('node-protobuf');

async = require("async");

proto_service_config = new protobuf(fs.readFileSync(__dirname + '/proto/svc_config.pb.desc'));

proto_diag = new protobuf(fs.readFileSync(__dirname + '/proto/diag.pb.desc'));

Protocol = (function() {
  function Protocol() {}

  Protocol.svcConfigSocket = null;

  Protocol.diag_socket = null;

  Protocol.diagAddress = null;

  Protocol.svcConfig = function(connectionString, onEndpoint) {
    this.svcConfigSocket = zmq.socket('req');
    this.svcConfigSocket.on('message', function(message) {
      var endpoint, endpointMessage, _i, _len, _ref, _results;
      endpointMessage = proto_service_config.parse(message, 'virtdb.interface.pb.Endpoint');
      _ref = endpointMessage.Endpoints;
      _results = [];
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        endpoint = _ref[_i];
        _results.push(onEndpoint(endpoint));
      }
      return _results;
    });
    return this.svcConfigSocket.connect(connectionString);
  };

  Protocol.sendEndpoint = function(endpoint) {
    return this.svcConfigSocket.send(proto_service_config.serialize(endpoint, 'virtdb.interface.pb.Endpoint'));
  };

  Protocol.bindHandler = function(socket, svcType, zmqType, onBound) {
    return function(err) {
      var zmqAddress;
      zmqAddress = "";
      if (!err) {
        zmqAddress = socket.getsockopt(zmq.ZMQ_LAST_ENDPOINT);
      }
      return onBound(err, svcType, zmqType, zmqAddress);
    };
  };

  Protocol.onDiagSocket = function(callback) {
    if (Protocol.diag_socket != null) {
      return callback();
    } else {
      return async.retry(5, function(retry_callback, results) {
        return setTimeout(function() {
          var err;
          err = null;
          if (Protocol.diag_socket == null) {
            err = "diag_socket is not set yet";
          }
          return retry_callback(err, Protocol.diag_socket);
        }, 50);
      }, function() {
        if (Protocol.diag_socket != null) {
          return callback();
        }
      });
    }
  };

  Protocol.sendDiag = function(logRecord) {
    return Protocol.onDiagSocket(function() {
      return Protocol.diag_socket.send(proto_diag.serialize(logRecord, "virtdb.interface.pb.LogRecord"));
    });
  };

  Protocol.connectToDiag = function(addresses) {
    var connected, ret;
    ret = null;
    connected = false;
    async.eachSeries(addresses, function(address, callback) {
      var e, socket;
      try {
        if (ret) {
          callback();
          return;
        }
        if (address === Protocol.diagAddress) {
          ret = address;
          callback();
          return;
        }
        socket = zmq.socket("push");
        socket.connect(address);
        Protocol.diagAddress = address;
        ret = address;
        Protocol.diag_socket = socket;
        connected = true;
        return callback();
      } catch (_error) {
        e = _error;
        return callback(e);
      }
    }, function(err) {});
    if (connected) {
      return ret;
    } else {
      return null;
    }
  };

  Protocol.close = function() {
    var _ref;
    return (_ref = Protocol.svcConfigSocket) != null ? _ref.close() : void 0;
  };

  return Protocol;

})();

module.exports = Protocol;

//# sourceMappingURL=protocol.js.map