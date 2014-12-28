#!/usr/bin/env python3
# -*- coding: utf-8

__author__ = 'Mathieu Clément'
__version__ = '0.1'

def char_to_multiple_outputs(char):
    asInt = ord(char) - ord('0')
    buf = ''
    for i in range(0,ord('z') - ord('0')):
        if i == asInt:
            buf += '1 '
        else:
            buf += '-1 '
    buf += '\n'
    return buf

def file_to_multiple_outputs(input_filename, output_filename):
    fh = open(input_filename, 'r')
    oh = open(output_filename, 'w')

    count = 0
    for line in fh:
        count += 1
        if count % 2 == 0:
            oh.write(char_to_multiple_outputs(line[0]))
        else:
            oh.write(line)

    fh.close()
    oh.close()

if __name__ == '__main__':
    import sys
    file_to_multiple_outputs(sys.argv[1], sys.argv[2])
