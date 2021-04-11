#!/usr/bin/ruby

require 'rnv'
require 'ruby-prof'
require 'pp'

if ARGV.length < 2
  puts "usage: rnv.rb schema.rnc file.xml"
else
  schema = ARGV[0]
  xml = ARGV[1]

  # RubyProf.start

  validator = RNV::Validator.new
  validator.load_schema_from_file(schema)
  validator.errors.each do |err|
    puts "#{xml}:#{err.to_s}"
  end

  validator.parse_file(xml)

  # result = RubyProf.stop

  # printer = RubyProf::FlatPrinter.new(result)
  # printer.print(STDOUT, min_percent: 1)

  #pp validator.errors
  validator.errors.each do |err|
    puts "#{xml}:#{err.to_s}"
  end

  if validator.document.valid?
    puts "#{xml} is valid"
  else
    puts "#{xml} is invalid"
  end
end
