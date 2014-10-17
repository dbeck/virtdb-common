Convert = require "../convert"

chai = require "chai"

chai.should()

describe "Config service helper", ->
    it "should convert simple config messages", ->
        source =
            Name: "config-convert-test"
            ConfigData: [
                Key: 'GLOBAL'
                Value:
                    Type: 'STRING'
                Children: [
                    Key: 'Path'
                    Value:
                        Type: 'STRING'
                        StringValue: [
                            '/usr/local/lib'
                        ]
                ]
            ]
        destination =
            AppName: "config-convert-test"
            Data: [
                Location: [
                    'GLOBAL',
                    'Path'
                ]
                Data:
                    Type: 'STRING'
                    StringValue: [
                        '/usr/local/lib'
                    ]
            ]
        configObject =
            GLOBAL:
                Path: '/usr/local/lib'

        Convert.ToNew(source).should.deep.equal(destination)
        Convert.ToObject(destination).should.deep.equal(configObject)
        # Convert.ToNew(Convert.ToOld(source)).should.deep.equal(source)

    it "should convert complex config messages", ->
        source =
            Name: "config-convert-test"
            ConfigData: [
                Key: 'Postgres'
                Value:
                    Type: 'STRING'
                Children: [
                    Key: 'Host'
                    Value:
                        Type: 'STRING'
                        StringValue: [
                            '192.168.221.11'
                        ]
                ,
                    Key: 'Port'
                    Value:
                        Type: 'UINT32'
                        UInt32Value: [
                            5432
                        ]
                ,
                    Key: 'Catalog'
                    Value:
                        Type: 'STRING'
                        StringValue: [
                            'gpadmin'
                        ]
                ,
                    Key: 'User'
                    Value:
                        Type: 'STRING'
                        StringValue: [
                            'gpadmin'
                        ]
                ,
                    Key: 'Password'
                    Value:
                        Type: 'STRING'
                        StringValue: [
                            'manager'
                        ]
                ]
            ,
                Key: 'Extension'
                Value:
                    Type: 'STRING'
                Children: [
                    Key: 'Path'
                    Value:
                        Type: 'STRING'
                        StringValue: [
                            '/usr/local/libgreenplum_ext.so'
                        ]
                ]
            ]
        destination =
            AppName: "config-convert-test"
            Data: [
                Location: [
                    'Postgres',
                    'Host'
                ]
                Data:
                    Type: 'STRING'
                    StringValue: [
                        '192.168.221.11'
                    ]
            ,
                Location: [
                    'Postgres',
                    'Port'
                ]
                Data:
                    Type: 'UINT32'
                    UInt32Value: [
                        5432
                    ]
            ,
                Location: [
                    'Postgres',
                    'Catalog'
                ]
                Data:
                    Type: 'STRING'
                    StringValue: [
                        'gpadmin'
                    ]
            ,
                Location: [
                    'Postgres',
                    'User'
                ]
                Data:
                    Type: 'STRING'
                    StringValue: [
                        'gpadmin'
                    ]
            ,
                Location: [
                    'Postgres',
                    'Password'
                ]
                Data:
                    Type: 'STRING'
                    StringValue: [
                        'manager'
                    ]
            ,
                Location: [
                    'Extension',
                    'Path'
                ]
                Data:
                    Type: 'STRING'
                    StringValue: [
                        '/usr/local/libgreenplum_ext.so'
                    ]
            ,
            ]
        configObject =
            Postgres:
                Host: '192.168.221.11'
                Port: 5432
                Catalog: 'gpadmin'
                User: 'gpadmin'
                Password: 'manager'
            Extension:
                Path: '/usr/local/libgreenplum_ext.so'

        Convert.ToNew(source).should.deep.equal(destination)
        Convert.ToObject(destination).should.deep.equal(configObject)
        # Convert.ToNew(Convert.ToOld(source)).should.deep.equal(source)
