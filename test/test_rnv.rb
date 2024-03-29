require 'rnv'
require 'pathname'
require 'minitest/autorun'

class TestRnv < Minitest::Test
  def test_parse_from_io
    validator = RNV::Validator.new
    validator.load_schema_from_file("test/fixtures/test001.rnc")
    validator.parse_file(File.open("test/fixtures/test001_1_valid.xml"))
    assert_equal 0, validator.errors.length
  end

  def test_parse_from_string
    validator = RNV::Validator.new
    validator.load_schema_from_string(File.read("test/fixtures/test001.rnc"))
    validator.parse_string(File.read("test/fixtures/test001_1_valid.xml"))
    assert_equal 0, validator.errors.length
  end

  def test_parse_multiple
    validator = RNV::Validator.new
    validator.load_schema_from_file("test/fixtures/test077.rnc")

    validator.parse_file("test/fixtures/test077_1_valid.xml")
    assert_equal 0, validator.errors.length
    validator.parse_file("test/fixtures/test077_2_valid.xml")
    assert_equal 0, validator.errors.length
    validator.parse_file("test/fixtures/test077_3_invalid.xml")
    assert validator.errors.length > 0
  end

  def test_no_schema
    validator = RNV::Validator.new
    validator.load_schema_from_string("invalid schema}")

    assert_raises RNV::SchemaNotLoaded do
      validator.parse_file("test/fixtures/test077_1_valid.xml")
    end

    validator = RNV::Validator.new
    assert_raises RNV::SchemaNotLoaded do
      validator.parse_file("test/fixtures/test077_1_valid.xml")
    end
  end

  def test_invalid_param
    validator = RNV::Validator.new
    assert_raises TypeError do
      validator.load_schema_from_file(Pathname.new("test/fixtures/test077.rnc"))
    end
    assert_raises TypeError do
      validator.load_schema_from_file(nil)
    end
    assert_raises TypeError do
      validator.load_schema_from_string(nil)
    end

    assert !validator.load_schema_from_file("test/fixtures/unknown.rnc")

    validator.load_schema_from_file("test/fixtures/test077.rnc")
    assert_raises TypeError do
      validator.parse_file(nil)
    end
    assert_raises RNV::DocumentEmpty do
      validator.parse_string(nil)
    end
    assert_raises RNV::DocumentEmpty do
      validator.parse_string("")
    end

    validator.parse_string(" ")
  end

  class TestDTL < RNV::DataTypeLibrary
    def equal(typ, val, s)
      #puts "equal #{typ} #{val} #{s} #{n}"
      ["x", "y", "z"].include?(val)
    end

    def allows(typ, ps, s)
      #puts "allows #{typ} #{ps} #{s} #{n}"
      s == "xx"
    end
  end

  def test_error_xpath
    validator = RNV::Validator.new
    validator.load_schema_from_string %{
default namespace = "http://www.example.com"
namespace ns1 = "http://www.example.com/1"
namespace ns2 = "http://www.example.com/2"
element foo {
  element ns1:bar {
    element ns2:baz {text+}*
  }*
 }
}
    xml = %{<foo xmlns = "http://www.example.com" xmlns:ns1="http://www.example.com/1" xmlns:ns2="http://www.example.com/2">
<ns1:bar><ns2:baz>text</ns2:baz><ns2:baz>more text</ns2:baz><ns2:baz><qux/></ns2:baz></ns1:bar>
</foo>
}
    validator.parse_string xml
    assert_equal 1, validator.errors.length
    err = validator.errors.first
    assert_equal "/xmlns:foo[1]/ns1:bar[1]/ns2:baz[3]/xmlns:qux[1]", err.xpath.first
    assert_equal({ "xmlns" => "http://www.example.com", "ns1" => "http://www.example.com/1", "ns2" => "http://www.example.com/2" },
                 err.xpath.last)
    assert Nokogiri::XML.parse(xml).root.at(*err.xpath) != nil

    xml = %{<foo xmlns = "http://www.example.com">
<bar xmlns="http://www.example.com/1"><baz xmlns="http://www.example.com/2">text</baz>
<baz xmlns="http://www.example.com/2"><qux/></baz></bar>
</foo>
}
    validator.parse_string xml
    assert_equal 1, validator.errors.length
    err = validator.errors.first
    assert_equal "/xmlns:foo[1]/*[local-name() = 'bar'][1]/*[local-name() = 'baz'][2]/*[local-name() = 'qux'][1]", err.xpath.first
    assert_equal({ "xmlns" => "http://www.example.com" },
                 err.xpath.last)
    assert Nokogiri::XML.parse(xml).root.at(*err.xpath) != nil

    xml = %{<foo xmlns = "http://www.example.com" xmlns:ns1="http://www.example.com/1">
<ns1:bar>text</ns1:bar><ns1:bar><baz xmlns="http://www.example.com/2">text</baz></ns1:bar>
</foo>
}
    validator.parse_string xml
    assert_equal 1, validator.errors.length
    err = validator.errors.first
    assert_equal "/xmlns:foo[1]/ns1:bar[1]", err.xpath.first
    assert_equal({ "xmlns" => "http://www.example.com", "ns1" => "http://www.example.com/1" },
                 err.xpath.last)
    assert Nokogiri::XML.parse(xml).root.at(*err.xpath) != nil
  end

  def test_datatype_library
    validator = RNV::Validator.new
    validator.load_schema_from_string %{
    datatypes dtl = "http://example.com/dtl"
    element foo {
      attribute val { dtl:attr "x" | dtl:attr "y"},
      dtl:text
     }
    }
    validator.document.add_datatype_library("http://example.com/dtl", TestDTL.new)
    validator.parse_string %{<foo val="x">xx</foo>}
    assert_equal 0, validator.errors.length

    validator.parse_string %{<foo val="z">yy</foo>}
    assert_equal 2, validator.errors.length
  end

  # test library that check if link is local or external
  class TestUriDTL < RNV::DataTypeLibrary
    def allows(typ, ps, s)
      #puts "allows #{typ} #{ps} #{s} #{n}"
      begin
        uri = URI.parse(s)
        case typ
        when "external"
          uri.scheme != nil
        when "local"
          uri.scheme == nil
        else
          false
        end
      rescue
        false
      end
    end
  end

  def test_datatype_library_uri
    validator = RNV::Validator.new
    validator.load_schema_from_string %{
    datatypes dtl = "http://example.com/dtl"
    element doc {
    element link {
      attribute href { dtl:local | dtl:external}
     }
    | element img {
      attribute src {dtl:local}
    }
    }
    }
    validator.document.add_datatype_library("http://example.com/dtl", TestUriDTL.new)
    validator.parse_string %{<doc><link href="local/file"/></doc>}
    assert_equal 0, validator.errors.length

    validator.parse_string %{<doc><link href="http://example.com/external/link"/></doc>}
    assert_equal 0, validator.errors.length

    validator.parse_string %{<doc><link href="http://example.com/external#invalid#href"/></doc>}
    #puts validator.errors
    assert_equal 1, validator.errors.length

    validator.parse_string %{<doc><img src="http://example.com/external/link"/></doc>}
    #puts validator.errors
    assert_equal 1, validator.errors.length
  end

end
