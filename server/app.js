require('./config/env');

var
  sys = require ('sys'),
  path = require ('path'),
  proxy = require ('./couchproxy').create (5984, 'localhost');


// get ('/', function (request) {
//    return request.redirect ('/assets');
// });

get ('/uuids', function (request) {
   sys.p (request.parsed_url ());
   proxy.request ('/_uuids', request, request.response);
});

// Get All Assets
get ('/assets/?', function (request) {
   proxy.request ('/assets/_all_docs', request, request.response);
});

// Get Assets Attachment
get ('/assets/:docId/:attachmentId', function (request) {
   var pathname = path.join ('/assets', request.docId, request.attachmentId);
   proxy.request (pathname, request, request.response);
});

// Add an attachment to the Asset
put ('/assets/:docId/:attachmentId', function (request) {
   var pathname = path.join ('/assets', request.docId, request.attachmentId);
   proxy.request (pathname, request, request.response);
});

// Get Asset
get ('/assets/:docId', function (request) {
   var pathname = path.join ('/assets', request.docId);
   proxy.request (pathname, request, request.response);
});

// Update/Create Asset
put ('/assets/:docId', function (request) {
   var pathname = path.join ('/assets', request.docId);
   proxy.request (pathname, request, request.response);
});

// Delete Asset
// del ('/assets/:docId', function (request) {
//    var pathname = path.join ('/assets', request.docId);
//    proxy.request (pathname, request, request.response);
// });

get ('/assets/search/all/?', function (request) {
   var pathname = '/assets/_fti/test/all';
   proxy.request (pathname, request, request.response);
});
