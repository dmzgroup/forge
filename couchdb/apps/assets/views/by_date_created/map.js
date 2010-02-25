function(doc) {

   if (doc.type == "asset") {

      emit(doc['created_at'], null);
   }  
}