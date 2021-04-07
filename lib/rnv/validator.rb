require 'nokogiri'
require 'rnv/rnv'

module RNV
  # @!visibility private
  class NokogiriSaxDocument < Nokogiri::XML::SAX::Document
    # @return [Nokogiri::XML::SAX::ParserContext]
    attr_accessor :ctx
    # @param [RNV::Document] document
    def initialize(document)
      @document = document
    end

    def start_element_namespace(name, attrs = [], prefix = nil, uri = nil, ns = nil)
      update_line_col
      tag_attrs = attrs.map { |attr| [attr.uri ? "#{attr.uri}:#{attr.localname}" : attr.localname, attr.value] }
      tag_name = uri ? "#{uri}:#{name}" : name
      @document.start_tag(tag_name, tag_attrs.flatten)
    end

    def end_element_namespace(name, prefix = nil, uri = nil)
      update_line_col
      tag_name = uri ? "#{uri}:#{name}" : name
      @document.end_tag(tag_name)
    end

    def characters str
      update_line_col
      @document.characters(str)
    end

    def cdata_block str
      update_line_col
      @document.characters(str)
    end

    private

    def update_line_col
      @document.last_line = @ctx.line
      @document.last_col = @ctx.column
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
    def parse_string(str)
      @document.errors = [] # reset errors
      rnv_doc = NokogiriSaxDocument.new(@document)

      parser = Nokogiri::XML::SAX::Parser.new(rnv_doc)
      parser.parse_memory(str) do |ctx|
        rnv_doc.ctx = ctx
      end

      @document.valid?
    end

    # parse and validate file
    # @param [String, File] xml XML file to parse
    # @return [Boolean] true if valid
    def parse_file(xml)
      @document.errors = [] # reset errors
      rnv_doc = NokogiriSaxDocument.new(@document)

      file = xml.is_a?(File) ? xml : File.open(xml)

      parser = Nokogiri::XML::SAX::Parser.new(rnv_doc)
      parser.parse(file) do |ctx|
        rnv_doc.ctx = ctx
      end

      @document.valid?
    end

  end
end