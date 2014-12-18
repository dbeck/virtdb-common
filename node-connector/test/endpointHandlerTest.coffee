EndpointHandler = (require "../protocol").EndpointHandler
protobuf    = require 'virtdb-proto'
zmq     = require 'zmq'

chai = require "chai"
chai.should()

sinon = require "sinon"
sinonChai = require "sinon-chai"
chai.use sinonChai

class SocketStub
    callback: null
    on: (message, @callback) =>

describe "EndpointHandler", ->
    sandbox = null
    socket = null
    url = "tcp://192.168.221.11:12345"

    beforeEach =>
        sandbox = sinon.sandbox.create()
        socket = new SocketStub
        connectStub = sandbox.stub zmq, "socket", (type) ->
            type.should.equal 'req'
            return socket
        socket.connect = sandbox.spy()
        socket.send = sandbox.spy()
        sandbox.spy(socket, 'on')

    afterEach =>
        sandbox.restore()

    it "should connect to endpoint service", ->
        handler = new EndpointHandler url, () ->
        socket.connect.should.have.been.calledWith url
        socket.on.should.have.been.called.once

    it "should not report empty messages", ->
        callback = sandbox.spy()
        handler = new EndpointHandler url, callback
        endpoint = {}
        socket.callback protobuf.service_config.serialize endpoint, "virtdb.interface.pb.Endpoint"
        callback.should.have.not.been.called

    it "should report single message", ->
        callback = sandbox.spy()
        handler = new EndpointHandler url, callback
        endpoint =
            Endpoints: [
                Name: "name"
                SvcType: "QUERY"
            ]
        socket.callback protobuf.service_config.serialize endpoint, "virtdb.interface.pb.Endpoint"
        callback.should.have.been.called.once

    it "should report multiple message", ->
        callback = sandbox.spy()
        handler = new EndpointHandler url, callback
        endpoint =
            Endpoints: [
                Name: "name"
                SvcType: "QUERY"
            ,
                Name: "name"
                SvcType: "COLUMN"
            ]
        socket.callback protobuf.service_config.serialize endpoint, "virtdb.interface.pb.Endpoint"
        callback.should.have.been.called.twice

    it "should survive malformed protocol buffers", ->
        callback = sandbox.spy()
        handler = new EndpointHandler url, callback
        endpoint =
            Endpoints: [
                Name: "name"
            ]
        socket.callback protobuf.service_config.serialize endpoint, "virtdb.interface.pb.Endpoint"
        callback.should.have.not.been.called

    it "should send endpoint messages", ->
        endpoint =
            Endpoints: [
                Name: "name"
                SvcType: "QUERY"
            ]
        handler = new EndpointHandler url, () ->
        handler.send endpoint
        socket.send.should.have.been.called.once
