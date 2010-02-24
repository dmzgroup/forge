require 'rubygems'
require 'sinatra'
require 'rest_client'
require 'json'
require 'haml'
require 'pp'

@@forge = "http://localhost:8080"

helpers do
  def thumbnail_link(doc, options={})
    filename = doc['thumbnails']['images'][0]
    attrs = options.map{|kv| %Q|#{kv.first}="#{kv.last}"|}.join(" ")
    %Q|<img #{attrs} src="/images/#{doc['_id']}/#{filename}"|
  end
  
  def partial(page, options={})
    haml page, options.merge!(:layout => false)
  end
end

get '/' do
  data = RestClient.get "#{@@forge}/assets?limit=16&descending=true"
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
  # etag(@asset['_rev'])
  @thumbnails = @asset['thumbnails']['images'];
  @title = @asset['name']
  haml :asset
end

get '/search' do
  limit = 25;
  query = params[:q] == '' ? "type:asset" : params[:q]
  sort = params[:q] == '' ? "%5Csort_date" : params[:sort]
  
  page = params[:page].to_i
  skip = (page < 2) ? 0 : ((page - 1) * limit)
  
  url = "#{@@forge}/assets/search?limit=#{limit}" +
    "&q=#{Rack::Utils.escape(query)}" +
    "&skip=#{skip}"

  if sort =~ /\w/
    order = params[:order] =~ /desc/ ? "%5C" : ""
    url += "&sort=#{order}#{sort}"
  end
  
  begin
    pp(url)
    data = RestClient.get url
    results = JSON.parse(data)
  rescue
    query = "type:asset"
    redirect("/assets/search?q=#{query}")
  end
  
  @assets = results['rows'].inject([]) do |result, doc|
    data = RestClient.get "#{@@forge}/assets/#{doc['id']}"
    asset = JSON.parse(data)
    result << asset
  end

  @title = "Search Results"
  haml results['total_rows'] == 0 ? :no_results : :search
end

get '/images/:asset/:image.:ext' do
  content_type "image/#{params[:ext]}"
  begin
    RestClient.get "#{@@forge}/assets/#{params[:asset]}/#{params[:image]}.#{params[:ext]}"
  rescue
    404
  end
end

not_found do
  haml :'404'
end

error do
  haml :'500'
end
