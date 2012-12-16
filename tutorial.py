texts = [
"""Welcome to the tutorial. To get started, import the blockchain module:

    > import blockchain

Use next() to move to the next help.
""",

"""The blockchain module gives us access to Bitcoin's blockchain in an easy
and intuitive interface. Blocks can be indexed using either their hash or
block number:

    > blockchain.blocks[110]
    > bh = blockchain.blocks[110].hash
    > bh.encode("hex")
""",

"""Either the human readable or raw hash bytes are accepted for indexing
blocks, transactions and other data. The statements below are equivalent.

    > blockchain.blocks[bh]
    > blockchain.blocks[bh.encode("hex")]
""",

"""How many blocks are in our blockchain?

    > len(blockchain.blocks)
""",

"""We can move forwards and backwards in the chain as needed, and access
block attributes we need.

    > b = blockchain.blocks[110]
    > b.previous_block
    > b.next_block
    > b.next_block.next_block.next_block
    > b.depth, b.hash, b.merkle, b.nonce, b.timestamp
""",

"""We have access to transactions too through either the block or the
blockchain interface.

    > b.transactions
    > tx = b.transactions[0]
    > tx.hash
    > blockchain.transactions[tx.hash].hash
""",

"""Through the transaction, we can lookup various fields, the inputs, and
outputs.

    > tx.inputs[0].is_coinbase
    > tx.inputs[0].previous_output
    > tx.outputs
    > tx.outputs[0].value
    > tx.outputs[0].spend
""",

"""Lets find the transaction history for the Bitcoin address
12Big2VKYfq5UE46iHrSCRZjDUx9aDVGpu by looking up the outputs (or credited
funds) and inputs (or debited funds).

    > blockchain.outputs["12Big2VKYfq5UE46iHrSCRZjDUx9aDVGpu"]
    > outputs = blockchain.outputs['12Big2VKYfq5UE46iHrSCRZjDUx9aDVGpu']
    > inputs = [output.spend for output in outputs]

The last line gives us a list of each corresponding spend for each output
in out list. If an output is unspent then False is returned instead.
""",

"""Every output either has 1 or 0 inputs associated with it:

    > outputs[0]
    > outputs[0].spend
    > outputs[0].spend.previous_output
""",

"""To check the balance of the address, we need to total the money coming in
and minus the money going out.

    > sum(output.value for output in outputs)

For the inputs, that's slightly more complicated since Bitcoin doesn't store
any values in the input. Instead you must lookup the previous output and then
read the amount coming into the input.

    > sum(input.previous_output.value for input in inputs)

The total balance of this address is 0. All the money has been spent.
That's because there are no unspent outputs.
""",

"""We can find the chronology of inputs and outputs by looking up their
parent transaction, and then the timestamp of the parent block.

    > outputs[0].parent_tx.parent_block.timestamp

Thanks for listening :)
""",
]

def next():
    global texts
    print texts[0]
    texts = texts[1:] + texts[0:1]

next()

