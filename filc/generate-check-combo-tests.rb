#!/usr/bin/env ruby
#
# Copyright (c) 2024 Epic Games, Inc. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

require 'fileutils'

BYTE_LIMIT = 32

class Field
    attr_reader :type, :offset, :size

    def initialize(type, offset, size)
        raise unless type == :int or type == :ptr or type == :padding
        case type
        when :int
            raise unless size == 1 or size == 2 or size == 4 or size == 8 or size == 16
        when :ptr
            raise unless size == 16
        when :padding
            raise unless size > 0
        else
            raise
        end
        raise unless offset % size == 0 or type == :padding
        @type = type
        @offset = offset
        @size = size
    end

    def hash
        type.hash + offset.hash + size.hash
    end

    def eql?(other)
        type == other.type and offset == other.offset and size == other.size
    end

    def ==(other)
        self.eql? other
    end

    def <=>(other)
        if offset != other.offset
            return offset <=> other.offset
        end
        if type != other.type
            return type <=> other.type
        end
        size <=> other.size
    end

    def to_s
        "#{offset}:#{type}:#{size}"
    end

    def ctype
        case type
        when :ptr
            "char*"
        when :int
            case size
            when 1
                "int8_t"
            when 2
                "int16_t"
            when 4
                "int32_t"
            when 8
                "int64_t"
            when 16
                "__int128"
            else
                raise
            end
        else
            raise
        end
    end

    def bufAccess
        "*(#{ctype}*)(buf + #{offset})"
    end
end

def totalSize(fields)
    if fields.empty?
        0
    else
        fields[-1].offset + fields[-1].size
    end
end

def nextOffset(fields, size)
    (totalSize(fields) + size - 1) / size * size
end

def withField(fields, type, size)
    if type == :padding
        return fields if size == 0
        return fields + [Field.new(type, totalSize(fields), size)]
    end
    fields + [Field.new(type, nextOffset(fields, size), size)]
end

def withoutPadding(fields)
    fields.filter {
        | entry |
        entry.type != :padding
    }
end

$allPossibilities = {}

def addPossibility(fields)
    fields = withoutPadding(fields)
    return if fields.empty?
    $allPossibilities[fields] = true
end

[1, 2, 4, 8, 16].each {
    | wordSize |
    [0, 1, 5, 9, 16].each {
        | padding |
        [[], withField([], :padding, padding)].each {
            | fields |
            while totalSize(fields) <= BYTE_LIMIT
                fields = withField(fields, :int, wordSize)
                fields = withField(fields, :padding, padding)
            end
            addPossibility(fields)
        }
    }
}

def recurse(fields)
    if totalSize(fields) > BYTE_LIMIT
        addPossibility(fields)
        return
    end
    recurse(withField(fields, :padding, 8))
    recurse(withField(fields, :int, 8))
    recurse(withField(fields, :ptr, 16))
end

recurse([])

numFields = 0
$allPossibilities.keys.sort.each {
    | fields |
    puts fields.join(", ")
    numFields += fields.size
}

puts "# possibilities: #{$allPossibilities.size}"
puts "total # fields: #{numFields}"

class OpaqueCfg
    attr_reader :opaque1, :opaque2, :suffix

    def initialize(opaque1, opaque2, suffix)
        @opaque1 = opaque1
        @opaque2 = opaque2
        @suffix = suffix
    end
end

OPAQUE_CFGS = [
    OpaqueCfg.new("opaque", "opaque", ""),
    OpaqueCfg.new("opaque", "", "_no2ndopaque"),
    OpaqueCfg.new("", "opaque", "_no1stopaque"),
    OpaqueCfg.new("", "", "_noopaque")
]

def generateTest(outp, writeFields, readFields, offset, opaqueCfg)
    outp.puts "    char* buf = #{opaqueCfg.opaque1}(malloc(#{totalSize(writeFields)}));"
    writeFields.each_with_index {
        | field, index |
        if field.type == :ptr
            outp.puts "    #{field.bufAccess} = \"hello\";"
        else
            outp.puts "    #{field.bufAccess} = 42;"
        end
    }
    outp.puts "    buf = (char*)#{opaqueCfg.opaque2}(buf) + #{offset};"
    readFields.each_with_index {
        | field, index |
        outp.puts "    #{field.ctype} f#{index} = #{field.bufAccess};"
    }
    readFields.each_with_index {
        | field, index |
        if field.type == :ptr
            outp.puts "    ZASSERT(!strcmp(f#{index}, \"hello\"));"
        else
            outp.puts "    ZASSERT(f#{index} == 42);"
        end
    }
end

