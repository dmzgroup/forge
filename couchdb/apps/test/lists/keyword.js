function(head, req) {

   provides("html", function () {

      var row;

      send("<!DOCTYPE html><html><body>");

      while (row = getRow ()) {

         send('<a href="/assets/_design/test/_show/asset/' + row.doc._id + '">');
         send("<br />" + row.doc.name + "<br />");
         send('<img src="/assets/' + row.doc._id + '/' + row.doc.thumbnails.images[0] + '" width="200"/>');
         send('</a>');
      }

      return "</body></html>";
   });
};
