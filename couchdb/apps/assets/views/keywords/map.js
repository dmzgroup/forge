function (doc) {

   if (doc.type == 'asset' && doc.keywords) {
      
      doc.keywords.forEach (function (keyword) {
         
         emit (keyword, 1);
      });
   }
}