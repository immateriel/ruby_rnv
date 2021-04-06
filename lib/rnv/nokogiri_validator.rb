require 'nokogiri'
require 'rnv/rnv'

module RNV
  # @!visibility private
  class NokogiriSaxDocument < Nokogiri::XML::SAX::Document
    # @param [RNV::Validator] validator
    def initialize(validator)
      @validator = validator
    end

    def start_element_namespace(name, attrs = [], prefix = nil, uri = nil, ns = nil)
      tag_attrs = attrs.map { |attr| [attr.uri ? "#{attr.uri}:#{attr.localname}" : attr.localname, attr.value] }
      tag_name = uri ? "#{uri}:#{name}" : name
      @validator.start_tag(tag_name, tag_attrs.flatten)
    end

    def end_element_namespace(name, prefix = nil, uri = nil)
      tag_name = uri ? "#{uri}:#{name}" : name
      @validator.end_tag(tag_name)
    end

    def characters str
      @validator.characters(str)
    end
  end

  class NokogiriValidator
    # @param [String] schema RNC schema filename
    # @param [String] xml XML filename
    # @return [RNV::Validator]
    def self.validate(schema, xml, trace = true)
      validator = RNV::Validator.new
      validator.load_file(schema)

      rnv_doc = NokogiriSaxDocument.new(validator)

      if trace
        parser = Nokogiri::XML::SAX::PushParser.new(rnv_doc)
        parser.replace_entities = true
        validator.last_line = 0
        validator.last_col = 0
        File.open(xml).each do |line|
          validator.last_line += 1
          validator.last_col = 0
          line.each_char do |c|
            validator.last_col += 1
            parser << c
          end
        end
        parser.finish
      else
        parser = Nokogiri::XML::SAX::Parser.new(rnv_doc)
        parser.parse(File.open(xml))
      end

      validator
    end
  end
end