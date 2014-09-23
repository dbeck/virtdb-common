# CONST = require('./config').Const
Protocol    = require './protocol'
VirtDBConnector   = require 'virtdb-connector'

class VirtDBDataProvider

    connector: null

    constructor: (@name, connectionString) ->
        @connector = new VirtDBConnector(@name, connectionString)
        @connector.connect()

    onMetaDataRequest: (callback) =>
        @connector.onIP =>
            @connector.setupEndpoint @name, Protocol.metaDataServer, callback
        return

    onQuery: (callback) =>
        @connector.onIP =>
            @connector.setupEndpoint @name, Protocol.queryServer, callback
            @connector.setupEndpoint @name, Protocol.columnServer

    sendMetaData: (data) ->
        Protocol.sendMetaData data

    sendColumn: (data) ->
        Protocol.sendColumn data

    close: =>
        Protocol.close()

    @log = VirtDBConnector.Log

module.exports = VirtDBDataProvider
