# cxx data formats

I got bored and wrote a json parser in C++ so i thought i make a repository for any future parsers i write for stuff similar to json.

I packaged it with my fmt lib for convenience.

## JSON api
### parser usage
fairly straightforward
```c++
    JSON::Parser parser(raw_json);

    // parse returns an optional containing a JSON object
    auto index = parser.parse();

    if(!index.has_value())
    {
        fmt::print("could not parse json {}\n", parser.error());
        return;
    }

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