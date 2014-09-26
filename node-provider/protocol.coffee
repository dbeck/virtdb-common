fs          = require 'fs'
zmq         = require 'zmq'
protobuf    = require 'node-protobuf'

proto_meta           = new protobuf(fs.readFileSync('../../src/common/proto/meta_data.pb.desc'))
proto_data           = new protobuf(fs.readFileSync('../../src/common/proto/data.pb.desc'))


class Protocol
    @metadata_socket = null
    @column_socket = null

    @metaDataServer = (name, connectionString, onRequest, onBound) =>
        @metadata_socket = zmq.socket "rep"
        @metadata_socket.on "message", (request) =>
            try
                newData = proto_meta.parse(request, "virtdb.interface.pb.MetaDataRequest")
                onRequest newData
            catch ex
                @metadata_socket.send 'err'
            return
        @metadata_socket.bind connectionString, onBound name, @metadata_socket, 'META_DATA', 'REQ_REP'

    @queryServer = (name, connectionString, onQuery, onBound) =>
        @query_socket = zmq.socket "pull"
        @query_socket.on "message", (request) =>
            try
                query = proto_data.parse(request, "virtdb.interface.pb.Query")
                onQuery query
            catch ex
                @query_socket.send 'err'
            return
        @query_socket.bind connectionString, onBound name, @query_socket, 'QUERY', 'PUSH_PULL'

    @columnServer = (name, connectionString, callback, onBound) =>
        @column_socket = zmq.socket "pub"
        @column_socket.bind connectionString, onBound name, @column_socket, 'COLUMN', 'PUB_SUB'

    @sendMetaData = (data) =>
        @metadata_socket.send proto_meta.serialize data, "virtdb.interface.pb.MetaData"

    @sendColumn = (columnChunk) =>
        @column_socket.send columnChunk.QueryId, zmq.ZMQ_SNDMORE
        @column_socket.send proto_data.serialize columnChunk, "virtdb.interface.pb.Column"

    @close = () =>
        @metadata_socket?.close()
        @column_socket?.close()



module.exports = Protocol
