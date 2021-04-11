require 'rnv'
require 'minitest/autorun'

class TestSuite < Minitest::Test
  @@failing = %w[test_150 test_160 test_509 test_523 test_533] +
    %w[test_tok_legal_ident_001 test_tok_illegal_cname_002 test_tok_legal_cname_005 test_tok_legal_ident_009
       test_tok_legal_lit_011 test_tok_legal_ident_006 test_tok_illegal_cname_006 test_tok_legal_cname_001
       test_tok_illegal_esc_005 test_tok_legal_cname_004 test_tok_legal_ident_003 test_tok_illegal_cname_008
       test_tok_legal_ident_004 test_tok_illegal_cname_003 test_tok_legal_lit_012 test_tok_legal_ident_002
       test_tok_legal_ident_007 test_tok_illegal_cname_005 test_tok_legal_lit_010 test_tok_illegal_esc_006
       test_tok_illegal_esc_002 test_tok_legal_ident_005 test_tok_legal_ident_008 test_tok_illegal_esc_003
       test_tok_legal_comm_010 test_tok_legal_cname_003 test_tok_illegal_esc_004 test_tok_illegal_cname_007
       test_tok_legal_lit_009]
  # James Clark's test suite
  # FIXME : unsupported include
  # trang : 5 failures/1768, rng with unresolved include are not converted
  # xslt : 69 failures/1762
  Dir.glob("test/fixtures/*.rnc").select { |f| !f.include?("_resource") }.each do |rnc|
    num = rnc.sub(/[^0-9]*([0-9]+)[^0-9]*/, '\1')
    test_name = "test_#{num}"
    unless @@failing.include?(test_name)
      define_method test_name do
        process(rnc)
      end
    end
  end

  # anglia test suite
  # 29 failures/120
  Dir.glob("test/fixtures/anglia/*.rnc") do |rnc|
    test_name = "test_" + File.basename(rnc.gsub(/\-/, "_").sub(/\.rnc$/, ""))
    unless @@failing.include?(test_name)
      define_method test_name do
        if rnc.include?("illegal")
          process_schema(rnc, false)
        else
          process_schema(rnc)
        end
      end
    end
  end

  private

  def process_schema(rnc, valid = true)
    v = RNV::Validator.new
    v.load_schema_from_file(rnc)
    if valid
      if v.errors.length > 0
        puts "FAIL should be valid #{rnc} : #{v.errors}"
      end
      assert_equal 0, v.errors.length
    else
      dump_errors(rnc, v.errors)
      assert v.errors.length > 0
    end
  end

  def process(rnc)
    rnc_name = rnc.sub(/\.rnc$/, "")
    if false
      resources = Dir.glob(rnc_name + "*_resource_*.rnc")
      tmp_res = []
      if resources.length > 0
        resources.each do |res|
          res_name = res.sub(/.*_resource_(.+)\.rnc/, '\1')
          tmp_res_fn = "#{File.dirname(res)}/#{res_name.gsub(/-/, "/")}"
          FileUtils.mkdir_p(File.dirname(tmp_res_fn))
          #puts "COPY RESOURCE #{res} -> #{tmp_res_fn}"
          FileUtils.cp res, tmp_res_fn
          tmp_res << tmp_res_fn
        end
      end
    end

    valid_xmls = Dir.glob(rnc_name + "*_valid.xml")
    valid_xmls.each do |xml|
      #puts "#{rnc} -> #{xml}"
      v = RNV::Validator.new
      v.load_schema_from_file(rnc)
      v.parse_file(xml)
      if v.errors.length > 0
        puts "FAIL should be valid #{rnc} -> #{xml} : #{v.errors}"
      end
      assert_equal 0, v.errors.length
    end
    invalid_xmls = Dir.glob(rnc_name + "*_invalid.xml")
    invalid_xmls.each do |xml|
      v = RNV::Validator.new
      v.load_schema_from_file(rnc)
      v.parse_file(xml)

      dump_errors(rnc, v.errors)
      if v.errors.length == 0
        puts "FAIL should be invalid #{rnc} -> #{xml}"
      end
      assert v.errors.length > 0
    end
    if false
      tmp_res.each do |tmp_res_fn|
        FileUtils.rm tmp_res_fn
      end
    end
  end

  def dump_errors(rnc, errors)
    if false
      errors.each do |e|
        puts "#{rnc} #{e.code} #{e.to_s}"
      end
    end
  end

end
