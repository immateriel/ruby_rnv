module RNV
  # Object to modify data before validation (eg: HTML5 data-* attributes)
  class PreProcessor
    # replace attributes before validate
    # @param [Array<Array<String>>] attrs
    # @return [Array<Array<String>>]
    def attributes(attrs)
      attrs
    end

    # replace tag name before validate
    # @param [String] tag
    # @return [String]
    def tag(tag)
      tag
    end

    # replace content text before validate
    # @param [String] txt
    # @return [String]
    def text(txt)
      txt
    end
  end
end