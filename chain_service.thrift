#!/usr/bin/thrift --gen cpp

// Mapped internally from std::error_code
exception csErrorCode {
  1: i32 what,
  2: string why
}

struct csBlockHeader {
  1: i64 version,
  2: binary previous_block_hash,
  3: binary merkle,
  4: i64 timestamp,
  5: i64 bits,
  6: i64 nonce
}

typedef list<binary> csHashList

struct csOutputPoint {
  1: binary hash,
  2: i32 index
}

typedef csOutputPoint csInputPoint
typedef list<csOutputPoint> csOutputPointList

struct csTransactionInput {
  1: csOutputPoint previous_output,
  2: binary input_script,
  3: i64 sequence
}

// signed 64 bit can hold the max_money for bitcoins of 21 million
struct csTransactionOutput {
  1: i64 value,
  2: binary output_script
}

struct csTransaction {
  1: i64 version,
  2: i64 locktime,
  3: list<csTransactionInput> inputs,
  4: list<csTransactionOutput> outputs
}

struct csTransactionIndex {
  1: i32 depth,
  2: i32 offset
}

service csBlockchainService {
  csBlockHeader block_header_by_depth(1:i32 depth)
  csBlockHeader block_header_by_hash(1:binary hash)
  csHashList block_transaction_hashes_by_depth(1:i32 depth)
  csHashList block_transaction_hashes_by_hash(1:binary hash)
  i32 block_depth(1:binary hash)
  i32 last_depth()
  csTransaction transaction(1:binary hash)
  csTransactionIndex transaction_index(1:binary hash)
  csInputPoint spend(1:csOutputPoint outpoint)
  csOutputPointList outputs(1:string address)
}

