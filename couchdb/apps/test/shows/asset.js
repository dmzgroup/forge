function(doc, req) {  

   // !code vendor/couchapp/template.js
   // !code vendor/couchapp/path.js
   // !json templates.asset

//if (doc) { log(doc.name) } else { log("no doc!") }
//if (doc) { log (doc); } else { log ("doc is not defined."); }
log(assetPath("foo"));
log(showPath("foo"));
log(listPath("foo"));
//log (template(templates.asset, doc));
//   return "<DOCTYPE html><html><body>HELLO!</body></html>"; //template(templates.asset, doc);
   return template(templates.asset, doc);
}
