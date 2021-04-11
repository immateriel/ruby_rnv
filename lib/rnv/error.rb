module RNV
  class Error
    def message
      @original_message
    end

    def expected
      @original_expected
    end

    # @param [String] data original content
    # @return [String] context
    def contextualize(data, max_len = 60)
      out = ""
      if data and @line
        err_data = data.split("\n")[@line - 1]
        err_data.insert(@col - 1, "🢑")

        start = 0
        if @col > max_len
          start = @col - max_len
        end
        out += (start > 0 ? "…" : "") + err_data[start..(@col + max_len)].strip + (@col + max_len < err_data.length ? "…" : "")
      end
      out
    end

    # @return [String]
    def to_s
      "#{@line}:#{@col}:error: #{self.message}\n#{self.expected}"
    end

    # @return [String]
    def inspect
      #"#<RNV::Error code: :#{@code}, message: '#{@message}', expected: '#{@expected}, line: #{@line}, column: #{@col}>"
      "#<RNV::Error code: :#{@code}, message: '#{self.message}', expected: '#{self.expected}, line: #{@line}, column: #{@col}>"
    end
  end
end