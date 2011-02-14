function(doc, req) {
   if (!req.query)
   if (doc.type) {
      if (doc.type == req.query.name) {
         return true;
      }
   }
   
   return false;
}
