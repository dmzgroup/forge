require 'rubygems'
require 'sinatra'
require 'rest_client'
require 'json'

DB = 'http://localhost:8080/assets'
#DB = 'http://dmzforge:5986/assets'

get '/hello' do
  'Hello World!'
end

get '/assets/:asset' do
  data = RestClient.get "#{DB}/#{params[:asset]}"
  result = JSON.parse(data)
  %Q{
<h1>#{result['name']}</h1>
<h3>#{result['brief']}</h3>
<p>#{result['details']}</p>
}
end
