#ifndef SYNC_BLOCKCHAIN_H
#define SYNC_BLOCKCHAIN_H

#include <bitcoin/blockchain/blockchain.hpp>
using namespace libbitcoin;

struct transaction_index_t
{
    size_t depth, offset;
};

class sync_blockchain
{
public:
    sync_blockchain(blockchain& chain);

    message::block block_header(size_t depth) const;
    message::block block_header(size_t depth, std::error_code& ec) const;

    message::block block_header(const hash_digest& block_hash) const;
    message::block block_header(const hash_digest& block_hash,
        std::error_code& ec) const;

    message::inventory_list block_transaction_hashes(
        size_t depth) const;
    message::inventory_list block_transaction_hashes(
        size_t depth, std::error_code& ec) const;

    message::inventory_list block_transaction_hashes(
        const hash_digest& block_hash) const;
    message::inventory_list block_transaction_hashes(
        const hash_digest& block_hash, std::error_code& ec) const;

    size_t block_depth(const hash_digest& block_hash) const;
    size_t block_depth(const hash_digest& block_hash,
        std::error_code& ec) const;

    size_t last_depth() const;
    size_t last_depth(std::error_code& ec) const;

    message::transaction transaction(
        const hash_digest& transaction_hash) const;
    message::transaction transaction(
        const hash_digest& transaction_hash, std::error_code& ec) const;

    transaction_index_t transaction_index(
        const hash_digest& transaction_hash) const;
    transaction_index_t transaction_index(
        const hash_digest& transaction_hash, std::error_code& ec) const;

    message::input_point spend(
        const message::output_point& outpoint) const;
    message::input_point spend(
        const message::output_point& outpoint, std::error_code& ec) const;

    message::output_point_list outputs(
        const payment_address& address) const;
    message::output_point_list outputs(
        const payment_address& address, std::error_code& ec) const;
private:
    blockchain& chain_;
};

#endif

