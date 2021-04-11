require 'nokogiri'
require 'fileutils'


clean_files = Dir.glob("*") - ["spectest.xml","conv_spectest.rb","RngToRncClassic.xsl"]

clean_files.each do |f|
  FileUtils.rm f unless File.directory?(f)
end

doc = Nokogiri::XML.parse(File.read("spectest.xml"))

def convert(rng_num, converter = :trang)
  case converter
  when :xslt
    xslt_cmd = "xsltproc RngToRncClassic.xsl test#{rng_num}.rng > test#{rng_num}.rnc"
    puts " -> #{xslt_cmd}"
    system(xslt_cmd)
  when :trang
    trang_cmd = "trang -I rng -O rnc test#{rng_num}.rng test#{rng_num}.rnc"
    puts " -> #{trang_cmd}"
    system(trang_cmd)
  end
end

rng_cnt = 1
doc.search("//testSuite").each do |test_suite|
  puts test_suite.at("./documentation").text if test_suite.at("./documentation")

  test_suite.search("testCase").each do |test_case|
    if test_case.search("incorrect").count == 0
      rng_num = ("%03d" % rng_cnt)
      puts "TEST CASE #{rng_num}"
      test_cnt = 1
      test_case.children.each do |child|
        case child.name
        when "resource"
          puts "RESOURCE #{child["name"]}"
          nm = child["name"].gsub(/\//,"-")
          File.open("test#{rng_num}_resource_#{nm}.rng", "w") do |f|
            f.write child.children.to_s.strip
          end
          convert("#{rng_num}_resource_#{nm}")
        when "incorrect"
          # IGNORED
          puts "INCORRECT"
          # won't convert to rnc
          File.open("test#{rng_num}_incorrect.rng", "w") do |f|
            f.write child.children.to_s.strip
          end
          convert("#{rng_num}_incorrect", :xslt)
          #FileUtils.rm("test#{rng_num}.rng")
          rng_cnt += 1
        when "correct"
          puts "CORRECT"
          File.open("test#{rng_num}.rng", "w") do |f|
            f.write child.children.to_s.strip
          end
          convert(rng_num)
          #FileUtils.rm("test#{rng_num}.rng")
          rng_cnt += 1
        when "invalid"
          puts "INVALID"
          File.open("test#{rng_num}_#{test_cnt}_invalid.xml", "w") do |f|
            f.write child.children.to_s.strip
          end
          test_cnt += 1
        when "valid"
          puts "VALID"
          File.open("test#{rng_num}_#{test_cnt}_valid.xml", "w") do |f|
            f.write child.children.to_s.strip
          end
          test_cnt += 1
        end
      end
    end
  end
end
