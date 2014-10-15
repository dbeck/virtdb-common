var KeyValue, log;

log = require("loglevel");

KeyValue = (function() {
  function KeyValue() {}

  KeyValue.parseJSON = function(data) {
    return KeyValue._processJSON("", data);
  };

  KeyValue._processJSON = function(key, value) {
    var key2, obj, value2;
    obj = {};
    if (obj.Key == null) {
      obj.Key = key;
    }
    if (obj.Children == null) {
      obj.Children = [];
    }
    if (Object.keys(value).length === 2 && (value.Type != null) && (value.Value != null)) {
      if (obj.Value == null) {
        obj.Value = {};
      }
      obj.Value["type"] = value.Type;
      obj.Value[KeyValue._selectValue(value.Type)] = value.Value;
    } else if (typeof value === "object" && value !== null) {
      for (key2 in value) {
        value2 = value[key2];
        obj.Children.push(KeyValue._processJSON(key2, value2));
      }
    } else {
      log.error("Not well formed json");
    }
    return obj;
  };

  KeyValue.toJSON = function(data) {
    if (data.length === 0) {
      return {};
    }
    return KeyValue._processKeyValue(data, {});
  };

  KeyValue._processKeyValue = function(data, result) {
    var child, obj, valueType, _i, _len, _ref;
    if (data.Value != null) {
      valueType = KeyValue._selectValue(data.Value);
      result[data.Key] = {
        Type: data.Value.Type,
        Value: data.Value[valueType]
      };
      return result;
    } else {
      if (data.Children.length !== 0) {
        obj = {};
        _ref = data.Children;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          child = _ref[_i];
          obj = KeyValue._processKeyValue(child, obj);
        }
        result[data.Key] = obj;
        return result;
      }
    }
  };

  KeyValue._selectValue = function(value) {
    switch (value.Type) {
      case "STRING":
        return "StringValue";
      case "INT32":
        return "Int32Value";
      case "INT64":
        return "Int64Value";
      case "UINT32":
        return "UInt32Value";
      case "UINT64":
        return "UInt64Value";
      case "DOUBLE":
        return "DoubleValue";
      case "FLOAT":
        return "FloatValue";
      case "BOOL":
        return "BoolValue";
      case "BYTES":
        return "BytesValue";
      default:
        return "StringValue";
    }
  };

  return KeyValue;

})();

module.exports = KeyValue;

//# sourceMappingURL=key_value.js.map