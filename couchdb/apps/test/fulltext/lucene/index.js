function (doc) {
   var ret = new Document ();

   function idx (obj) {
      for (var key in obj) {
         switch (typeof obj[key]) {
            case 'object':
               idx (obj[key]);
               break;
            case 'function':
               break;
            default:
               ret.add (obj[key]);
               break;
         }
      }
   };

   idx (doc);

   if (doc._attachments) {
      for (var i in doc._attachments) {
         ret.attachment ("attachment", i);
      }
   }

   return ret;
}
