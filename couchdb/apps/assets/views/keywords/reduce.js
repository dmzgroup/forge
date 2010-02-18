function(keys, values, rereduce) {

   if (rereduce) { return sum(values); }
   else { return keys.length }
}
