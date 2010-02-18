function(doc) {

   if (doc.type == "asset") {

      emit(doc['creator'], null);
   }  
}