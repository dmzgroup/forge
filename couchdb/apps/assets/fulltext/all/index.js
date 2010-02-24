function (rec) {
   
   if (rec.type == 'asset') {

      var doc = new Document ();
      var ix;
      
      doc.add (rec.brief);
      doc.add (rec.brief, {'field' : 'brief'});
      
      doc.add (rec.details);
      doc.add (rec.details, {'field' : 'details'});
      
      doc.add (rec.name);
      doc.add (rec.name, {'field' : 'name'});
      
      doc.add (rec.media, {'field' : 'media'});
      doc.add (rec.creator, {'field' : 'creator'});
      doc.add (rec.license, {'field' : 'license'});

      // doc.add ((rec.keywords || []).join (', '));
      // doc.add ((rec.keywords || []).join (', '), {'field' : 'keyword'});
      
      for (ix in rec.keywords) {
         doc.add (rec.keywords[ix]);
         doc.add (rec.keywords[ix], {'field' : 'keyword'});
      }
      
      for (ix in rec.current) {
         doc.add (ix);
         doc.add (ix, {'field':'mime_type'});
      }
      
      for (ix in rec.original_name) {
         doc.add (rec.original_name[ix], {'field':'original_name'});
      }
      
      // if (rec._attachments) {
      //    for (ix in rec._attachments) {
      //       doc.attachment ("attachment", ix);
      //    }
      // }
      
      doc.add (rec.type, {'field':'type'});
      
      return doc;
   }
   else { return null; }
}
