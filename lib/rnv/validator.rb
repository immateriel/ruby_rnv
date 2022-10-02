require 'nokogiri'
require 'rnv/rnv'
require 'rnv/pre_processor'

module RNV
  class DocumentEmpty < StandardError; end

  # @!visibility private
  class XpathNode
    attr_accessor :name
    attr_accessor :uri, :namespaces, :rev_namespaces, :xpath_name
    attr_accessor :parent
    attr_accessor :idx
    attr_accessor :children

    def initialize(name)
      @name = name
      @idx = 0
      @children = []
      @namespaces = {}
      @rev_namespaces = {}
      @xpath_name = nil
    end

    def add_child(name, uri = nil, local_namespaces = {})
      child = XpathNode.new(name)
      child.uri = uri

      @rev_namespaces.each do |ns_uri, ns_alias|
        child.rev_namespaces[ns_uri] = ns_alias
      end
      local_namespaces.each do |ns_alias, ns_uri|
        child.rev_namespaces[ns_uri] = ns_alias
      end

      all_namespaces = {}
      child.rev_namespaces.each do |ns_uri, ns_alias|
        all_namespaces[ns_alias] ||= []
        all_namespaces[ns_alias] << ns_uri
      end

      child.namespaces = @namespaces.dup
      child.parent = self
      child.idx = self.children.count { |other_child| other_child.name == name } + 1

      if child.rev_namespaces[child.uri]
        if all_namespaces[child.rev_namespaces[child.uri]].length == 1
          child.namespaces[child.rev_namespaces[child.uri]] = child.uri
          child.xpath_name = "#{child.rev_namespaces[child.uri]}:#{child.name}[#{child.idx}]"
        else
          child.xpath_name = "*[local-name() = '#{child.name}'][#{child.idx}]" # we can't say which ns it is
        end
      else
        child.xpath_name = "xmlns:#{child.name}[#{child.idx}]"
      end

      @children << child
      child
    end

    def to_xpath
      xpath = []
      current = self

      while current.name
        xpath << current.xpath_name
        current = current.parent
      end

      [xpath.reverse, self.namespaces]
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
      ns&.each do |n|
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