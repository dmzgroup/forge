function(doc) {

   if (doc.type == "asset") {

      emit(doc['license'], null);
   }  
}