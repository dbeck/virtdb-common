fs          = require 'fs'
zmq         = require 'zmq'
protobuf    = require 'node-protobuf'
async       = require "async"

proto_service_config = new protobuf(fs.readFileSync(__dirname + '/proto/svc_config.pb.desc'))
proto_diag           = new protobuf(fs.readFileSync(__dirname + '/proto/diag.pb.desc'))


class Protocol
    @svcConfigSocket = null
    @diag_socket = null
    @diagAddress = null

    @svcConfig = (connectionString, onEndpoint) ->
        @svcConfigSocket = zmq.socket 'req'
        @svcConfigSocket.on 'message', (message) ->
            endpointMessage = proto_service_config.parse message, 'virtdb.interface.pb.Endpoint'
            for endpoint in endpointMessage.Endpoints
                onEndpoint endpoint

        @svcConfigSocket.connect connectionString

    @sendEndpoint = (endpoint) ->
        @svcConfigSocket.send proto_service_config.serialize endpoint, 'virtdb.interface.pb.Endpoint'

    @bindHandler = (socket, svcType, zmqType, onBound) ->
        return (err) ->
            zmqAddress = ""
            if not err
                zmqAddress = socket.getsockopt zmq.ZMQ_LAST_ENDPOINT
            onBound err, svcType, zmqType, zmqAddress

    @onDiagSocket = (callback) =>
        if @diag_socket?
            callback()
        else
            async.retry 5, (retry_callback, results) =>
                setTimeout =>
                    err = null
                    # log.debug @diag_socket
                    if not @diag_socket?
                        err = "diag_socket is not set yet"
                    retry_callback err, @diag_socket
                , 50
            , =>
                if @diag_socket?
                    callback()

    @sendDiag = (logRecord) =>
        @onDiagSocket () =>
            # log.debug logRecord
            @diag_socket.send proto_diag.serialize logRecord, "virtdb.interface.pb.LogRecord"

    @connectToDiag = (addresses) =>
        ret = null
        connected = false
        async.eachSeries addresses, (address, callback) =>
            try
                if ret
                    callback()
                    return
                if address == @diagAddress
                    ret = address
                    callback()
                    return
                socket = zmq.socket "push"
                socket.connect address
                @diagAddress = address
                ret = address
                @diag_socket = socket
                connected = true
                callback()
            catch e
                callback e
        , (err) ->
            return
        if connected
            ret
        else
            null

    @close = () =>
        @svcConfigSocket?.close()

module.exports = Protocol
