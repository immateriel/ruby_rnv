module RNV
  class Error
    attr_reader :original_message, :original_expected
    attr_reader :ns_uri, :element, :attr, :value, :required, :allowed

    # @return [String]
    def message
      @original_message
    end

    # @return [String]
    def expected
      @original_expected
    end

    # @return [String]
    def compact_message
      case @code
      when :rnv_er_elem
        "element <#{@element}> is not allowed"
      when :rnv_er_ufin
        "element <#{@element}> is incomplete"
      when :rnv_er_akey
        "attribute #{@attr} is not allowed"
      when :rnv_er_aval
        "attribute #{@attr} has invalid value '#{@value}'"
      when :rnv_er_amis
        "element <#{@element}> attribute missing"
      else
        @original_message
      end
    end

    # @return [String]
    def compact_expected
      [(compact_required_set.length > 0 ? compact_expected_phrase(compact_required_set) +" required" : nil),
      (compact_allowed_set.length > 0 ? compact_expected_phrase(compact_allowed_set) + " allowed" : nil)].join("; ")
    end

    # @param [String] data original content
    # @return [String] context
    def contextualize(data, max_len = 60)
      out = ""
      if data and @line
        err_data = data.split("\n")[@line - 1]
        if err_data
          err_data.insert(@col - 1, "🢑")

          start = 0
          if @col > max_len
            start = @col - max_len
          end
          out += (start > 0 ? "…" : "") + err_data[start..(@col + max_len)].strip + (@col + max_len < err_data.length ? "…" : "")
        end
      end
      out
    end

    # @return [String]
    def to_s
      "#{@line}:#{@col}:error: #{self.message}\n#{self.expected}"
    end

    # @return [String]
    def inspect
      "#<RNV::Error code: :#{@code}, message: '#{self.message}', expected: '#{self.expected}, line: #{@line}, column: #{@col}>"
    end

    private

    def compact_expected_to_s(e, with_ns = false)
      case e.first
      when :rn_p_attribute
        compact_expected_to_s(e.last, with_ns)
      when :rn_p_element
        "<"+compact_expected_to_s(e.last, with_ns)+">"
      when :rn_p_data
        compact_expected_to_s(e.last, with_ns)
      when :rn_nc_datatype
        "datatype #{e[-2]}:#{e.last}"
      when :rn_nc_qname
        with_ns ? "#{e.first}:#{e.last}" : e.last
      end
    end

    def compact_required_set
      @compact_required_set ||= @required.map{|r| compact_expected_to_s(r)}.compact.uniq
    end

    def compact_allowed_set
      @compact_allowed_set ||= @allowed.map{|r| compact_expected_to_s(r)}.compact.uniq
    end

    def compact_expected_phrase(set)
      if set.length > 1
        set[0..-2].join(", ") + " or "+set.last
      else
        set.last
      end
    end
  end
end
