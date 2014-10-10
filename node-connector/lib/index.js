var ConfigService, Constants, EndpointService, KeyValue, Protocol, VirtDBConnector, async, log, udp, zmq;

udp = require('dgram');

async = require("async");

Protocol = require('./protocol');

log = require('./diag');

zmq = require('zmq');

ConfigService = require("./config_service");

KeyValue = require("./key_value");

EndpointService = require("./endpoint_service");

Constants = require("./constants");

VirtDBConnector = (function() {
  function VirtDBConnector() {}

  VirtDBConnector.IP = null;

  VirtDBConnector.log = log;

  VirtDBConnector.KeyValue = KeyValue;

  VirtDBConnector.ConfigService = ConfigService;

  VirtDBConnector.EndpointService = EndpointService;

  VirtDBConnector.Constants = Constants;

  VirtDBConnector.connect = function(name, connectionString) {
    var endpoint;
    Protocol.svcConfig(connectionString, VirtDBConnector.onEndpoint);
    endpoint = {
      Endpoints: [
        {
          Name: name,
          SvcType: 'NONE'
        }
      ]
    };
    return Protocol.sendEndpoint(endpoint);
  };

  VirtDBConnector.close = function() {
    return Protocol.close();
  };

  VirtDBConnector.onEndpoint = function(endpoint) {
    var connection, newAddress, _i, _j, _len, _len1, _ref, _ref1, _results, _results1;
    switch (endpoint.SvcType) {
      case 'IP_DISCOVERY':
        if (VirtDBConnector.IP == null) {
          _ref = endpoint.Connections;
          _results = [];
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            connection = _ref[_i];
            if (connection.Type === 'RAW_UDP') {
              _results.push(VirtDBConnector._findMyIP(connection.Address[0]));
            } else {
              _results.push(void 0);
            }
          }
          return _results;
        }
        break;
      case 'LOG_RECORD':
        _ref1 = endpoint.Connections;
        _results1 = [];
        for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
          connection = _ref1[_j];
          if (connection.Type === 'PUSH_PULL') {
            newAddress = Protocol.connectToDiag(connection.Address);
            if (newAddress != null) {
              _results1.push(console.log("Connected to logger: ", newAddress));
            } else {
              _results1.push(void 0);
            }
          } else {
            _results1.push(void 0);
          }
        }
        return _results1;
    }
  };

  VirtDBConnector._findMyIP = function(discoveryAddress) {
    var address, client, ip, message, parts, port;
    if (discoveryAddress.indexOf('raw_udp://' === 0)) {
      client = null;
      message = new Buffer('?');
      address = discoveryAddress.replace(/^raw_udp:\/\//, '');
      if (address.indexOf('[') > -1) {
        ip = address.replace(/^\[|\]:[0-9]{2,5}/g, '');
        port = address.replace(/\[.*\]:/g, '');
        client = udp.createSocket('udp6');
      } else {
        parts = address.split(':');
        ip = parts[0];
        port = parts[1];
        client = udp.createSocket('udp4');
      }
      if (client != null) {
        client.on('message', function(message, remote) {
          VirtDBConnector.IP = message.toString();
          return client.close();
        });
      }
      return async.retry(5, function(callback, results) {
        var err;
        err = null;
        if (client != null) {
          client.send(message, 0, 1, port, ip, function(err, bytes) {
            if (err) {
              return console.log(err);
            }
          });
        }
        return setTimeout(function() {
          if (VirtDBConnector.IP === null) {
            err = "IP is not set yet!";
          }
          return callback(err, VirtDBConnector.IP);
        }, 50);
      }, function() {});
    }
  };

  VirtDBConnector.onIP = function(callback) {
    if (VirtDBConnector.IP != null) {
      console.log("Our IP:", VirtDBConnector.IP);
      return callback();
    } else {
      return async.retry(5, function(retry_callback, results) {
        return setTimeout(function() {
          var err;
          err = null;
          if (VirtDBConnector.IP == null) {
            err = "IP is not set yet";
          }
          return retry_callback(err, VirtDBConnector.IP);
        }, 50);
      }, function() {
        if (VirtDBConnector.IP != null) {
          console.log("Our IP:", VirtDBConnector.IP);
          return callback();
        } else {
          throw "Unable to detect own IP.";
        }
      });
    }
  };

  VirtDBConnector.setupEndpoint = function(name, protocol_call, callback) {
    protocol_call(name, 'tcp://' + VirtDBConnector.IP + ':*', callback, VirtDBConnector.OnBound);
  };

  VirtDBConnector.OnBound = function(name, socket, svcType, zmqType) {
    return function(err) {
      var endpoint, zmqAddress;
      if (err) {
        log.error("Error during binding socket: " + err);
        return;
      }
      zmqAddress = socket.getsockopt(zmq.ZMQ_LAST_ENDPOINT);
      console.log("Listening (" + svcType + ") on", zmqAddress);
      log.info("Listening (" + svcType + ") on", zmqAddress);
      endpoint = {
        Endpoints: [
          {
            Name: name,
            SvcType: svcType,
            Connections: [
              {
                Type: zmqType,
                Address: [zmqAddress]
              }
            ]
          }
        ]
      };
      return Protocol.sendEndpoint(endpoint);
    };
  };

  return VirtDBConnector;

})();

module.exports = VirtDBConnector;

//# sourceMappingURL=index.js.map