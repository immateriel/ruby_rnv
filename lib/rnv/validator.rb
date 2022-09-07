require 'nokogiri'
require 'rnv/rnv'
require 'rnv/pre_processor'

module RNV
  class DocumentEmpty < StandardError; end

  # @!visibility private
  class XpathNode
    attr_accessor :name
    attr_accessor :uri, :namespaces
    attr_accessor :parent
    attr_accessor :children

    def initialize(name)
      @name = name
      @children = []
    end

    def add_child(name, uri = nil, namespaces = {})
      child = XpathNode.new(name)
      child.uri = uri
      child.namespaces = namespaces
      child.parent = self
      @children << child
      child
    end

    def to_xpath
      xpath = []
      current = self
      namespaces = {}
      rev_namespaces = {}
      ns_current = self
      while ns_current.name
        ns_current.namespaces.each do |prefix, uri|
          rev_namespaces[uri] ||= []
          rev_namespaces[uri] << prefix
        end
        ns_current = ns_current.parent
      end

      rev_namespaces.each do |uri, prefixes|
        if prefixes.length > 0
          prefixes.select! { |prefix| prefix != "xmlns" }
        end
        namespaces[prefixes.first||"xmlns"] = uri
      end

      while current.name
        current_name_with_ns = "#{rev_namespaces[current.uri]&.first||"xmlns"}:#{current.name}"
        xpath << "#{current_name_with_ns}[#{current.parent.children.select { |child| child.name == current.name }.index(current) + 1}]"
        current = current.parent
      end
      [xpath.reverse, namespaces]
    end
  end

  # @!visibility private
  class NokogiriSaxDocument < Nokogiri::XML::SAX::Document
    # @return [Nokogiri::XML::SAX::ParserContext]
    attr_accessor :ctx
    attr_accessor :pre_processor

    # @param [RNV::Document] document
    def initialize(document)
      @document = document
      @xpath_tree = XpathNode.new(nil)
    end

    def start_element_namespace(name, attrs = [], prefix = nil, uri = nil, ns = nil)
      tag_attrs = attrs.map { |attr| [attr.uri ? "#{attr.uri}:#{attr.localname}" : attr.localname, attr.value] }
      tag_name = uri ? "#{uri}:#{name}" : name
      namespaces = {}
      ns.each do |n|
        namespaces[n.first || "xmlns"] = n.last
      end
      @xpath_tree = @xpath_tree.add_child(name, uri, namespaces)
      update_line_col
      @document.start_tag(@pre_processor.tag(tag_name), @pre_processor.attributes(tag_attrs))
    end

    def end_element_namespace(name, prefix = nil, uri = nil)
      tag_name = uri ? "#{uri}:#{name}" : name
      update_line_col
      @document.end_tag(@pre_processor.tag(tag_name))
      @xpath_tree = @xpath_tree.parent
    end

    def characters(value)
      update_line_col
      @document.characters(@pre_processor.text(value))
    end

    def cdata_block(value)
      update_line_col
      @document.characters(@pre_processor.text(value))
    end

    private

    def update_line_col
      @document.last_line = @ctx.line
      @document.last_col = @ctx.column
      xpath = @xpath_tree.to_xpath
      @document.xpath = ["/" + xpath.first.join("/"), xpath.last]
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

      raise RNV::DocumentEmpty if str.nil? or str.empty?

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

      raise RNV::DocumentEmpty if file.size == 0

      parser = Nokogiri::XML::SAX::Parser.new(rnv_doc)
      parser.parse(file) do |ctx|
        ctx.replace_entities = true
        rnv_doc.ctx = ctx
      end

      @document.valid?
    end

  end
end