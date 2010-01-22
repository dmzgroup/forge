#!/usr/bin/env ruby

require 'digest/sha1'
require 'json'
require 'CouchRest'
require 'pp'

def show obj
  puts obj.inspect
  puts
end

def create_date_array
  t = Time.now.utc
  [t.year, t.month, t.day, t.hour, t.min, t.sec]
end


class Uploader
  def initialize args={}
    @db = CouchRest.database! 'http://localhost:5984/assets'
    @db.recreate!
    @uuid = []
  end
  
  def start(dir, filter='*.ive')
    @dir = dir
    Dir[File.join(@dir, filter)].each { |file| process_asset(File.join(Dir.pwd, file)) }
  end
  
  def save_doc
    @doc['updated'] = create_date_array
    response = @db.save_doc @doc
    @doc['_rev'] = response['rev']
    response['ok']
  end
  
  def put_attachment(name, file, type)
    data = File.read file
    response = @db.put_attachment(@doc, name, data, {'content_type' => type})
    @doc['_rev'] = response['rev']
    response['ok']
  end
  
  def process_asset file
    jsonFile = file + '.json'
    thumbDir = jsonFile + '.tdb'
    if File.exists? jsonFile
      raw = File.read jsonFile
      data = JSON.parse raw
      if !data['ignore']
        puts "[#{@uuid.size+1}] #{File.basename file} => #{data['_id']}"
        @doc = CouchRest::Document.new data
        @doc.database = @db
        @id = @doc['_id']
        @uuid << @id
        @doc['created'] = create_date_array
        if save_doc
          upload_attachment file
          upload_thumbnails thumbDir
        end
      end
    end
  end
  
  def upload_attachment file
    type = 'model/x-ive'
    current = @doc['current']
    name = current[type]
    put_attachment(name, file, type)
  end
  
  def upload_thumbnails dir
    type = 'image/png'
    images = @doc['thumbnails']['images']
    images.each do |image|
      file = File.join(dir, image)
      put_attachment(image, file, type)
    end
  end
end

if(ARGV.length == 0)
  puts "usage: uploader dir"
  exit 0
end

uploader = Uploader.new

uploader.start ARGV[0]

puts '----Done----'
