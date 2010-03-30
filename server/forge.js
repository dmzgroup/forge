var sys = require ('sys'),
    http = require ('http'),
    url = require ('url'),
    path = require ('path'),
    querystring = require ('querystring'),
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
         
         request.proxy = {
            path: uri.pathname || '/',
            params: uri.query || {},
         };
         
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
            
            response.write (content);
            response.close ();
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


var Proxy = function (port, host, db) {
   var self = this;
   this._port = port || 5984;
   this._host = host || '127.0.0.1';
   this.db = db || 'forge';
   this.client = http.createClient (this._port, this._host);
   this._log = http.createClient (this._port, this._host);
   
   this.request = function (clientRequest, clientResponse) {
      var search = '?' + querystring.stringify (clientRequest.proxy.params);
      var path = clientRequest.proxy.path + search;

sys.puts ('Proxy ' + clientRequest.method + ': ' + Date ());
sys.puts ('  ' + clientRequest.uri.path + " -> " + clientRequest.proxy.path);
//sys.puts ('  ' + clientRequest.uri.search + " -> " + search);

      var request = this.client.request (clientRequest.method, path, clientRequest.headers);
      
      clientRequest.addListener ('data', function (chunk) {
         request.write (chunk);
      });
      
      clientRequest.addListener ('end', function () {
         request.addListener ('response', function (response) {
            
            clientResponse.sendHeader (response.statusCode, response.headers);
            response.addListener ('data', function (chunk) {
               clientResponse.write (chunk, 'binary');
            });
            response.addListener ('end', function () {
               clientResponse.close ();
            });
         });
         request.close ();
      });
   };
};


var Forge = function (config) {
   var self = this;
   this.config = config;
   this.proxy = new Proxy (this.config.couchdb.port, this.config.couchdb.host, this.config.couchdb.db);
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
forge.get ('^/assets/search/?$', function (asset) {
   this.request.proxy.path = path.join ('/', this.proxy.db, '_fti/assets/all');
   // if (!this.request.proxy.params.q) {
   //    this.request.proxy.params.q = 'type:assets';
   // }
   this.proxy.request (this.request, this.response);
});

// Get All Assets
forge.get ('^/assets/?$', function () {
   this.request.proxy.path = path.join ('/', this.proxy.db, '_design/assets/_view/by_date');
   //if (!this.request.proxy.params.descending) {
      //this.request.proxy.params.descending = 'true';
   //}
   //this.request.proxy.path = '/' + this.proxy.db + '/_fti/assets/all';
   // this.request.proxy.params.q = 'type:assets';
   this.proxy.request (this.request, this.response);
});

// Get Asset Attachemnt
forge.get ('^/assets/([^/]*)/([^/]*)', function (asset, attachment) {
   this.request.proxy.path = path.join ('/', this.proxy.db, asset, attachment);
   this.proxy.request (this.request, this.response);
});

// Get Asset
forge.get ('^/assets/([^/]*)', function (asset) {
   this.request.proxy.path = path.join ('/', this.proxy.db, asset);
   this.proxy.request (this.request, this.response);
});

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
