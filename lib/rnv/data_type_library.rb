module RNV
  # Datatype library callback object
  # @see https://www.oasis-open.org/committees/relax-ng/spec-20010811.html#IDA1I1R
  class DataTypeLibrary
    # @param [String] typ the data type
    # @param [String] val
    # @param [String] s
    # @return [Boolean]
    def equal(typ, val, s)
      true
    end

    # @param [String] typ the data type
    # @param [String] ps
    # @param [String] s the value
    # @return [Boolean]
    def allows(typ, ps, s)
      true
    end
  end
end
