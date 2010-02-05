var http = require('http'),
   url = require('url'),
   sys = require("sys");

function dump(x){sys.puts(x===undefined?'undefined':JSON.stringify(x,null,1))}
function inspect(x){sys.puts(sys.inspect(x))}

var Server = exports;

var NOT_FOUND = "Not Found!\n";

Server.notFound = function (request, response) {
  response.sendHeader(404, [["Content-Type", "text/plain"], ["Content-Length", NOT_FOUND.length]]);
  response.sendBody(NOT_FOUND);
  response.finish();
}

var routes = [];

addRoute = function (method, pattern, handler) {
   var route = {
      method: method,
      pattern: pattern,
      handler: handler
   };
   
   routes.push (route);
}

Server.get = function (pattern, handler) { return addRoute("GET", pattern, handler);};
Server.post = function (pattern, handler, format) { return addRoute("POST", pattern, handler, format); };
Server.put = function (pattern, handler, format) { return addRoute("PUT", pattern, handler, format); };
Server.del = function (pattern, handler) { return addRoute("DELETE", pattern, handler); };

var server = http.createServer (function(request, response) {
   var uri = url.parse(request.url);
   var path = uri.pathname;

sys.puts(request.method + " " + path);

   for (var ix = 0, maxRoutes = routes.length; ix < maxRoutes; ix += 1) {
      var route = routes[ix];
      if (request.method === route.method) {
         var match = path.match (route.pattern);
         if (match && match[0].length > 0) {
            match.shift ();
            inspect (match);
            match = match.map (unescape);
            inspect (match);
            match.unshift (response);
            match.unshift (request);
            route.handler.apply (null, match);
            return;
         }
      }
   }

   notFound (request, response);
});

Server.listen = function(port, host) {
  server.listen(port, host);
  sys.puts("Server at http://" + (host || "127.0.0.1") + ":" + port.toString() + "/");
};

Server.close = function() {
  server.close();
};
