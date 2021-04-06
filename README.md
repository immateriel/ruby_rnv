# RelaxNG Compact Syntax validator for Ruby

Based on http://www.davidashen.net/rnv.html by David Tolpin.
Original C program has been refactored to be useable as a library.
The result is integrated with Nokogiri SAX parser to provide a high level ruby gem.

## Usage
```ruby
require 'rnv'

validator = RNV::NokogiriValidator.validate("test/fixtures/test330.rnc","test/fixtures/test330_2_invalid.xml")
pp validator.errors # [#<RNV::Error code: :rnv_er_emis, message: 'incomplete content', line: 2, col: 8>, #<RNV::Error code: :rnv_er_elem, message: 'element ^bar not allowed', line: 3, col: 8>]
```
