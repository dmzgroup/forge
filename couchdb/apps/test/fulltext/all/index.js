function (doc) {
   
   var ret = new Document ();
   
   // function idx (obj) {
   //    var ix;
   //    for (var key in obj) {
   //       switch (typeof obj[key]) {
   //          case 'object':
   //             if (key == 'keywords') {
   //                for (ix in obj[key]) {
   //                   ret.add (obj[key][ix], { 'field':'keyword' });
   //                }
   //             }
   //             else if (key == 'current') {
   //                for (ix in obj[key]) {
   //                   var type = ix.split ('/');
   //                   ret.add (type[0], { 'field':'mime_type' });
   //                   ret.add (type[1], { 'field':'mime_type' });
   //                }
   //             }
   //             else if (key == 'original_name') {
   //                for (ix in obj[key]) {
   //                   ret.add (obj[key][ix], { 'field':'original_name' });
   //                }
   //             }
   //             else if (key == 'created') {
   //                break;
   //             }
   //             else if (key == 'updated') {
   //                break;
   //             }
   //             else if (key == 'thumbnails') {
   //                break;
   //             }
   //             else {
   //                idx (obj[key]);
   //             }
   //             break;
   //          case 'function':
   //             break;
   //          default:
   //             ret.add (obj[key]);
   //             ret.add (obj[key], { 'field' : key });
   //             break;
   //       }
   //    }
   // }
   
   if (doc['type'] == 'asset') {
   
      // idx (doc);
      var ix;
      
      ret.add (doc.brief, { 'field' : 'brief' });
      ret.add (doc.details, { 'field' : 'details' });
      ret.add (doc.name, { 'field' : 'name' });
      
      ret.add (doc.media, { 'field' : 'media' });
      ret.add (doc.creator, { 'field' : 'creator' });
      ret.add (doc.license, { 'field' : 'license' })

      ret.add ((doc.keywords || []).join (', '), { 'field' : 'keyword' });
      
      for (ix in doc.current) {
         ret.add (ix, { 'field' : 'mime_type' , 'store':'yes'});
         // ret.add (ix.split ('/').join (', '), { 'field' : 'mime_type' , 'store':'yes'});
         // var type = ix.split ('/');
         // ret.add (type[0], { 'field':'mime_type' });
         // ret.add (type[1], { 'field':'mime_type', 'index':'not_analyzed' });
      }
      
      for (ix in doc.original_name) {
         ret.add (doc.original_name[ix], { 'field':'original_name' });
      }
      
      // if (doc._attachments) {
      //    for (ix in doc._attachments) {
      //       ret.attachment ("attachment", ix);
      //    }
      // }
      
      return ret;
   }
}
