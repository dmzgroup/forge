require 'forge'
require 'rubygems'
require 'sinatra'

root_dir = File.dirname(__FILE__)

set :environment, :development
set :root,        root_dir
set :app_root,    root_dir
set :app_file,    File.join(root_dir, 'forge.rb')
disable :run

run Sinatra::Application
