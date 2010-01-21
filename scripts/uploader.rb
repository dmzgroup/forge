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

# def generate_hash file
#   hash_func = Digest::SHA1.new
#   open(file, 'rb') do |io|
#     counter = 0
#     while (!io.eof)
#       readBuf = io.readpartial(1024)
#       hash_func.update readBuf
#     end
#   end
#   hash_func.hexdigest
# end


class Uploader
  def initialize args={}
    @db = CouchRest.database! 'http://localhost:5984/assets'
    @db.recreate!
    @id = nil
    @dir = nil
  end
  
  def start(dir, filter='*.ive')
    @dir = dir
    Dir[File.join(@dir, filter)].each { |file|
      process_asset File.join(Dir.pwd, file)
    }
  end
  
  def process_asset file
    puts "=-=-=- Processing #{File.basename file} -=-=-="
    jsonFile = file + '.json'
    thumbDir = jsonFile + '.tdb'
    data = File.read jsonFile;
    asset = JSON.parse data;
    @id = asset['_id']
    asset['created'] = create_date_array
    asset['updated'] = create_date_array
    asset['current'] = []
    asset['thumbnails'] = {}
    response = @db.save_doc asset;
    puts "Document saved: #{@id}"
    upload_attachment file
    upload_thumbnails thumbDir
  end
  
  def upload_attachment file
    data = File.read file;
    sha1 = Digest::SHA1.hexdigest(data)
    # sha1 = generate_hash file
    name = sha1 + '.ive'
    type = 'model/x-ive'
    @doc = @db.get @id
    response = @db.put_attachment(@doc, name, data, {'content_type' => type})
    doc = @db.get @id
    puts "Attachment uploaded: #{name} for #{File.basename file}"
    doc['current'] << {'attachment' => name, 'content_type' => type}
    doc['updated'] = create_date_array
    response = @db.save_doc doc
  end
  
  def upload_thumbnails dir
    filter = '*.png'
    type = 'image/png'
    images = []
    Dir[File.join(dir, filter)].each { |file|
      data = File.read file;
      name = File.basename file
      doc = @db.get @id
      response = @db.put_attachment(doc, name, data, {'content_type' => type})
      print '.'
      images << name
    }
    puts " #{images.size} Thumbnails uploaded"
    doc = @db.get @id
    doc['thumbnails']['images'] = images
    doc['updated'] = create_date_array
    @db.save_doc doc
  end
end

if(ARGV.length == 0)
  puts "usage: uploader dir"
  exit 0
end

uploader = Uploader.new

uploader.start ARGV[0]

puts '----Done----'
