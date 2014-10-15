Convert = require "../convert"

chai = require "chai"

chai.should()

describe "Config service helper", ->
    it "should convert simple template config messages with string", ->
        source =
            AppName: 'config-helper-test'
            Config: [
                VariableName: 'url'
                Scope: 'GLOBAL'
                Type: 'STRING'
            ]
        destination =
            Name: 'config-helper-test'
            ConfigData:
                Key: ''
                Children:[
                    Key: 'url'
                    Children: [
                        Key: 'Value'
                        Value:
                            Type: 'STRING'
                            StringValue: []
                    ,
                        Key: 'Scope'
                        Value:
                            Type: 'STRING'
                            StringValue: [
                                'GLOBAL'
                            ]
                    ]
                ]
        Convert.TemplateToOld(source).should.deep.equal(destination)
        Convert.TemplateToNew(Convert.TemplateToOld(source)).should.deep.equal(source)

    it "should convert simple template config messages with int32", ->
        source =
            AppName: 'config-helper-test'
            Config: [
                VariableName: 'url'
                Scope: 'GLOBAL'
                Type: 'INT32'
            ]
        destination =
            Name: 'config-helper-test'
            ConfigData:
                Key: ''
                Children:[
                    Key: 'url'
                    Children: [
                        Key: 'Value'
                        Value:
                            Type: 'INT32'
                            Int32Value: []
                    ,
                        Key: 'Scope'
                        Value:
                            Type: 'STRING'
                            StringValue: [
                                'GLOBAL'
                            ]
                    ]
                ]
        Convert.TemplateToOld(source).should.deep.equal(destination)
        Convert.TemplateToNew(Convert.TemplateToOld(source)).should.deep.equal(source)

    it "should convert complex templates with hierarchy", ->
        source =
            AppName: 'config-helper-test'
            Config: [
                VariableName: 'Postgres'
                Children: [
                    VariableName: 'Host'
                    Type: 'STRING'
                    Required: true
                ,
                    VariableName: 'Port'
                    Type: 'UINT32'
                    Required: true
                    Default: 5432
                ,
                    VariableName: 'Catalog'
                    Type: 'STRING'
                    Required: true
                    Default: 'gpadmin'
                ,
                    VariableName: 'User'
                    Type: 'STRING'
                    Required: false
                ,
                    VariableName: 'Password'
                    Type: 'STRING'
                    Required: false
                ]
                Scope: 'GLOBAL'
            ,
                VariableName: 'SharedObjectPath'
                Scope: 'GLOBAL'
                Type: 'STRING'
                Required: true
            ]
        destination =
            Name: 'config-helper-test'
            ConfigData:
                Key: ''
                Children:[
                    Key: 'Postgres'
                    Children: [
                        Key: 'Host'
                        Children: [
                            Key: 'Value'
                            Value:
                                Type: 'STRING'
                                StringValue: []
                        ,
                            Key: 'Scope'
                            Value:
                                Type: 'STRING'
                                StringValue: [
                                    'GLOBAL'
                                ]
                        ,
                            Key: 'Required'
                            Value:
                                Type: 'BOOL'
                                BoolValue: [
                                    true
                                ]
                        ]
                    ,
                        Key: 'Port'
                        Children: [
                            Key: 'Value'
                            Value:
                                Type: 'UINT32'
                                UInt32Value: []
                        ,
                            Key: 'Scope'
                            Value:
                                Type: 'STRING'
                                StringValue: [
                                    'GLOBAL'
                                ]
                        ,
                            Key: 'Required'
                            Value:
                                Type: 'BOOL'
                                BoolValue: [
                                    true
                                ]
                        ,
                            Key: 'Default'
                            Value:
                                Type: 'UINT32'
                                UInt32Value: [ 5432 ]
                        ]
                    ,
                        Key: 'Catalog'
                        Children: [
                            Key: 'Value'
                            Value:
                                Type: 'STRING'
                                StringValue: []
                        ,
                            Key: 'Scope'
                            Value:
                                Type: 'STRING'
                                StringValue: [
                                    'GLOBAL'
                                ]
                        ,
                            Key: 'Required'
                            Value:
                                Type: 'BOOL'
                                BoolValue: [
                                    true
                                ]
                        ,
                            Key: 'Default'
                            Value:
                                Type: 'STRING'
                                StringValue: [ 'gpadmin' ]
                        ]
                    ,
                        Key: 'User'
                        Children: [
                            Key: 'Value'
                            Value:
                                Type: 'STRING'
                                StringValue: []
                        ,
                            Key: 'Scope'
                            Value:
                                Type: 'STRING'
                                StringValue: [
                                    'GLOBAL'
                                ]
                        ,
                            Key: 'Required'
                            Value:
                                Type: 'BOOL'
                                BoolValue: [
                                    false
                                ]
                        ]
                    ,
                        Key: 'Password'
                        Children: [
                            Key: 'Value'
                            Value:
                                Type: 'STRING'
                                StringValue: []
                        ,
                            Key: 'Scope'
                            Value:
                                Type: 'STRING'
                                StringValue: [
                                    'GLOBAL'
                                ]
                        ,
                            Key: 'Required'
                            Value:
                                Type: 'BOOL'
                                BoolValue: [
                                    false
                                ]
                        ]
                    ]
                ,
                    Key: 'SharedObjectPath'
                    Children: [
                        Key: 'Value'
                        Value:
                            Type: 'STRING'
                            StringValue: []
                    ,
                        Key: 'Scope'
                        Value:
                            Type: 'STRING'
                            StringValue: [
                                'GLOBAL'
                            ]
                    ,
                        Key: 'Required'
                        Value:
                            Type: 'BOOL'
                            BoolValue: [
                                true
                            ]
                    ]
                ]
        Convert.TemplateToOld(source).should.deep.equal(destination)
        Convert.TemplateToNew(Convert.TemplateToOld(source)).should.deep.equal(source)
