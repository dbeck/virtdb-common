var Protocol, async, fs, proto_data, proto_diag, proto_meta, proto_service_config, protobuf, zmq;

fs = require('fs');

zmq = require('zmq');

protobuf = require('node-protobuf');

async = require("async");

proto_service_config = new protobuf(fs.readFileSync('../../src/common/proto/svc_config.pb.desc'));

proto_meta = new protobuf(fs.readFileSync('../../src/common/proto/meta_data.pb.desc'));

proto_data = new protobuf(fs.readFileSync('../../src/common/proto/data.pb.desc'));

proto_diag = new protobuf(fs.readFileSync('../../src/common/proto/diag.pb.desc'));

Protocol = (function() {
  function Protocol() {}

  Protocol.svcConfigSocket = null;

  Protocol.metadata_socket = null;

  Protocol.column_socket = null;

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

  Protocol.metaDataServer = function(connectionString, onRequest, onBound) {
    Protocol.metadata_socket = zmq.socket("rep");
    Protocol.metadata_socket.on("message", function(request) {
      var ex, newData;
      try {
        newData = proto_meta.parse(request, "virtdb.interface.pb.MetaDataRequest");
        onRequest(newData);
      } catch (_error) {
        ex = _error;
        console.log(ex);
        Protocol.metadata_socket.send('err');
      }
    });
    return Protocol.metadata_socket.bind(connectionString, Protocol.bindHandler(Protocol.metadata_socket, 'META_DATA', 'REQ_REP', onBound));
  };

  Protocol.queryServer = function(connectionString, onQuery, onBound) {
    Protocol.query_socket = zmq.socket("pull");
    Protocol.query_socket.on("message", function(request) {
      var ex, query;
      try {
        query = proto_data.parse(request, "virtdb.interface.pb.Query");
        onQuery(query);
      } catch (_error) {
        ex = _error;
        console.log(ex);
        Protocol.query_socket.send('err');
      }
    });
    return Protocol.query_socket.bind(connectionString, Protocol.bindHandler(Protocol.query_socket, 'QUERY', 'PUSH_PULL', onBound));
  };

  Protocol.columnServer = function(connectionString, callback, onBound) {
    Protocol.column_socket = zmq.socket("pub");
    return Protocol.column_socket.bind(connectionString, Protocol.bindHandler(Protocol.column_socket, 'COLUMN', 'PUB_SUB', onBound));
  };

  Protocol.sendMetaData = function(data) {
    return Protocol.metadata_socket.send(proto_meta.serialize(data, "virtdb.interface.pb.MetaData"));
  };

  Protocol.sendColumn = function(columnChunk) {
    Protocol.column_socket.send(columnChunk.QueryId, zmq.ZMQ_SNDMORE);
    Protocol.column_socket.send(proto_data.serialize(columnChunk, "virtdb.interface.pb.Column"));
    return console.log("Column data sent: ", columnChunk.Name, "(", columnChunk.Data.length(), ")");
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
    }, function(err) {
      if (err) {
        return console.log(err);
      }
    });
    if (connected) {
      return ret;
    } else {
      return null;
    }
  };

  Protocol.close = function() {
    var _ref, _ref1, _ref2;
    if ((_ref = Protocol.svcConfigSocket) != null) {
      _ref.close();
    }
    if ((_ref1 = Protocol.metadata_socket) != null) {
      _ref1.close();
    }
    return (_ref2 = Protocol.column_socket) != null ? _ref2.close() : void 0;
  };

  return Protocol;

})();

module.exports = Protocol;

//# sourceMappingURL=protocol.js.map