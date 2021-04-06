require 'rnv'

require 'minitest/autorun'

# James Clark's test suite
# FIXME : unsupported include
# trang : 8 failures/1768, rng with unresolved include are not converted
# xslt : 69 failures/1762
class TestSuite < Minitest::Test
  @@failing = %w[test_016 test_150 test_160 test_176 test_194 test_509 test_523 test_533]+
    %w[test_011 test_171 test_189]  # FIXME bug with unknown entities

  Dir.glob("test/fixtures/*.rnc").select{|f| !f.include?("_resource")}.each do |rnc|
    num = rnc.sub(/[^0-9]*([0-9]+)[^0-9]*/,'\1')
    unless @@failing.include?("test_#{num}")
      define_method "test_#{num}" do
        process(rnc)
      end
    end
  end

  private

  def process(rnc)
    rnc_name = rnc.sub(/\.rnc$/,"")
    if false
    resources = Dir.glob(rnc_name+"*_resource_*.rnc")
    tmp_res = []
    if resources.length > 0
      resources.each do |res|
        res_name = res.sub(/.*_resource_(.+)\.rnc/,'\1')
        tmp_res_fn = "#{File.dirname(res)}/#{res_name.gsub(/-/,"/")}"
        FileUtils.mkdir_p(File.dirname(tmp_res_fn))
        #puts "COPY RESOURCE #{res} -> #{tmp_res_fn}"
        FileUtils.cp res, tmp_res_fn
        tmp_res << tmp_res_fn
      end
    end
    end

    valid_xmls = Dir.glob(rnc_name+"*_valid.xml")
    valid_xmls.each do |xml|
      #puts "#{rnc} -> #{xml}"
      v = RNV::NokogiriValidator.validate(rnc,xml)
      if v.errors.length > 0
        puts "FAIL should be valid #{rnc} -> #{xml} : #{v.errors}"
      end
      assert_equal 0, v.errors.length
    end
    invalid_xmls = Dir.glob(rnc_name+"*_invalid.xml")
    invalid_xmls.each do |xml|
      v = RNV::NokogiriValidator.validate(rnc,xml)
      #puts "#{rnc} -> #{xml} #{v.errors}"
      #v.errors.each do |e|
        #puts e
      #end
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
end