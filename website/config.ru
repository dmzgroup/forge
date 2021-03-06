require 'forge'
require 'rubygems'
require 'sinatra'
# require 'rack/cache'

# use Rack::Cache,
#   :verbose     => true,
#   :metastore   => 'file:/var/cache/rack/meta',
#   :entitystore => 'file:/var/cache/rack/body'

root_dir = File.dirname(__FILE__)

set :environment, :development
set :root,        root_dir
set :app_root,    root_dir
set :app_file,    File.join(root_dir, 'forge.rb')
disable :run

run Sinatra::Application
