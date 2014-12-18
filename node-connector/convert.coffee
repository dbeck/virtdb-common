class Convert
    setValueArray = (type, defaultValue) =>
        value = {}
        value.Type = type
        switch type
            when "INT32"
                value.Int32Value = []
                value.Int32Value = [ defaultValue ] if defaultValue?
            when "INT64"
                value.Int64Value = []
                value.Int64Value = [ defaultValue ] if defaultValue?
            when "UINT32"
                value.UInt32Value = []
                value.UInt32Value = [ defaultValue ] if defaultValue?
            when "UINT64"
                value.UInt64Value = []
                value.UInt64Value = [ defaultValue ] if defaultValue?
            when "FLOAT"
                value.FloatValue = []
                value.FloatValue = [ defaultValue ] if defaultValue?
            when "DOUBLE"
                value.DoubleValue = []
                value.DoubleValue = [ defaultValue ] if defaultValue?
            when "BOOL"
                value.BoolValue = []
                value.BoolValue = [ defaultValue ] if defaultValue?
            when "BYTES"
                value.BytesValue = []
                value.BytesValue = [ defaultValue ] if defaultValue?
            else
                value.StringValue = []
                value.StringValue = [ defaultValue ] if defaultValue?
        return value

    addValue = (object, config) ->
        if config.Type?
            index = object.Children.push({}) - 1
            object.Children[index].Key = 'Value'
            object.Children[index].Value = setValueArray config.Type

    addRequired = (object, config) ->
        if config.Required?
            index = object.Children.push({}) - 1
            object.Children[index].Key = 'Required'
            object.Children[index].Value = {}
            object.Children[index].Value.Type = 'BOOL'
            object.Children[index].Value.BoolValue = [ config.Required ]

    addDefault = (object, config) ->
        if config.Default? and config.Type?
            index = object.Children.push({}) - 1
            object.Children[index].Key = 'Default'
            object.Children[index].Value = {}
            object.Children[index].Value.Type = config.Type
            object.Children[index].Value = setValueArray config.Type, config.Default

    addScope = (object, config, rootScope) ->
        if config.Type?
            config.Scope ?= rootScope
            index = object.Children.push({}) - 1
            object.Children[index].Key = 'Scope'
            object.Children[index].Value = {}
            object.Children[index].Value.Type = 'STRING'
            object.Children[index].Value.StringValue = [ config.Scope ]

    addNodeToOld = (destination, source, rootScope) ->
        for config in source
            object = {}
            object.Key = config.VariableName
            object.Children = []
            addValue(object, config)
            addScope(object, config, rootScope)
            addRequired(object, config)
            addDefault(object, config)
            if config.Children?
                object.Children = []
                addNodeToOld object.Children, config.Children, config.Scope
            destination.push object

    @TemplateToOld: (source) =>
        ret = {}
        ret.Name = source.AppName
        if source.Config?
            ret.ConfigData = []
            ret.ConfigData.push
                Key: ""
                Children: []
            root = ret.ConfigData[0].Children
            addNodeToOld root, source.Config
        return ret

    getValue = (value) ->
        if not value?
            return null
        switch value.Type
            when "INT32"
                return value.Int32Value[0]
            when "INT64"
                return value.Int64Value[0]
            when "UINT32"
                return value.UInt32Value[0]
            when "UINT64"
                return value.UInt64Value[0]
            when "FLOAT"
                return value.FloatValue[0]
            when "DOUBLE"
                return value.DoubleValue[0]
            when "BOOL"
                return value.BoolValue[0]
            when "BYTES"
                return value.BytesValue[0]
            else
                return value.StringValue[0]

    getScope = (nodes) ->
        for node in nodes
            if node.Key == 'Scope'
                return getValue node.Value
        return null

    getType = (nodes) ->
        for node in nodes
            if node.Key == 'Value'
                return node.Value.Type
        return null

    getRequired = (nodes) ->
        for node in nodes
            if node.Key == 'Required'
                return node.Value.BoolValue[0]
        return null

    getDefault = (nodes) ->
        for node in nodes
            if node.Key == 'Default'
                return getValue node.Value
        return null

    addNodeToNew = (destination, source) ->
        if not source?
            return null
        isCommonScope = true
        commonScope = null
        for node in source
            object = {}
            object.VariableName = node.Key
            type = getType node.Children
            if type?
                object.Type = type
            required = getRequired node.Children
            if required?
                object.Required = required
            defaultValue = getDefault node.Children
            if defaultValue?
                object.Default = defaultValue
            if node.Children[0]?.Children?
                object.Children = []
                childrenScope = addNodeToNew object.Children, node.Children
                if childrenScope?
                    object.Scope = childrenScope
                    commonScope ?= childrenScope
                    if commonScope != childrenScope
                        isCommonScope = false
            scope = getScope node.Children
            if scope?
                object.Scope = scope
                commonScope ?= scope
                if commonScope != scope
                    isCommonScope = false
            destination.push object
        if isCommonScope
            return commonScope
        else
            return null

    @TemplateToNew: (source) =>
        ret = {}
        ret.AppName = source.Name
        ret.Config = []
        addNodeToNew ret.Config, source.ConfigData[0].Children
        return ret

    @ToNew: (source) =>
        ret = {}
        ret.AppName = source.Name
        ret.Data = []
        for item in source.ConfigData
            for child in item.Children
                data = {}
                data.Location = []
                data.Location.push item.Key
                data.Location.push child.Key
                data.Data = {}
                data.Data = child.Value
                ret.Data.push data
        return ret

    addProperty = (to, list, index, value) ->
        to[list[index]] ?= {}
        if list.length - 1 == index
            to[list[index]] = getValue value
            return
        addProperty to[list[index]], list, index + 1, value

    @ToObject: (source) =>
        ret = {}
        for item in source.Data
            addProperty ret, item.Location, 0, item.Data
        return ret

module.exports = Convert
