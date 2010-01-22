function(doc) {

   if (doc.type == "asset") {

      if(doc.keywords.length > 0) {

         for(var idx in doc.keywords) {

            emit(doc.keywords[idx], null);
         }
      }
   }  
}

