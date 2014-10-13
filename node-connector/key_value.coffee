log = require "loglevel"

class KeyValue

    @parseJSON = (data) =>
        return @_processJSON("", data)

    @_processJSON = (key, value) =>
        obj = {}
        obj.Key ?= key
        obj.Children ?= []
        if Object.keys(value).length is 2 and value.Type? and value.Value?
            obj.Value ?= {}
            obj.Value["type"] = value.Type
            obj.Value[@_selectValue value.Type] = value.Value
        else if typeof value is "object" and value isnt null
            for key2, value2 of value
                obj.Children.push @_processJSON key2, value2
        else
            log.error "Not well formed json"
        return obj

    @toJSON = (data) =>
        if data.length is 0
            return {}
        @_processKeyValue(data, {})

    @_processKeyValue = (data, result) =>
        if data.Value?
            valueType = @_selectValue data.Value
            result[data.Key] =
                Type: data.Value.Type
                Value: data.Value[valueType]
            return result
        else
            if data.Children.length isnt 0
                obj = {}
                for child in data.Children
                    obj = @_processKeyValue child, obj
                result[data.Key] = obj
                return result

    @_selectValue = (value) =>
        switch value.Type
            when "STRING"
                return "StringValue"
            when "INT32"
                return "Int32Value"
            when "INT64"
                return "Int64Value"
            when "UINT32"
                return "UInt32Value"
            when "UINT64"
                return "UInt64Value"
            when "DOUBLE"
                return "DoubleValue"
            when "FLOAT"
                return "FloatValue"
            when "BOOL"
                return "BoolValue"
            when "BYTES"
                return "BytesValue"
            else # "DATE", "TIME", "DATETIME", "NUMERIC", "INET4", "INET6", "MAC", "GEODATA"
                return "StringValue"

module.exports = KeyValue
