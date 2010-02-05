var http = require ('http'),
   url_parse = require ('url').parse,
   sys = require ('sys'),
   utils = require ('./utils');

var dump = utils.dump;
var inspect = utils.inspect;

var Server = exports;

Server.not_found = function (request, response, reason, error, code) {
   var error = { "error": error || "not_found", "reason": reason || "missing" };
   var message = JSON.stringify (error);
   response.sendHeader (code || 404,
      [["Content-Type", "text/plain"], ["Content-Length", message.length]]);
   response.sendBody (message);
   response.finish ();
};

var routes = [];

addRoute = function (method, pattern, handler) {
   routes.push ({
      method: method,
      pattern: pattern,
      handler: handler,
   });
}

Server.get = function (pattern, handler) {
   return addRoute ("GET", pattern, handler);
};

Server.post = function (pattern, handler) {
   return addRoute ("POST", pattern, handler);
};

Server.put = function (pattern, handler) {
   return addRoute ("PUT", pattern, handler);
};

Server.del = function (pattern, handler) {
   return addRoute ("DELETE", pattern, handler);
};

var server = http.createServer (function (request, response) {
   var url = url_parse (request.url);
   var path = url.pathname;
sys.puts (request.method + ': ' + path);
   for (var ix = 0, maxRoutes = routes.length; ix < maxRoutes; ix += 1) {
      var route = routes[ix];
      if (request.method === route.method) {
         var match = path.match (route.pattern);
         if (match && match[0].length > 0) {
            match.shift ();
            match = match.map (unescape);
            match.unshift (response);
            match.unshift (request);
            route.handler.apply (null, match);
            return;
         }
      }
   }

   Server.not_found (request, response, 'unsupported_request');
});

Server.listen = function(port, host) {
   var port = port || 8008;
   var host = host || '127.0.0.1';
   server.listen (port, host);
   sys.puts ('Server at http://' + host + ':' + port);
};

Server.close = function() {
  server.close();
};
