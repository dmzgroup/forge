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
    @id = nil
    @dir = nil
    @doc = nil
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
  
  def put_attachment(name, data, options)
    response = @db.put_attachment(@doc, name, data, options)
    @doc = @db.get @id
    response['ok']
  end
  
  def process_asset file
    puts "=-=-=- Processing #{File.basename file} -=-=-="
    jsonFile = file + '.json'
    thumbDir = jsonFile + '.tdb'
    raw = File.read jsonFile
    data = JSON.parse raw
    @doc = CouchRest::Document.new data
    @doc.database = @db
    @id = @doc['_id']
    @doc['created'] = create_date_array
    @doc['current'] = []
    @doc['thumbnails'] = {}
    if save_doc
      puts "Document saved: #{@id}"
      upload_attachment file
      upload_thumbnails thumbDir
    end
  end
  
  def upload_attachment file
    data = File.read file;
    sha1 = Digest::SHA1.hexdigest data
    name = sha1 + '.ive'
    type = 'model/x-ive'
    if put_attachment(name, data, {'content_type' => type})
      puts "Attachment uploaded: #{name}"
      @doc['current'] << {'attachment' => name, 'content_type' => type}
      save_doc
    end
  end
  
  def upload_thumbnails dir
    filter = '*.png'
    type = 'image/png'
    images = []
    Dir[File.join(dir, filter)].each { |file|
      data = File.read file
      name = File.basename file
      if put_attachment(name, data, {'content_type' => type})
        print '.'
        images << name
      end
    }
    puts " #{images.size} Thumbnails uploaded"
    @doc['thumbnails']['images'] = images
    @doc['updated'] = create_date_array
    save_doc
  end
end

if(ARGV.length == 0)
  puts "usage: uploader dir"
  exit 0
end

uploader = Uploader.new

uploader.start ARGV[0]

puts '----Done----'
