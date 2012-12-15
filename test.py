import blockchain
print blockchain.blocks[0].merkle.encode("hex")
print blockchain.blocks[0].hash.encode("hex")
print blockchain.blocks[110].depth
print blockchain.blocks[2].transactions

tx = blockchain.blocks[2].transactions[0]
print tx.depth, tx.offset
print "tx hash:", tx.hash.encode("hex")
print blockchain.blocks.count

tx = blockchain.transactions["9b0fc92260312ce44e74ef369f5c66bbb85848f2eddd5a7a1cde251e54ccfdd5"]
print tx.parent_block.hash.encode("hex")
print blockchain.blocks[2].hash.encode("hex")
print tx.locktime, tx.version
print tx.inputs
print tx.inputs[0].sequence, tx.inputs[0].script.encode("hex")
print tx.inputs[0].previous_output

tx = blockchain.blocks[100000].transactions[1]
print tx.outputs[0].value
print tx.outputs[0].parent_tx.outputs[0].parent_tx.outputs[0].value
print tx.inputs[0].previous_output.value, "->", sum([out.value for out in tx.outputs])
print tx.inputs[0].previous_output.spend.previous_output.value
print blockchain.blocks[0].transactions[0].outputs[0].spend

print blockchain.outputs["12Big2VKYfq5UE46iHrSCRZjDUx9aDVGpu"]
print blockchain.blocks[110]

