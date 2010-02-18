function(doc) {

   if (doc.type == "asset") {

      emit(doc['created'], null);
   }  
}