def generateIncludes(outp)
    outp.puts "#include <stdfil.h>"
    outp.puts "#include <inttypes.h>"
    outp.puts "#include <string.h>"
    outp.puts "#include <stdlib.h>"
    outp.puts "#include \"utils.h\""
end

FileUtils.mkdir_p("filc/tests/combostorm_success")
File.open("filc/tests/combostorm_success/combostorm_success.c", "w") {
    | outp |
    generateIncludes(outp)
    numTests = 0
    $allPossibilities.keys.each_with_index {
        | fields, index |
        OPAQUE_CFGS.each {
            | opaqueCfg |
            outp.puts "static void test#{numTests}(void)"
            outp.puts "{"
            generateTest(outp, fields, fields, 0, opaqueCfg)
            outp.puts "}"
            numTests += 1
            
            unless fields.select{ | field | field.type == :ptr }.empty? or
                  fields.select{ | field | field.type == :int }.empty?
                outp.puts "static void test#{numTests}(void)"
                outp.puts "{"
                generateTest(outp, fields, fields.select{ | field | field.type != :ptr }, 0, opaqueCfg)
                outp.puts "}"
                numTests += 1
            end
        }
    }
    outp.puts "int main()"
    outp.puts "{"
    numTests.times {
        | index |
        outp.puts "    test#{index}();"
    }
    outp.puts "    return 0;"
    outp.puts "}"
}
IO::write("filc/tests/combostorm_success/manifest", "return: success")

$allPossibilities.keys.each_with_index {
    | fields, index |
    OPAQUE_CFGS.each {
        | opaqueCfg |
        testName = "combostorm_oobleft#{index}#{opaqueCfg.suffix}"
        FileUtils.mkdir_p("filc/tests/#{testName}")
        File.open("filc/tests/#{testName}/#{testName}.c", "w") {
            | outp |
            generateIncludes(outp)
            outp.puts "int main()"
            outp.puts "{"
            generateTest(outp, fields, fields, -0x6660, opaqueCfg)
            outp.puts "    return 0;"
            outp.puts "}"
        }
        IO::write("filc/tests/#{testName}/manifest",
                  "return: failure\noutput-includes: \"filc safety error\"")

        testName = "combostorm_oobright#{index}#{opaqueCfg.suffix}"
        FileUtils.mkdir_p("filc/tests/#{testName}")
        File.open("filc/tests/#{testName}/#{testName}.c", "w") {
            | outp |
            generateIncludes(outp)
            outp.puts "int main()"
            outp.puts "{"
            generateTest(outp, fields, fields, 0x6660, opaqueCfg)
            outp.puts "    return 0;"
            outp.puts "}"
        }
        IO::write("filc/tests/#{testName}/manifest",
                  "return: failure\noutput-includes: \"filc safety error\"")
    }
}

def replaceIntsWithPtrs(fields)
    $results = {}
    fields.size.times {
        | index |
        field = fields[index]
        next unless field.type == :int
        newField = Field.new(:ptr, field.offset / 16 * 16, 16)
        newFields = fields.dup
        newFields[index] = newField
        newFields.filter! {
            | otherField |
            otherField == newField or
                otherField.offset + otherField.size <= newField.offset or
                otherField.offset >= newField.offset + newField.size
        }
        $results[newFields] = true
    }
    $results.keys
end

$allPossibilities.keys.each_with_index {
    | fields, index |
    borkedFields = replaceIntsWithPtrs(fields)
    borkedFields.each_with_index {
        | otherFields, index2 |
        OPAQUE_CFGS.each {
            | opaqueCfg |
            testName = "combostorm_badtype#{index}_#{index2}_read#{opaqueCfg.suffix}"
            FileUtils.mkdir_p("filc/tests/#{testName}")
            File.open("filc/tests/#{testName}/#{testName}.c", "w") {
                | outp |
                generateIncludes(outp)
                outp.puts "int main()"
                outp.puts "{"
                generateTest(outp, fields, otherFields, 0, opaqueCfg)
                outp.puts "    return 0;"
                outp.puts "}"
            }
            IO::write("filc/tests/#{testName}/manifest",
                      "return: failure\noutput-includes: \"filc safety error\"")

            testName = "combostorm_badtype#{index}_#{index2}_write#{opaqueCfg.suffix}"
            FileUtils.mkdir_p("filc/tests/#{testName}")
            File.open("filc/tests/#{testName}/#{testName}.c", "w") {
                | outp |
                generateIncludes(outp)
                outp.puts "int main()"
                outp.puts "{"
                generateTest(outp, otherFields, fields, 0, opaqueCfg)
                outp.puts "    return 0;"
                outp.puts "}"
            }
            IO::write("filc/tests/#{testName}/manifest",
                      "return: failure\noutput-includes: \"filc safety error\"")
        }
    }
}
