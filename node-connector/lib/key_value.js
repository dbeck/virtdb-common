var KeyValue, log;

log = require("loglevel");

KeyValue = (function() {
  function KeyValue() {}

  KeyValue.parseJSON = function(data) {};

  KeyValue.toJSON = function(data) {
    log.debug(JSON.stringify(data));
    if (data.length === 0) {
      return {};
    }
    return KeyValue._processData(data, {});
  };

  KeyValue._processData = function(data, result) {
    var child, obj, _i, _len, _ref;
    if (data.Value != null) {
      log.debug(data);
      result[data.Key] = KeyValue._selectValue(data.Value);
      return result;
    } else {
      if (data.Children.length !== 0) {
        obj = {};
        _ref = data.Children;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          child = _ref[_i];
          obj = KeyValue._processData(child, obj);
        }
        result[data.Key] = obj;
        return result;
      }
    }
  };

  KeyValue._selectValue = function(value) {
    var newValue;
    log.debug(value);
    newValue = {};
    switch (value.Type) {
      case "STRING":
        return newValue = {
          value: value.StringValue,
          type: value.Type
        };
      case "INT32":
        return newValue = {
          value: value.Int32Value,
          type: value.Type
        };
      case "INT64":
        return newValue = {
          value: value.Int64Value,
          type: value.Type
        };
      case "UINT32":
        return newValue = {
          value: value.UInt32Value,
          type: value.Type
        };
      case "UINT64":
        return newValue = {
          value: value.UInt64Value,
          type: value.Type
        };
      case "DOUBLE":
        return newValue = {
          value: value.DoubleValue,
          type: value.Type
        };
      case "FLOAT":
        return newValue = {
          value: value.FloatValue,
          type: value.Type
        };
      case "BOOL":
        return newValue = {
          value: value.BoolValue,
          type: value.Type
        };
      case "BYTES":
        return newValue = {
          value: value.BytesValue,
          type: value.Type
        };
      default:
        return newValue = {
          value: value.StringValue,
          type: value.Type
        };
    }
  };

  return KeyValue;

})();

module.exports = KeyValue;

//# sourceMappingURL=key_value.js.map