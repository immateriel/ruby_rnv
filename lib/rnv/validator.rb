require 'nokogiri'
require 'rnv/rnv'
require 'rnv/pre_processor'

module RNV
  # @!visibility private
  class NokogiriSaxDocument < Nokogiri::XML::SAX::Document
    # @return [Nokogiri::XML::SAX::ParserContext]
    attr_accessor :ctx, :pre_processor
    # @param [RNV::Document] document
    def initialize(document)
      @document = document
    end

    def start_element_namespace(name, attrs = [], prefix = nil, uri = nil, ns = nil)
      update_line_col
      tag_attrs = attrs.map { |attr| [attr.uri ? "#{attr.uri}:#{attr.localname}" : attr.localname, attr.value] }
      tag_name = uri ? "#{uri}:#{name}" : name
      @document.start_tag(@pre_processor.tag(tag_name), @pre_processor.attributes(tag_attrs))
    end

    def end_element_namespace(name, prefix = nil, uri = nil)
      update_line_col
      tag_name = uri ? "#{uri}:#{name}" : name
      @document.end_tag(@pre_processor.tag(tag_name))
    end

    def characters str
      update_line_col
      @document.characters(@pre_processor.text(str))
    end

    def cdata_block str
      update_line_col
      @document.characters(@pre_processor.(str))
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
    # @return [Boolean] true if schema loaded successfuly
    def load_schema_from_string(str)
      @document.load_string(str)
    end

    # @param [String] file RNC schema filename
    # @return [Boolean] true if schema loaded successfuly
    def load_schema_from_file(file)
      @document.load_file(file)
    end

    # parse and validate buffer
    # @param [String] str XML buffer to parse
    # @param [RNV::PreProcessor] pre_processor an optional pre-processor for tag and attributes data
    # @return [Boolean] true if valid
    def parse_string(str, pre_processor = PreProcessor.new)
      @document.start_document
      rnv_doc = NokogiriSaxDocument.new(@document)
      rnv_doc.pre_processor = pre_processor

      parser = Nokogiri::XML::SAX::Parser.new(rnv_doc)
      parser.parse_memory(str) do |ctx|
        ctx.replace_entities = true
        rnv_doc.ctx = ctx
      end

      @document.valid?
    end

    # parse and validate file
    # @param [String, File] xml XML file to parse
    # @param [RNV::PreProcessor] pre_processor an optional pre-processor for tag and attributes data
    # @return [Boolean] true if valid
    def parse_file(xml, pre_processor = PreProcessor.new)
      @document.start_document
      rnv_doc = NokogiriSaxDocument.new(@document)
      rnv_doc.pre_processor = pre_processor

      file = xml.is_a?(File) ? xml : File.open(xml)

      parser = Nokogiri::XML::SAX::Parser.new(rnv_doc)
      parser.parse(file) do |ctx|
        ctx.replace_entities = true
        rnv_doc.ctx = ctx
      end

      @document.valid?
    end

  end
end