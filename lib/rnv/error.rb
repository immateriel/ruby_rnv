module RNV
  class Error
    # @return [String]
    def to_s
      "#{@line}:#{@col}:error: #{self.message}\n#{self.expected}"
    end

    def message
      @original_message
    end

    def expected
      @original_expected
    end

    # @return [String]
    def inspect
      #"#<RNV::Error code: :#{@code}, message: '#{@message}', expected: '#{@expected}, line: #{@line}, column: #{@col}>"
      "#<RNV::Error code: :#{@code}, message: '#{self.message}', expected: '#{self.expected}, line: #{@line}, column: #{@col}>"
    end
  end
end