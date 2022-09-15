# cxx data formats

Collection of data format parsers written in C++.

This lib is packaged with my fmt lib for convenience.

## JSON api
### parser usage
fairly straightforward
```c++
    JSON::Parser parser(raw_json);

    // parse returns an optional containing a JSON object
    auto index = parser.parse();

    if(!index.has_value())
        fmt::fatal("could not parse json {}\n", parser.error());

    fmt::print("{}\n", index.value());
```
### object usage
thanks to some C++ fuckery the api is much like one you would see in a python json parser
```c++
    JSON::object_t json
    {
        {"yes", 10}
    };

    json["number"] = 1;
    json["null_value"] = nullptr;
    json["bool"] = true;
    json["string"] = "hello";
    json["array"] = {1, "yes", true};
    json["object"] = {{"key", 10}, {"k2", 20}};
```

### to_string
you can stringify objects and values with the JSON::to_string overloads
```cpp
    JSON::object_t json;

    json["string"] = "hello";
    json["number"] = 2;
    json["array"] = {1, "two", "three"};
    json["nested"] =
    {
        {"k1", 1},
        {"k2", 2}
    };

    fmt::print("{}", JSON::to_string(json));
    
    // output
    {
        "string": "hello",
        "number": 2,
        "array": [ 1, "two", "three" ],
        "nested": {
            "k1": 1,
            "k2": 2
        }
    }
```
the JSON::to_string function can be used on Value, array_t and object_t types

### Map
this lib comes with a custom hash table that maintains insertion order. the api is fairly similar to std::map although slightly different in a few places.