function(doc) {

   if ((doc.type === "object") && doc.object && doc.object.type) {
      
      emit(doc.object.type, null);
   }
}
