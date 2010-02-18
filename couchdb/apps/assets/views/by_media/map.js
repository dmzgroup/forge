function(doc) {

   if (doc.type == "asset") {

      emit(doc['media'], null);
   }  
}