zmq = require "zmq"
fs = require "fs"
protobuf = require "node-protobuf"
log = require "loglevel"
Const = require "./constants"
EndpointService = require "./endpoint_service"
Convert = require "./convert"

require("source-map-support").install()
log.setLevel "debug"

serviceConfigProto = new protobuf(fs.readFileSync(__dirname + "/proto/svc_config.pb.desc"))

class ConfigService

    instance: null

    @getInstance: () ->
        @instance ?= new ConfigServiceConnector

    @reset: () ->
        @instance = null

    @ConvertTemplateToOld: (source) ->
        Convert.TemplateToOld source

    @ConvertTemplateToNew: (source) ->
        Convert.TemplateToNew source

    @ConvertToOld: (source) ->
        Convert.ToOld source

    @ConvertToNew: (source) ->
        Convert.ToNew source

    class ConfigServiceConnector

        reqRepSocket: null
        configs: null

        constructor: () ->
            @configs = {}
            @reqRepSocket = zmq.socket(Const.ZMQ_REQ)
            @reqRepSocket.on "message", @_onMessage
            @connect()
            @_requestConfigs()

        connect: =>
            try
                addresses = EndpointService.getInstance().getConfigServiceAddresses()
                @reqRepSocket.connect(addresses[Const.ENDPOINT_TYPE.CONFIG][Const.SOCKET_TYPE.REQ_REP][0])
                log.debug "Connected to the config service!"
            catch ex
                log.error "Error during connecting to config service!", ex, addresses

        getConfigs: () =>
            return @configs

        sendConfig: (config) =>
            try
                @reqRepSocket.send serviceConfigProto.serialize config, "virtdb.interface.pb.Config"
            catch ex
                log.error "Error during sending config!", ex

        _onMessage: (message) =>
            configMessage = serviceConfigProto.parse message, "virtdb.interface.pb.Config"
            log.debug "Got config message: ", configMessage
            @configs[configMessage.Name] = configMessage
            @_subscribeConfigs() unless @pubsubSocket
            return

        _requestConfigs: () =>
            components = EndpointService.getInstance().getComponents()
            for component in components
                configMessage =
                    Name: component
                @reqRepSocket.send serviceConfigProto.serialize configMessage, "virtdb.interface.pb.Config"
            return

        _onPublishedMessage: (channelId, message) =>
            configMessage = (serviceConfigProto.parse message, "virtdb.interface.pb.Config")
            @configs[configMessage.Name] = configMessage

        _subscribeConfigs: () =>
            addresses = EndpointService.getInstance().getServiceConfigAddresses()
            @pubsubSocket = zmq.socket(Const.ZMQ_SUB)
            @pubsubSocket.on "message", @_onPublishedMessage
            for connection in addresses when connection.Type is Const.SOCKET_TYPE.PUB_SUB
                for address in addresses[Const.ENDPOINT_TYPE.CONFIG][Const.SOCKET_TYPE.PUB_SUB]
                    try
                        @pubsubSocket.connect address
                    catch ex
                        continue
                    @pubsubSocket.subscribe Const.EVERY_CHANNEL
                    break

module.exports = ConfigService
