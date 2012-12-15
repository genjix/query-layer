#include <future>

#include <bitcoin/bitcoin.hpp>
using namespace bc;

#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

#include "sync_blockchain.hpp"

#include "gen-cpp/csBlockchainService.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

void error_exit(const std::string& message, int status=1)
{
    log_error() << "session: " << message;
    exit(status);
}

void blockchain_started(const std::error_code& ec)
{
    if (ec)
        error_exit(ec.message());
    else
        log_info() << "Blockchain initialized!";
}

template <typename T>
std::string to_binary(const T& bytes)
{
    std::string binary;
    extend_data(binary, bytes);
    return binary;
}

class blockchain_service_handler
  : public csBlockchainServiceIf
{
public:
    blockchain_service_handler(blockchain& chain);

    void block_header_by_depth(csBlockHeader& blk, const int32_t depth);
    void block_header_by_hash(csBlockHeader& blk, const std::string& hash);
    void block_transaction_hashes_by_depth(
        csHashList& tx_hashes, const int32_t depth);
    void block_transaction_hashes_by_hash(
        csHashList& tx_hashes, const std::string& hash);
    int32_t block_depth(const std::string& hash);
    int32_t last_depth();
    void transaction(csTransaction& tx, const std::string& hash);
    void transaction_index(
        csTransactionIndex& tx_index, const std::string& hash);
    void spend(csInputPoint& inpoint, const csOutputPoint& outpoint);
    void outputs(csOutputPointList& outpoints, const std::string& address);

private:
    sync_blockchain chain_;
};

blockchain_service_handler::blockchain_service_handler(blockchain& chain)
  : chain_(chain)
{
}

void check_errc(const std::error_code& ec)
{
    if (ec)
    {
        csErrorCode except;
        except.what = 0;
        except.why = ec.message();
        throw except;
    }
}

template<typename IndexType>
void block_header_impl(sync_blockchain& chain,
    csBlockHeader& blk, IndexType index)
{
    std::error_code ec;
    auto b = chain.block_header(index, ec);
    check_errc(ec);
    blk.version = b.version;
    blk.timestamp = b.timestamp;
    blk.previous_block_hash = to_binary(b.previous_block_hash);
    blk.merkle = to_binary(b.merkle);
    blk.bits = b.bits;
    blk.nonce = b.nonce;
}

void blockchain_service_handler::block_header_by_depth(
    csBlockHeader& blk, const int32_t depth)
{
    block_header_impl(chain_, blk, depth);
}

hash_digest proper_hash(const std::string& hash_str)
{
    hash_digest hash;
    if (hash_str.size() != hash.size())
    {
        csErrorCode except;
        except.what = 0;
        except.why = "Invalid hash";
        throw except;
    }
    std::copy(hash_str.begin(), hash_str.end(), hash.begin());
    return hash;
}

void blockchain_service_handler::block_header_by_hash(
    csBlockHeader& blk, const std::string& hash)
{
    block_header_impl(chain_, blk, proper_hash(hash));
}

template <typename IndexType>
void block_tx_hashes_impl(
    sync_blockchain& chain, csHashList& tx_hashes, IndexType index)
{
    std::error_code ec;
    auto txs = chain.block_transaction_hashes(index, ec);
    check_errc(ec);
    for (const auto& inv: txs)
    {
        BITCOIN_ASSERT(inv.type == message::inventory_type::transaction);
        tx_hashes.push_back(to_binary(inv.hash));
    }
}

void blockchain_service_handler::block_transaction_hashes_by_depth(
    csHashList& tx_hashes, const int32_t depth)
{
    block_tx_hashes_impl(chain_, tx_hashes, depth);
}
void blockchain_service_handler::block_transaction_hashes_by_hash(
    csHashList& tx_hashes, const std::string& hash)
{
    block_tx_hashes_impl(chain_, tx_hashes, proper_hash(hash));
}

int32_t blockchain_service_handler::block_depth(const std::string& hash)
{
    std::error_code ec;
    auto depth = chain_.block_depth(proper_hash(hash), ec);
    check_errc(ec);
    return depth;
}
int32_t blockchain_service_handler::last_depth()
{
    std::error_code ec;
    auto depth = chain_.last_depth(ec);
    check_errc(ec);
    return depth;
}

void blockchain_service_handler::transaction(
    csTransaction& tx, const std::string& hash)
{
    std::error_code ec;
    auto tmp_tx = chain_.transaction(proper_hash(hash), ec);
    check_errc(ec);
    tx.version = tmp_tx.version;
    tx.locktime = tmp_tx.locktime;
    for (const auto& tx_input: tmp_tx.inputs)
    {
        csTransactionInput in;
        in.previous_output.hash = to_binary(tx_input.previous_output.hash);
        in.previous_output.index = tx_input.previous_output.index;
        in.input_script = to_binary(save_script(tx_input.input_script));
        in.sequence = tx_input.sequence;
        tx.inputs.push_back(in);
    }
    for (const auto& tx_output: tmp_tx.outputs)
    {
        csTransactionOutput out;
        out.value = tx_output.value;
        out.output_script = to_binary(save_script(tx_output.output_script));
        tx.outputs.push_back(out);
    }
}

void blockchain_service_handler::transaction_index(
    csTransactionIndex& tx_index, const std::string& hash)
{
    std::error_code ec;
    auto tidx = chain_.transaction_index(proper_hash(hash), ec);
    check_errc(ec);
    tx_index.depth = tidx.depth;
    tx_index.offset = tidx.offset;
}

void blockchain_service_handler::spend(
    csInputPoint& inpoint, const csOutputPoint& outpoint)
{
    std::error_code ec;
    message::output_point out{
        proper_hash(outpoint.hash), (uint32_t)outpoint.index};
    auto ipt = chain_.spend(out, ec);
    check_errc(ec);
    inpoint.hash = to_binary(ipt.hash);
    inpoint.index = ipt.index;
}

void blockchain_service_handler::outputs(
    csOutputPointList& outpoints, const std::string& address)
{
    std::error_code ec;
    auto outs = chain_.outputs(address, ec);
    check_errc(ec);
    for (const auto& out: outs)
    {
        csOutputPoint outpoint;
        outpoint.hash = to_binary(out.hash);
        outpoint.index = out.index;
        outpoints.push_back(outpoint);
    }
}

int main()
{
    async_service service(1);
    bdb_blockchain chain(service);
    chain.start("database", blockchain_started);

    boost::shared_ptr<TProtocolFactory> protocol_factory(
        new TBinaryProtocolFactory());
    boost::shared_ptr<blockchain_service_handler> handler(
        new blockchain_service_handler(chain));
    boost::shared_ptr<TProcessor> processor(
        new csBlockchainServiceProcessor(handler));
    boost::shared_ptr<TServerTransport> server_transport(
        new TServerSocket(9090));
    boost::shared_ptr<TTransportFactory> transport_factory(
        new TBufferedTransportFactory());

    TSimpleServer server(processor, server_transport,
        transport_factory, protocol_factory);

    log_info() << "Starting the server...";
    server.serve();

    // Finish.
    service.stop();
    service.join();
    chain.stop();

    return 0;
}

