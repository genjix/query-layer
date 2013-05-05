import sys
sys.path.append("gen-py")

import hashlib
import serializer

from chain_service import csBlockchainService
from chain_service.ttypes import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

class BlocksManager:

    def __init__(self, client):
        self.client = client

    def __getitem__(self, index):
        depth, hash = None, None
        if type(index) == int:
            depth = index
            if depth < 0:
                last_depth = self.last_depth
                if abs(depth) > last_depth + 1:
                    raise IndexError("Negative index out of range.")
                # Count backwards from the end
                depth = self.last_depth + depth + 1
            block_head = self.client.block_header_by_depth(depth)
        elif type(index) == str and len(index) == 64:
            hash = index.decode("hex")
            block_head = self.client.block_header_by_hash(hash)
        elif type(index) == str and len(index) == 32:
            hash = index
            block_head = self.client.block_header_by_hash(hash)
        else:
            raise IndexError("Bad index.")
        return Block(self.client, block_head, depth, hash)

    @property
    def last_depth(self):
        return self.client.last_depth()

    def __len__(self):
        return self.last_depth + 1

def generate_sha256_hash(data):
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()[::-1]

def hash_block_header(block):
    key = serializer.Serializer()
    key.write_4_bytes(block.version)
    key.write_hash(block.previous_block_hash)
    key.write_hash(block.merkle)
    key.write_4_bytes(block.timestamp)
    key.write_4_bytes(block.bits)
    key.write_4_bytes(block.nonce)
    return generate_sha256_hash(key.data)

class Block:

    def __init__(self, client, header, depth, hash):
        self.client = client
        self.header = header
        self.depth_ = depth
        self.hash_ = hash
        self.transactions_ = None

    def __repr__(self):
        return "<Block %s>" % self.depth

    @property
    def hash(self):
        if self.hash_ is None:
            self.hash_ = hash_block_header(self.header)
        return self.hash_

    @property
    def depth(self):
        if self.depth_ is None:
            self.depth_ = self.client.block_depth(self.hash_)
        return self.depth_

    @property
    def version(self):
        return self.header.version
    @property
    def previous_block_hash(self):
        return self.header.previous_block_hash
    @property
    def merkle(self):
        return self.header.merkle
    @property
    def timestamp(self):
        return self.header.timestamp
    @property
    def bits(self):
        return self.header.bits
    @property
    def nonce(self):
        return self.header.nonce

    @property
    def transactions(self):
        if self.transactions_ is None:
            self.populate_transactions()
        return self.transactions_

    def populate_transactions(self):
        assert self.depth_ is not None or self.hash_ is not None
        if self.depth_ is not None:
            tx_hashes = \
                self.client.block_transaction_hashes_by_depth(self.depth_)
        elif self.hash_ is not None:
            tx_hashes = \
                self.client.block_transaction_hashes_by_hash(self.hash_)
        self.transactions_ = []
        for offset, tx_hash in enumerate(tx_hashes):
            tx = Transaction(self.client, tx_hash, self, self.depth_, offset)
            self.transactions_.append(tx)

    @property
    def previous_block(self):
        if self.depth == 0:
            raise IndexError("No previous block exists.")
        return blocks[self.previous_block_hash]

    @property
    def next_block(self):
        next_depth = self.depth + 1
        if next_depth >= len(blocks):
            raise IndexError("No next block exists (yet).")
        blk = blocks[next_depth]
        if blk.previous_block_hash != self.hash:
            raise IndexError("Next block not found.")
        return blk

class TransactionsManager:

    def __init__(self, client):
        self.client = client

    def __getitem__(self, index):
        if type(index) != str:
            raise IndexError("Bad index.")
        if len(index) == 64:
            hash = index.decode("hex")
        elif len(index) == 32:
            hash = index
        else:
            raise IndexError("Bad index.")
        return Transaction(self.client, hash, None, None, None)

