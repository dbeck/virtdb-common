var Protocol, fs, proto_data, proto_meta, protobuf, zmq;

fs = require('fs');

zmq = require('zmq');

protobuf = require('node-protobuf');

proto_meta = new protobuf(fs.readFileSync('../../src/common/proto/meta_data.pb.desc'));

proto_data = new protobuf(fs.readFileSync('../../src/common/proto/data.pb.desc'));

Protocol = (function() {
  function Protocol() {}

  Protocol.metadata_socket = null;

  Protocol.column_socket = null;

  Protocol.metaDataServer = function(name, connectionString, onRequest, onBound) {
    Protocol.metadata_socket = zmq.socket("rep");
    Protocol.metadata_socket.on("message", function(request) {
      var ex, newData;
      try {
        newData = proto_meta.parse(request, "virtdb.interface.pb.MetaDataRequest");
        onRequest(newData);
      } catch (_error) {
        ex = _error;
        Protocol.metadata_socket.send('err');
      }
    });
    return Protocol.metadata_socket.bind(connectionString, onBound(name, Protocol.metadata_socket, 'META_DATA', 'REQ_REP'));
  };

  Protocol.queryServer = function(name, connectionString, onQuery, onBound) {
    Protocol.query_socket = zmq.socket("pull");
    Protocol.query_socket.on("message", function(request) {
      var ex, query;
      try {
        query = proto_data.parse(request, "virtdb.interface.pb.Query");
        onQuery(query);
      } catch (_error) {
        ex = _error;
        Protocol.query_socket.send('err');
      }
    });
    return Protocol.query_socket.bind(connectionString, onBound(name, Protocol.query_socket, 'QUERY', 'PUSH_PULL'));
  };

  Protocol.columnServer = function(name, connectionString, callback, onBound) {
    Protocol.column_socket = zmq.socket("pub");
    return Protocol.column_socket.bind(connectionString, onBound(name, Protocol.column_socket, 'COLUMN', 'PUB_SUB'));
  };

  Protocol.sendMetaData = function(data) {
    return Protocol.metadata_socket.send(proto_meta.serialize(data, "virtdb.interface.pb.MetaData"));
  };

  Protocol.sendColumn = function(columnChunk) {
    Protocol.column_socket.send(columnChunk.QueryId, zmq.ZMQ_SNDMORE);
    return Protocol.column_socket.send(proto_data.serialize(columnChunk, "virtdb.interface.pb.Column"));
  };

  Protocol.close = function() {
    var _ref, _ref1;
    if ((_ref = Protocol.metadata_socket) != null) {
      _ref.close();
    }
    return (_ref1 = Protocol.column_socket) != null ? _ref1.close() : void 0;
  };

  return Protocol;

})();

module.exports = Protocol;

//# sourceMappingURL=protocol.js.map