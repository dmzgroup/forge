var sys = require ('sys'),
   http = require ('http'),
   url = require ('url'),
   couchproxy = require ('./couchproxy');
   server = require ('./server');
   
var PORT = 8080;
var proxy = couchproxy.create (5984, 'dmzforge');

function dump(x){sys.puts(x===undefined?'undefined':JSON.stringify(x,null,1))}
function inspect(x){sys.puts(sys.inspect(x))}

server.listen (PORT);

function index_handler (request, response) {
   response.sendHeader(404, [["Content-Type", "text/plain"], ["Content-Length", NOT_FOUND.length]]);
   response.sendBody(NOT_FOUND);
   response.finish();
};

function index_handler (request, response) {
   proxy.request ('/assets/_all_docs', request, response);
};

function asset_handler (request, response, id) {
   proxy.request ('/assets/' + id, request, response);
};

function asset_attachment_handler (request, response, id, attachment) {
   proxy.request ('/assets/' + id + '/' + attachment, request, response);
};

server.get ('^/$', index_handler);
server.get ('^/favicon.ico$', server.notFound);
server.get ('^/assets$', index_handler);
server.get ('^/assets/$', index_andler);
server.get ('^/assets/([^/]+)$', asset_handler);
server.get ('^/assets/([^/]+)/([^/]+)$', asset_attachment_handler);

