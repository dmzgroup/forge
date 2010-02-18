function(doc) {

   if (doc.type == "asset") {

      emit(doc['updated'], null);
   }  
}