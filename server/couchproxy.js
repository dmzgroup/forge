var sys = require ('sys'),
   url_parse = require ('url').parse,
   http = require ('http'),
   utils = require ('./utils');

var dump = utils.dump;
var inspect = utils.inspect;

exports.create = function (port, host) {
   var port = port || 5984;
   var host = host || 'localhost';
   var proxy = new CouchProxy (port, host);
   proxy.__defineGetter__('host', function () { return host; });
   proxy.__defineGetter__('port', function () { return port; });
   return proxy;
};

var CouchProxy = exports.CouchProxy = function (couchPort, couchHost) {
   this.couchClient = http.createClient (couchPort, couchHost)
};

CouchProxy.prototype = {
   request : function (couchPath, clientRequest, clientResponse) {
      var url = url_parse (clientRequest.url);
      var path = couchPath + (url.search || '');
sys.puts ('Proxy -> ' + clientRequest.method + ": " + path);
      var couchRequest = this.couchClient.request (
         clientRequest.method, path, clientResponse.headers);
      clientRequest.addListener ('body', function (chunk) {
         couchRequest.sendBody (chunk);
      });
      clientRequest.addListener ('complete', function () {
         couchRequest.finish (function (couchResponse) {
            clientResponse.sendHeader (couchResponse.statusCode, couchResponse.headers);
            couchResponse.addListener ('body', function (chunk) {
               clientResponse.sendBody (chunk, 'binary');
            });
            couchResponse.addListener ('complete', function () {
               clientResponse.finish ();
            });
         });
      });
   }
}
