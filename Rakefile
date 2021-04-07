require "rake/extensiontask"
require "bundler/gem_tasks"
require "rake/testtask"

spec = Bundler::GemHelper.gemspec

Rake::ExtensionTask.new "rnv", spec do |ext|
  ext.ext_dir = "ext/rnv/"
  ext.lib_dir = "lib/rnv/"
end

Rake::TestTask.new(:test) do |t|
  t.libs << "test"
  t.libs << "lib"
  t.test_files = FileList['test/**/test_*.rb']
end

task :default => :test
