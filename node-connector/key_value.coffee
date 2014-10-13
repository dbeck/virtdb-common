log = require "loglevel"

class KeyValue

    @parseJSON = (data) =>

    @toJSON = (data) =>
        log.debug JSON.stringify data
        if data.length is 0
            return {}
        @_processData(data, {})

    @_processData = (data, result) =>
        if data.Value?
            log.debug data
            result[data.Key] = @_selectValue data.Value
            return result
        else
            if data.Children.length isnt 0
                obj = {}
                for child in data.Children
                    obj = @_processData child, obj
                result[data.Key] = obj
                return result

    @_selectValue = (value) =>
        log.debug value
        newValue = {}
        switch value.Type
            when "STRING"
                return newValue =
                    value: value.StringValue
                    type: value.Type
            when "INT32"
                return newValue =
                    value: value.Int32Value
                    type: value.Type
            when "INT64"
                return newValue =
                    value: value.Int64Value
                    type: value.Type
            when "UINT32"
                return newValue =
                    value: value.UInt32Value
                    type: value.Type
            when "UINT64"
                return newValue =
                    value: value.UInt64Value
                    type: value.Type
            when "DOUBLE"
                return newValue =
                    value: value.DoubleValue
                    type: value.Type
            when "FLOAT"
                return newValue =
                    value: value.FloatValue
                    type: value.Type
            when "BOOL"
                return newValue =
                    value: value.BoolValue
                    type: value.Type
            when "BYTES"
                return newValue =
                    value: value.BytesValue
                    type: value.Type
            else # "DATE", "TIME", "DATETIME", "NUMERIC", "INET4", "INET6", "MAC", "GEODATA"
                return newValue =
                    value: value.StringValue
                    type: value.Type


module.exports = KeyValue
