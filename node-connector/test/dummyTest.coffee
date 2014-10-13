Dummy = require "../dummy"

chai = require "chai"

chai.should()

describe "Dummy", ->
    it "should add two numbers", ->
        Dummy.add(2,4).should.equal(6)
        Dummy.add(3,4).should.equal(7)