class Transaction:

    def __init__(self, client, hash, parent_block, depth, offset):
        self.client = client
        self.hash = hash
        self.body = None
        self.parent_block_ = parent_block
        self.depth_ = depth
        self.offset_ = offset
        self.inputs_ = None
        self.outputs_ = None

    def __repr__(self):
        return "<Transaction %s>" % self.hash.encode("hex")[:10]

    @property
    def depth(self):
        if self.depth_ is None:
            self.set_depth_offset()
        return self.depth_

    @property
    def offset(self):
        if self.offset_ is None:
            self.set_depth_offset()
        return self.offset_

    @property
    def parent_block(self):
        if self.parent_block_ is None:
            self.parent_block_ = blocks[self.depth]
        return self.parent_block_

    def lazy_init_body(self):
        if self.body is None:
            self.body = self.client.transaction(self.hash)

    @property
    def version(self):
        self.lazy_init_body()
        return self.body.version
    @property
    def locktime(self):
        self.lazy_init_body()
        return self.body.locktime
    @property
    def inputs(self):
        self.lazy_init_body()
        if self.inputs_ is None:
            self.populate_inputs()
        return self.inputs_
    @property
    def outputs(self):
        self.lazy_init_body()
        if self.outputs_ is None:
            self.populate_outputs()
        return self.outputs_

    def set_depth_offset(self):
        assert self.hash is not None
        tx_index = self.client.transaction_index(self.hash)
        self.depth_ = tx_index.depth
        self.offset_ = tx_index.offset

    def populate_inputs(self):
        self.inputs_ = []
        for index, body_input in enumerate(self.body.inputs):
            input = Input(self.client, self.hash, index, body_input, self)
            self.inputs_.append(input)

    def populate_outputs(self):
        self.outputs_ = []
        for index, body_output in enumerate(self.body.outputs):
            output = Output(self.client, self.hash, index, body_output, self)
            self.outputs_.append(output)

class Input:

    def __init__(self, client, hash, index, body, parent_tx):
        self.client = client
        self.hash = hash
        self.index = index
        self.body = body
        self.parent_tx_ = parent_tx

    def __repr__(self):
        return "<Input %s:%s>" % (self.hash.encode("hex")[:10], self.index)

    def lazy_init_body(self):
        if self.body is None:
            self.body = self.client.transaction(self.hash).inputs[self.index]

    @property
    def script(self):
        self.lazy_init_body()
        return self.body.input_script
    @property
    def sequence(self):
        self.lazy_init_body()
        return self.body.sequence
    @property
    def previous_output(self):
        prevout = self.previous_outpoint
        output = Output(self.client, prevout.hash, prevout.index,
                        None, None)
        return output

    @property
    def previous_outpoint(self):
        self.lazy_init_body()
        return self.body.previous_output

    @property
    def is_coinbase(self):
        prevout = self.previous_outpoint
        return prevout.hash == chr(0) * 32 and prevout.index == -1

    @property
    def parent_tx(self):
        if self.parent_tx_ is None:
            self.parent_tx_ = transactions[self.hash]
        return self.parent_tx_

class OutputsManager:

    def __init__(self, client):
        self.client = client

    def __getitem__(self, address):
        if type(address) != str:
            raise IndexError("Bad address supplied.")
        outputs = []
        outpoints = self.client.outputs(address)
        for outpoint in outpoints:
            output = Output(self.client, outpoint.hash, outpoint.index,
                            None, None)
            outputs.append(output)
        return outputs

class Output:

    def __init__(self, client, hash, index, body, parent_tx):
        self.client = client
        self.hash = hash
        self.index = index
        self.body = body
        self.parent_tx_ = parent_tx
        # None represents value not set, False represents no spend exists
        self.spend_ = None

    def __repr__(self):
        return "<Output %s:%s>" % (self.hash.encode("hex")[:10], self.index)

    def lazy_init_body(self):
        if self.body is None:
            self.body = self.client.transaction(self.hash).outputs[self.index]

    @property
    def value(self):
        self.lazy_init_body()
        return self.body.value

    @property
    def script(self):
        self.lazy_init_body()
        return self.body.output_script

    @property
    def parent_tx(self):
        if self.parent_tx_ is None:
            self.parent_tx_ = transactions[self.hash]
        return self.parent_tx_

    @property
    def spend(self):
        if self.spend_ is None:
            outpoint = csOutputPoint()
            outpoint.hash = self.hash
            outpoint.index = self.index
            try:
                inpoint = self.client.spend(outpoint)
            except:
                self.spend_ = False
            else:
                self.spend_ = Input(self.client, inpoint.hash, inpoint.index,
                                    None, None)
        return self.spend_

# Make socket
transport = TSocket.TSocket('localhost', 9090)
# Buffering is critical. Raw sockets are very slow
transport = TTransport.TBufferedTransport(transport)
# Wrap in a protocol
protocol = TBinaryProtocol.TBinaryProtocol(transport)
# Create a client to use the protocol encoder
client = csBlockchainService.Client(protocol)
# Connect!
transport.open()

blocks = BlocksManager(client)
transactions = TransactionsManager(client)
outputs = OutputsManager(client)

