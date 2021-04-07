require 'nokogiri'
require 'rnv/rnv'

module RNV
  # @!visibility private
  class NokogiriSaxDocument < Nokogiri::XML::SAX::Document
    # @param [RNV::Document] document
    def initialize(document)
      @document = document
    end

    def start_element_namespace(name, attrs = [], prefix = nil, uri = nil, ns = nil)
      tag_attrs = attrs.map { |attr| [attr.uri ? "#{attr.uri}:#{attr.localname}" : attr.localname, attr.value] }
      tag_name = uri ? "#{uri}:#{name}" : name
      @document.start_tag(tag_name, tag_attrs.flatten)
    end

    def end_element_namespace(name, prefix = nil, uri = nil)
      tag_name = uri ? "#{uri}:#{name}" : name
      @document.end_tag(tag_name)
    end

    def characters str
      @document.characters(str)
    end
  end

  class Validator
    # @return [RNV::Document]
    attr_accessor :document
    def initialize
      @document = RNV::Document.new
    end

    # errors from document
    # @return [Array<RNV::Error>]
    def errors
      @document.errors
    end

    # @param [String] str RNC schema buffer
    def load_schema_from_string(str)
      @document.load_string(str)
    end

    # @param [String] file RNC schema filename
    def load_schema_from_file(file)
      @document.load_file(file)
    end

    # parse and validate buffer
    # @param [String] str XML buffer to parse
    # @return [Boolean] true if valid
    def parse_string(str, trace = true)
      @document.errors = [] # reset errors
      rnv_doc = NokogiriSaxDocument.new(@document)

      if trace
        parser = Nokogiri::XML::SAX::PushParser.new(rnv_doc)
        parser.replace_entities = true
        @document.last_line = 0
        @document.last_col = 0
        str.each_line do |line|
          @document.last_line += 1
          @document.last_col = 0
          line.each_char do |c|
            @document.last_col += 1
            parser << c
          end
        end
        parser.finish
      else
        parser = Nokogiri::XML::SAX::Parser.new(rnv_doc)
        parser.parse_memory(file)
      end

      @document.valid?
    end

    # parse and validate file
    # @param [String, File] xml XML file to parse
    # @return [Boolean] true if valid
    def parse_file(xml, trace = true)
      @document.errors = [] # reset errors
      rnv_doc = NokogiriSaxDocument.new(@document)

      file = xml.is_a?(File) ? xml : File.open(xml)

      if trace
        parser = Nokogiri::XML::SAX::PushParser.new(rnv_doc)
        parser.replace_entities = true
        @document.last_line = 0
        @document.last_col = 0
        file.each do |line|
          @document.last_line += 1
          @document.last_col = 0
          line.each_char do |c|
            @document.last_col += 1
            parser << c
          end
        end
        parser.finish
      else
        parser = Nokogiri::XML::SAX::Parser.new(rnv_doc)
        parser.parse(file)
      end

      @document.valid?
    end

  end
end