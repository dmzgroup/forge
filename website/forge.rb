require 'rubygems'
require 'sinatra'
require 'rest_client'
require 'json'
require 'haml'
require 'pp'

DEFAULT_QUERY = "type:asset"

@@forge = "http://localhost:8080"

helpers do
  def thumbnail_link(doc, options={})
    filename = doc['thumbnails']['images'][0]
    attrs = options.map{|kv| %Q|#{kv.first}="#{kv.last}"|}.join(" ")
    %Q|<img #{attrs} src="#{@@forge}/assets/#{doc['_id']}/#{filename}"|
  end
  
  def partial(page, options={})
    haml page, options.merge!(:layout => false)
  end
end

get '/' do
  data = RestClient.get "#{@@forge}/assets?limit=10"
  assetList = JSON.parse(data)['rows']
  
  @assets = assetList.inject([]) do |result, doc|
    data = RestClient.get "#{@@forge}/assets/#{doc['id']}"
    asset = JSON.parse(data)
    result << asset
  end

  haml :index
end

get '/assets/:asset' do
  data = RestClient.get "#{@@forge}/assets/#{params['asset']}"
  @asset = JSON.parse(data)
  @thumbnails = @asset['thumbnails']['images'];
  @title = @asset['name'] + ' (Asset)'
  haml :asset
end

get '/assets/search' do
#  @query = params[:q] == '' ? DEFAULT_QUERY : params[:q]
#  @sort = params[:q] == '' ? "%5Csort_date" : params[:sort]
end

# not_found do
#   haml :'404'
# end
# 
# error do
#   haml :'500'
# end
