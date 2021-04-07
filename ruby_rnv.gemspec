Gem::Specification.new do |s|
  s.name    = "ruby_rnv"
  s.version = "0.2.0"
  s.summary = "RelaxNG compact syntax validator"
  s.author  = "Julien Boulnois"

  s.extensions = "ext/rnv/extconf.rb"

  s.files = ["Gemfile"] +
            Dir.glob("ext/**/*.{c,h,rb}") +
            Dir.glob("lib/**/*.{rb}")

  s.require_paths = ["lib"]

  s.add_development_dependency "bundler", "~> 1.14"
  s.add_development_dependency "rake", "~> 10.0"
  s.add_development_dependency "minitest", "~> 5.0"
  s.add_development_dependency "rake-compiler"

  s.add_dependency "nokogiri"

end
