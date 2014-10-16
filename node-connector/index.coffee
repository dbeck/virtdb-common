udp         = require 'dgram'
async       = require "async"
Protocol    = require './protocol'
log         = require './diag'
zmq         = require 'zmq'
ConfigService = require "./config_service"
KeyValue = require "./key_value"
EndpointService = require "./endpoint_service"
Constants = require "./constants"

class VirtDBConnector
    @IP: null
    @log = log
    @KeyValue = KeyValue
    @ConfigService = ConfigService
    @EndpointService = EndpointService
    @Constants = Constants
    @Handlers = {}
    @Sockets = {}

    @connect: (name, connectionString) =>
        Protocol.svcConfig connectionString, @onEndpoint

        endpoint =
            Endpoints: [
                Name: name
                SvcType: 'NONE'
            ]
        Protocol.sendEndpoint endpoint

    @close: =>
        Protocol.close()

    @onAddress: (service_type, connection_type, callback) =>
        @Handlers[service_type] ?= {}
        @Handlers[service_type][connection_type] = callback

    @subscribe: (service_type, connection_type, callback, channel) =>
        @onAddress service_type, connection_type, (name, address) =>
            if connection_type == 'PUB_SUB'
                socket_type = 'sub'
            if @Sockets?[name]?[service_type]?[connection_type]?
                @Sockets[name][service_type][connection_type].unsubscribe channel
                @Sockets[name][service_type][connection_type].close()
                @Sockets[name][service_type][connection_type] = null

            @Sockets[name] ?= {}
            @Sockets[name][service_type] ?= {}
            @Sockets[name][service_type][connection_type] = zmq.socket socket_type
            socket = @Sockets[name][service_type][connection_type]
            socket.on "message", callback
            socket.connect address
            if connection_type == 'PUB_SUB' and channel?
                socket.subscribe channel

    @onEndpoint: (endpoint) =>
        switch endpoint.SvcType
            when 'IP_DISCOVERY'
                if not @IP?
                    for connection in endpoint.Connections
                        if connection.Type == 'RAW_UDP'
                            @_findMyIP connection.Address[0] # TODO should we handle more addresses here?
            when 'LOG_RECORD'
                for connection in endpoint.Connections
                    if connection.Type == 'PUSH_PULL'
                        newAddress = Protocol.connectToDiag connection.Address
                        if newAddress?
                            console.log "Connected to logger: ", newAddress
            else
                if endpoint.Connections?
                    for connection in endpoint.Connections
                        for address in connection.Address
                            @Handlers?[endpoint.SvcType]?[connection.Type]? endpoint.Name, address


    @_findMyIP: (discoveryAddress) =>
        if discoveryAddress.indexOf 'raw_udp://' == 0
            client = null
            message = new Buffer('?')
            address = discoveryAddress.replace /^raw_udp:\/\//, ''
            if address.indexOf('[') > -1 # IPv6
                ip = address.replace /^\[|\]:[0-9]{2,5}/g, ''
                port = address.replace /\[.*\]:/g, ''
                client = udp.createSocket 'udp6'
            else    # IPv4
                parts = address.split(':')
                ip = parts[0]
                port = parts[1]
                client = udp.createSocket 'udp4'

            client?.on 'message', (message, remote) =>
                @IP = message.toString()
                client.close()


            async.retry 5, (callback, results) =>
                err = null
                client?.send message, 0, 1, port, ip, (err, bytes) ->
                    if err
                        console.log err
                setTimeout =>
                    if @IP == null
                        err = "IP is not set yet!"
                    callback err, @IP
                , 50
            , ->
                return

    @onIP: (callback) =>
        if @IP?
            console.log "Our IP:", @IP
            callback()
        else
            async.retry 5, (retry_callback, results) =>
                setTimeout =>
                    err = null
                    if not @IP?
                        err = "IP is not set yet"
                    retry_callback err, @IP
                , 50
            , =>
                if @IP?
                    console.log "Our IP:", @IP
                    callback()
                else
                    throw "Unable to detect own IP."

    @setupEndpoint: (name, protocol_call, callback) =>
        protocol_call name, 'tcp://' + @IP + ':*', callback, @OnBound
        return

    @OnBound = (name, socket, svcType, zmqType) ->
        return (err) ->
            if err
                log.error "Error during binding socket: " + err
                return
            zmqAddress = socket.getsockopt zmq.ZMQ_LAST_ENDPOINT
            console.log "Listening (" + svcType + ") on", zmqAddress
            log.info "Listening (" + svcType + ") on", zmqAddress
            endpoint =
                Endpoints: [
                    Name: name
                    SvcType: svcType
                    Connections: [
                        Type: zmqType
                        Address: [
                            zmqAddress
                        ]
                    ]
                ]
            Protocol.sendEndpoint endpoint

module.exports = VirtDBConnector
