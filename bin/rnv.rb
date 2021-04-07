#!/usr/bin/ruby

require 'rnv'
require 'pp'

if ARGV.length < 2
  puts "usage: rnv.rb schema.rnc file.xml"
else
  schema = ARGV[0]
  xml = ARGV[1]
  validator = RNV::Validator.new
  validator.load_schema_from_file(schema)
  validator.parse_file(xml)

  #pp validator.errors
  validator.errors.each do |err|
    puts "#{xml}:#{err.to_s}"
  end

  if validator.valid?
    puts "#{xml} is valid"
  else
    puts "#{xml} is invalid"
  end
end
