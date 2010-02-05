var sys = require ('sys'),
   http = require ('http'),
   path = require ('path'),
   url_parse = require ('url').parse,
   couchproxy = require ('./couchproxy'),
   server = require ('./server'),
   config = require ('./config'),
   utils = require ('./utils');

var proxy = couchproxy.create (config.couchPort, config.couchHost);

server.listen (config.serverPort);

var dump = utils.dump;
var inspect = utils.inspect;

function proxy_handler (request, response) {
   var url = url_parse (request.url);
   proxy.request (url.pathname, request, response);
};

function get_uuids (request, response) {
   proxy.request ('/_uuids', request, response);
};

function get_all_assets (request, response) {
   proxy.request ('/assets/_all_docs', request, response);
};

// function get_asset_attachment (request, response, id, attachment) {
//    proxy.request (path.join ('/assets', id, attachment), request, response);
// };

server.get ('^/$', proxy_handler);
server.get ('^/favicon.ico$', server.not_found);
server.get ('^/uuids', get_uuids);
server.get ('^/assets$', get_all_assets);
server.get ('^/assets/$', get_all_assets);
server.get ('^/assets2/([^/]+)$', proxy_handler);
server.put ('^/assets2/([^/]+)$', proxy_handler);
server.del ('^/assets2/([^/]+)$', proxy_handler);
server.get ('^/assets/([^/]+)/([^/]+)$', proxy_handler);
//server.get ('^/assets/([^/]+)/([^/]+)$', get_asset_attachment);
