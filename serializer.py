import struct

class Serializer:

    def __init__(self):
        self.data = ""

    def write(self, bytes):
        self.data += bytes

    def write_string(self, string):
        self.write_variable_uint(len(string))
        self.write(string)

    def write_hash(self, hash):
        self.write(hash[::-1])

    def write_byte(self, val):
        self.write(chr(val))
    def write_2_bytes(self, val):
        self._write_num('<H', val)
    def write_4_bytes(self, val):
        self._write_num('<I', val)
    def write_8_bytes(self, val):
        self._write_num('<Q', val)

    def write_variable_uint(self, size):
        if size < 0xfd:
            self.write_byte(val)
        elif size <= 0xffff:
            self.write('\xfd')
            self.write_2_bytes(val)
        elif size <= 0xffffffff:
            self.write('\xfe')
            self.write_4_bytes(val)
        else:
            self.write('\xff')
            self.write_8_bytes(val)

    def _write_num(self, format, num):
        s = struct.pack(format, num)
        self.write(s)

