module RNV
  class Error
    # @return [String]
    def to_s
      "#{@line}:#{@col}:error: #{@message}\n#{@expected}"
    end

    # @return [String]
    def inspect
      "#<RNV::Error code: :#{@code}, message: '#{@message}', expected: '#{@expected} line: #{@line}, column: #{@col}>"
    end
  end
end