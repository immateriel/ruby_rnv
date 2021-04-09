module RNV
  # Datatype library callback object
  # @see https://www.oasis-open.org/committees/relax-ng/spec-20010811.html#IDA1I1R
  class DataTypeLibrary
    # @param [String] typ
    # @param [String] val
    # @param [String] s
    # @param [Integer] n
    # @return [Boolean]
    def equal(typ, val, s, n)
      true
    end

    # @param [String] typ
    # @param [String] ps
    # @param [String] s
    # @param [Integer] n
    # @return [Boolean]
    def allows(typ, ps, s, n)
      true
    end
  end
end
