#include "sync_blockchain.hpp"

#include <future>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

sync_blockchain::sync_blockchain(blockchain& chain)
  : chain_(chain)
{
}

template<typename ReturnType, typename FetchFunc, typename IndexType>
ReturnType get_impl(FetchFunc fetch, IndexType index, std::error_code& ec)
{
    std::promise<std::error_code> ec_promise;
    std::promise<ReturnType> obj_promise;
    
    auto handle =
        [&ec_promise, &obj_promise]
            (const std::error_code& ec, const ReturnType& obj)
        {
            ec_promise.set_value(ec);
            obj_promise.set_value(obj);
        };
    fetch(index, handle);
    ec = ec_promise.get_future().get();
    return obj_promise.get_future().get();
}

template <typename IndexType>
message::block block_header_impl(blockchain& chain,
    IndexType index, std::error_code& ec)
{
    std::promise<std::error_code> ec_promise;
    std::promise<message::block> block_header_promise;

    auto handle_block_header =
        [&ec_promise, &block_header_promise]
            (const std::error_code& ec, const message::block& blk)
        {
            ec_promise.set_value(ec);
            block_header_promise.set_value(blk);
        };
    chain.fetch_block_header(index, handle_block_header);
    ec = ec_promise.get_future().get();
    return block_header_promise.get_future().get();
}

message::block sync_blockchain::block_header(size_t depth) const
{
    std::error_code discard_ec;
    return block_header(depth, discard_ec);
}
message::block sync_blockchain::block_header(size_t depth,
    std::error_code& ec) const
{
    return block_header_impl(chain_, depth, ec);
}

message::block sync_blockchain::block_header(
    const hash_digest& block_hash) const
{
    std::error_code discard_ec;
    return block_header(block_hash, discard_ec);
}
message::block sync_blockchain::block_header(const hash_digest& block_hash,
    std::error_code& ec) const
{
    return block_header_impl(chain_, block_hash, ec);
}

template <typename IndexType>
message::inventory_list block_tx_hashes_impl(blockchain& chain,
    IndexType index, std::error_code& ec)
{
    std::promise<std::error_code> ec_promise;
    std::promise<message::inventory_list> hashes_promise;

    auto handle_tx_hashes =
        [&ec_promise, &hashes_promise]
            (const std::error_code& ec, const message::inventory_list& hashes)
        {
            ec_promise.set_value(ec);
            hashes_promise.set_value(hashes);
        };
    chain.fetch_block_transaction_hashes(index, handle_tx_hashes);
    ec = ec_promise.get_future().get();
    return hashes_promise.get_future().get();
}

message::inventory_list sync_blockchain::block_transaction_hashes(
    size_t depth) const
{
    std::error_code discard_ec;
    return block_transaction_hashes(depth, discard_ec);
}
message::inventory_list sync_blockchain::block_transaction_hashes(
    size_t depth, std::error_code& ec) const
{
    return block_tx_hashes_impl(chain_, depth, ec);
}

message::inventory_list sync_blockchain::block_transaction_hashes(
    const hash_digest& block_hash) const
{
    std::error_code discard_ec;
    return block_transaction_hashes(block_hash, discard_ec);
}
message::inventory_list sync_blockchain::block_transaction_hashes(
    const hash_digest& block_hash, std::error_code& ec) const
{
    return block_tx_hashes_impl(chain_, block_hash, ec);
}

size_t sync_blockchain::block_depth(const hash_digest& block_hash) const
{
    std::error_code discard_ec;
    return block_depth(block_hash, discard_ec);
}
size_t sync_blockchain::block_depth(const hash_digest& block_hash,
    std::error_code& ec) const
{
    return get_impl<size_t>(
        std::bind(&blockchain::fetch_block_depth, &chain_, _1, _2),
        block_hash, ec);
}

size_t sync_blockchain::last_depth() const
{
    std::error_code discard_ec;
    return last_depth(discard_ec);
}
size_t sync_blockchain::last_depth(std::error_code& ec) const
{
    // We discard the index since it isn't used for fetching the last depth.
    // get_impl expects an index value so we give it a value to discard.
    return get_impl<size_t>(
        std::bind(&blockchain::fetch_last_depth, &chain_, _2),
        0, ec);
}

message::transaction sync_blockchain::transaction(
    const hash_digest& transaction_hash) const
{
    std::error_code discard_ec;
    return transaction(transaction_hash, discard_ec);
}
message::transaction sync_blockchain::transaction(
    const hash_digest& transaction_hash, std::error_code& ec) const
{
    return get_impl<message::transaction>(
        std::bind(&blockchain::fetch_transaction, &chain_, _1, _2),
        transaction_hash, ec);
}

transaction_index_t sync_blockchain::transaction_index(
    const hash_digest& transaction_hash) const
{
    std::error_code discard_ec;
    return transaction_index(transaction_hash, discard_ec);
}
transaction_index_t sync_blockchain::transaction_index(
    const hash_digest& transaction_hash, std::error_code& ec) const
{
    std::promise<std::error_code> ec_promise;
    std::promise<transaction_index_t> tx_index_promise;

    auto handle_tx_index =
        [&ec_promise, &tx_index_promise]
            (const std::error_code& ec, size_t depth, size_t offset)
        {
            ec_promise.set_value(ec);
            tx_index_promise.set_value({depth, offset});
        };
    chain_.fetch_transaction_index(transaction_hash, handle_tx_index);
    ec = ec_promise.get_future().get();
    return tx_index_promise.get_future().get();
}

message::input_point sync_blockchain::spend(
    const message::output_point& outpoint) const
{
    std::error_code discard_ec;
    return spend(outpoint, discard_ec);
}
message::input_point sync_blockchain::spend(
    const message::output_point& outpoint, std::error_code& ec) const
{
    return get_impl<message::input_point>(
        std::bind(&blockchain::fetch_spend, &chain_, _1, _2),
        outpoint, ec);
}

message::output_point_list sync_blockchain::outputs(
    const payment_address& address) const
{
    std::error_code discard_ec;
    return outputs(address, discard_ec);
}
message::output_point_list sync_blockchain::outputs(
    const payment_address& address, std::error_code& ec) const
{
    return get_impl<message::output_point_list>(
        std::bind(&blockchain::fetch_outputs, &chain_, _1, _2),
        address, ec);
}

