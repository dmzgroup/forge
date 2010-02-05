var sys = require('sys'),
   http = require('http');
   
exports.create = function (couchPort, couchHost) {
   var port = couchPort || 5984;
   var host = couchHost || 'localhost';
   var proxy = new CouchProxy (port, host);
   
   proxy.__defineGetter__('host', function() { return host; });
   proxy.__defineGetter__('port', function() { return port; });
   
   return proxy;
};

var CouchProxy = exports.CouchProxy = function (couchPort, couchHost) {
   this.couchClient = http.createClient (couchPort, couchHost)
};

CouchProxy.prototype = {
   request : function (couchPath, clientRequest, clientResponse) {
      sys.puts ('Proxy -> ' + clientRequest.method + ": " + couchPath);
      
      var couchRequest = this.couchClient.request (
         clientResponse.method,
         couchPath,
         clientResponse.headers
      );
      
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
            })
         });
      });
   }
}
