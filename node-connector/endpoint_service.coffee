zmq = require "zmq"
fs = require "fs"
protobuf = require "node-protobuf"
log = require "loglevel"
Const = require "./constants"

require("source-map-support").install()
log.setLevel "debug"

serviceConfigProto = new protobuf(fs.readFileSync(__dirname + "/proto/svc_config.pb.desc"))

class EndpointService

    instance: null
    serviceConfigName: null
    serviceConfigAddress: null

    @getInstance: () ->
        @instance ?= new EndpointServiceConnector(@serviceConfigAddress)

    @reset: () ->
        @instance = null

    @setConnectionData: (name, address) ->
        @serviceConfigName = name
        @serviceConfigAddress = address

    class EndpointServiceConnector
        reqrepSocket: null
        pubsubSocket: null
        endpoints: []
        serviceConfigConnections: []
        name: null
        address: null

        constructor: (@address) ->
            @svcConfigName = 'config-service'
            @reqrepSocket = zmq.socket(Const.ZMQ_REQ)
            @reqrepSocket.on "message", @_onMessage
            @connect()
            @_requestEndpoints()

        getComponentAddresses: (name) =>
            addresses = {}
            log.debug "Endpoints: ", @endpoints
            for endpoint in @endpoints when endpoint.Name is name
                addresses[endpoint.SvcType] ?= {}
                for conn in endpoint.Connections
                    addresses[endpoint.SvcType][conn.Type] = conn.Address
            return addresses

        getConfigServiceAddresses: () =>
            log.debug "In getConfigServiceAddresses", @svcConfigName
            return @getComponentAddresses(@svcConfigName)

        getEndpoints: () =>
            @endpoints

        getComponents: () =>
            components = []
            for endpoint in @endpoints
                if endpoint.Name not in components
                    components.push endpoint.Name
            return components

        connect: =>
            try
                @reqrepSocket.connect(@address)
                log.debug "Connected to the endpoint service!"
            catch ex
                log.error "Error during connecting to endpoint service!", ex

        _onMessage: (reply) =>
            @endpoints = (serviceConfigProto.parse reply, "virtdb.interface.pb.Endpoint").Endpoints
            @serviceConfigConnections = endpoint.Connections for endpoint in @endpoints when endpoint.Name is @svcConfigName
            @_subscribeEndpoints() unless @pubsubSocket
            return

        _requestEndpoints: () =>
            endpointMessage =
                Endpoints: [
                    Name: ""
                    SvcType: Const.ENDPOINT_TYPE.NONE
                ]

            @reqrepSocket.send serviceConfigProto.serialize endpointMessage, "virtdb.interface.pb.Endpoint"
            return

        _onPublishedMessage: (channelId, message) =>
            data = (serviceConfigProto.parse message, "virtdb.interface.pb.Endpoint")
            for newEndpoint in data.Endpoints
                for endpoint in @endpoints
                    if endpoint.Name == newEndpoint.Name and endpoint.SvcType == newEndpoint.SvcType
                        @endpoints.splice @endpoints.indexOf(endpoint), 1
                        break
                @endpoints = @endpoints.concat newEndpoint

        _subscribeEndpoints: () =>
            @pubsubSocket = zmq.socket(Const.ZMQ_SUB)
            @pubsubSocket.on "message", @_onPublishedMessage
            for connection in @serviceConfigConnections when connection.Type is Const.SOCKET_TYPE.PUB_SUB
                for address in connection.Address
                    try
                        @pubsubSocket.connect address
                    catch ex
                        continue
                    @pubsubSocket.subscribe Const.EVERY_CHANNEL
                    break

module.exports = EndpointService
