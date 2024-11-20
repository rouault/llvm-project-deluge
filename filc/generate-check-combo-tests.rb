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

BYTE_LIMIT = 24

class Field
    attr_reader :type, :offset, :size

    def initialize(type, offset, size)
        raise unless type == :int or type == :ptr or type == :padding
        case type
        when :int
            raise unless size == 1 or size == 2 or size == 4 or size == 8
        when :ptr
            raise unless size == 8
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

[1, 2, 4, 8].each {
    | wordSize |
    [0, 1, 5, 7, 8].each {
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
    recurse(withField(fields, :ptr, 8))
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
            outp.puts "    #{field.bufAccess} = hello;"
        else
            outp.puts "    #{field.bufAccess} = value;"
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
            outp.puts "    ZASSERT(f#{index} == value);"
        end
    }
end

def generateTop(outp)
    outp.puts "#include <stdfil.h>"
    outp.puts "#include <inttypes.h>"
    outp.puts "#include <string.h>"
    outp.puts "#include <stdlib.h>"
    outp.puts "#include <stdbool.h>"
    outp.puts "#include \"utils.h\""
    outp.puts "static char* hello = \"hello\";"
    outp.puts "static unsigned char value;"
    outp.puts "static void init_test(void)"
    outp.puts "{"
    outp.puts "    unsigned index;"
    outp.puts "    value = 42;"
    outp.puts "    bool good = false;"
    outp.puts "    while (!good) {"
    outp.puts "        good = true;"
    outp.puts "        for (index = sizeof(char*); index--;) {"
    outp.puts "            if (((char*)&hello)[index] == value) {"
    outp.puts "                good = false;"
    outp.puts "                break;"
    outp.puts "            }"
    outp.puts "        }"
    outp.puts "        if (good)"
    outp.puts "            break;"
    outp.puts "        value++;"
    outp.puts "    }"
    outp.puts "}"
end

chunkedPossibilities = [[]]
$allPossibilities.keys.each_with_index {
    | fields, index |
    if chunkedPossibilities.last.size >= 10
        chunkedPossibilities << []
    end
    chunkedPossibilities.last << [fields, index]
}
chunkedPossibilities.each_with_index {
    | chunk, chunkIndex |
    FileUtils.mkdir_p("filc/tests/combostorm_success#{chunkIndex}")
    File.open("filc/tests/combostorm_success#{chunkIndex}/combostorm_success#{chunkIndex}.c", "w") {
        | outp |
        generateTop(outp)
        numTests = 0
        chunk.each {
            | entry |
            fields = entry[0]
            index = entry[1]
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
        outp.puts "    init_test();"
        numTests.times {
            | index |
            outp.puts "    test#{index}();"
        }
        outp.puts "    return 0;"
        outp.puts "}"
    }
    IO::write("filc/tests/combostorm_success#{chunkIndex}/manifest", "return: success\nslow: true")
}

$allPossibilities.keys.each_with_index {
    | fields, index |
    OPAQUE_CFGS.each {
        | opaqueCfg |
        testName = "combostorm_oobleft#{index}#{opaqueCfg.suffix}"
        FileUtils.mkdir_p("filc/tests/#{testName}")
        File.open("filc/tests/#{testName}/#{testName}.c", "w") {
            | outp |
            generateTop(outp)
            outp.puts "int main()"
            outp.puts "{"
            outp.puts "    init_test();"
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
            generateTop(outp)
            outp.puts "int main()"
            outp.puts "{"
            outp.puts "    init_test();"
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
        newField = Field.new(:ptr, field.offset / 8 * 8, 8)
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
                generateTop(outp)
                outp.puts "int main()"
                outp.puts "{"
                outp.puts "    init_test();"
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
                generateTop(outp)
                outp.puts "int main()"
                outp.puts "{"
                outp.puts "    init_test();"
                generateTest(outp, otherFields, fields, 0, opaqueCfg)
                outp.puts "    return 0;"
                outp.puts "}"
            }
            IO::write("filc/tests/#{testName}/manifest",
                      "return: failure\noutput-includes: \"filc user error\"")
        }
    }
}
