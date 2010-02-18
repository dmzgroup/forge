var sys = require ('sys'),
    http = require ('http'),
    url = require ('url'),
    path = require ('path'),
    config = require ('./config');

var Server = function (port, host, proxy) {
   var self = this;
   this._port = port || 8080;
   this._host = host || '127.0.0.1';
   this._proxy = proxy;
   this._routes = [];
   
   this.get = function (path, handler) { self._map ('GET', path, handler); };
   this.put = function (path, handler) { self._map ('PUT', path, handler); };
   this.post = function (path, handler) { self._map ('POST', path, handler); };
   this.del = function (path, handler) { self._map ('DELETE', path, handler); };

   this._map = function (method, path, handler) {
      var rpath = new RegExp (path);
      this._routes.push ({
         method: method,
         path: rpath,
         handler: handler,
      });
   };
   
   this._not_found = function () {
      this.response.reply (404, {
         error: 'not_found',
         reason: 'missing',
         path: this.request.uri.path,
      });
   };
   
   this.start = function () {
      http.createServer (function (request, response) {
         var handler = self._not_found;
         var uri = url.parse (request.url, true);
         var args = [];
         var ix;
         var route;
         
         request.headers['accept'] = 'application/json';

         request.uri = {
            path: uri.pathname || '/',
            params: uri.query || {},
            search: uri.search || '',
         };
         
         request.proxy = request.uri;
         
         for (ix = 0; ix < self._routes.length; ix++) {
            route = self._routes[ix];
            if (route.method === request.method && route.path.test (request.uri.path)) {
               args = route.path.exec (request.uri.path);
               handler = route.handler;
               break;
            }
         }
         
         response.reply = function (code, data) {
            var content = JSON.stringify (data);
            response.sendHeader (code, {
               'content-type': 'application/json',
               'content-length': content.length,
            });
            
            response.sendBody (content);
            response.finish ();
         };
         
         try {
            handler.apply ({
                  request: request,
                  response: response,
                  proxy: self._proxy,
               },
               args.slice (1));
         }
         catch (e) {
            response.reply (500, {
                error: e.name,
                reason: e.message,
            });
            sys.debug (e.stack);
         }
      }).listen (this._port, this._host);
      sys.puts ('Server running at ' + this._host + ':' + this._port);
   };
};


var Proxy = function (port, host) {
   var self = this;
   this._port = port || 5984;
   this._host = host || '127.0.0.1';
   this.client = http.createClient (this._port, this._host)
   
   this.request = function (clientRequest, clientResponse) {
      var path = clientRequest.proxy.path + clientRequest.proxy.search;
      
sys.puts ('Proxy ' + clientRequest.method + ': ' + clientRequest.uri.path + " -> " + clientRequest.proxy.path);

      var request = this.client.request (clientRequest.method, path, clientRequest.headers);
         
      clientRequest.addListener ('body', function (chunk) {
         request.sendBody (chunk);
      });
      
      clientRequest.addListener ('complete', function () {
         request.finish (function (response) {
            
            clientResponse.sendHeader (response.statusCode, response.headers);
            response.addListener ('body', function (chunk) {
               clientResponse.sendBody (chunk, 'binary');
            });
            response.addListener ('complete', function () {
               clientResponse.finish ();
            });
         });
      });
   }
};


var Forge = function (config) {
   var self = this;
   this.config = config;
   this.proxy = new Proxy (this.config.couchdb.port, this.config.couchdb.host);
   this.server = new Server (this.config.server.port, this.config.server.host, this.proxy);
   
   this.get = this.server.get;
   this.put = this.server.put;
   this.post = this.server.post;
   this.del = this.server.del;
   
   this.start = function () {
      this.server.start ();
   };
};


var forge = new Forge (config);

// Search Assets
// forge.get ('^/assets/search/?$', function (asset) {
//    sys.p (this.request.headers);
//    this.request.proxy.path = '/assets/_fti/test/all';
//    this.proxy.request (this.request, this.response);
// });

// Get All Assets
// forge.get ('^/assets/?$', function () {
//    sys.puts ('assets_all_docs')
//    this.request.proxy.path = '/assets/_all_docs';
//    this.proxy.request (this.request, this.response);
// });

// Get Asset Attachemnt
// forge.get ('^/assets/([^/]*)/([^/]*)', function (asset, attachment) {
//    sys.puts ('get asset attachment: ' + asset);
//    this.proxy.request (this.request, this.response);
// });

// Get Asset
// forge.get ('^/assets/(\d+)', function (year, month, day) {
// sys.puts (year + '-' + month + '-' + day);
//    this.request.proxy.path = '/assets/_design/test/_view/by_date_short';
//    this.proxy.request (this.request, this.response);
// });

// forge.get ('^/assets/([^/]*)', function (asset) {
//    sys.puts ('get asset : ' + asset);
//    this.proxy.request (this.request, this.response);
// });


// Update/Create Asset
forge.get ('^/assets/([^/]*)', function (asset) {
   sys.puts ('get asset: ' + asset);
   this.proxy.request (this.request, this.response);
});

// forge.del ('^/assets/([^/]*)', function (asset) {
//    this.response.reply (403, { error: 'forbidden', reason: 'deleting and asssets is not permitted'});
// });


// Get new UUID's
forge.get ('^/uuids/?$', function () {
   this.request.proxy.path = '/_uuids';
   this.proxy.request (this.request, this.response);
});

forge.get ('^/$', function () {
   this.response.reply (200, {
      dmzforge: 'Welcome',
      version: '0',
   });
});

forge.start ();
