#include "middleware/cgi_handler.hpp"
#include "middleware/static.hpp"
#include "server.hpp"

map<std::string, std::string> parse_config(std::string filename) {
    char key[128], value[128], temp[256];
    map<std::string, std::string> result;
    ifstream config("server.cfg");
    while (config >> temp) {
        sscanf(temp, "%[^=]=%s", key, value);
        result[key] = value;
        cout << key << " : " << value << endl;
    }
    return result;
}

int main() {
    auto cfg = parse_config("server.cfg");

    Server app;

    app.Use(Static(cfg["STATIC_PATH"]));

    app.Use(CGI_Handler());

    app.Use("/api/v1/demo", [](Request<> &res, Response &req) {
        req.status(200).body("<p>Good</p>");
    });

    app.Listen(cfg["PORT"], []() -> void {
        cout << "Server started" << endl;
    });
}