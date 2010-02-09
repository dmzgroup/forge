require('picard')
 
picard.env = {
 root: __filename.replace(/\/config\/env.js$/, ''),
 server: 'DMZ Forge v0.1',
 mode: 'production',
 port: 9900,
 public_dir: '/public',
 views: '/views'
}
 
picard.start ()