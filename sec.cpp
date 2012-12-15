#include <bitcoin/bitcoin.hpp>
using namespace bc;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

typedef std::vector<std::string> string_list;

struct history_entry
{
    hash_digest tx_hash;
    bool is_input;
    size_t index;
    size_t height;
    hash_digest block_hash;
    uint64_t timestamp;
    string_list outputs;
};

typedef std::vector<history_entry> history_info;

typedef std::function<void (const std::error_code&, const history_info&)>
    history_handler;

class query_history
{
public:
    query_history(async_service& service, blockchain& chain);
    void start(const payment_address& payaddr,
        history_handler handle);

private:
    void stop(const std::error_code& ec, const history_info& history);

    void handle_outputs(const std::error_code& ec,
        const message::output_point_list& outpoints);
    // is_input
    // tx_hash
    // height
    // block_hash
    // timestamp

    // index
    // value
    // outputs
    // inputs
    void load_details();
    void add_depth(const std::error_code& ec,
        size_t block_depth, size_t offset);
    void add_block_hash(const std::error_code& ec,
        const message::block& block_header);
    void read_tx(const std::error_code& ec, const message::transaction& tx);
    void add_prevout_addr();
    void store_spend(const std::error_code& ec,
        const message::input_point& inpoint);

    async_service& service_;
    blockchain& chain_;
    io_service::strand strand_;
    payment_address payaddr_;
    history_handler handle_;

    history_info history_;
    message::output_point_list outpoints_;
    size_t outpoints_idx_;
    message::output_point_list prevouts_;
    size_t prevouts_idx_;
    message::input_point_list spends_;
};

query_history::query_history(async_service& service, blockchain& chain)
  : service_(service), chain_(chain), strand_(service.get_service())
{
}

void query_history::start(const payment_address& payaddr,
    history_handler handle)
{
    payaddr_ = payaddr;
    handle_ = handle;
    chain_.fetch_outputs(payaddr,
        strand_.wrap(std::bind(&query_history::handle_outputs,
            this, _1, _2)));
}

void query_history::stop(const std::error_code& ec,
    const history_info& history)
{
    handle_(ec, history);
    delete this;
}

void query_history::handle_outputs(const std::error_code& ec,
    const message::output_point_list& outpoints)
{
    if (ec)
    {
        log_error() << ec.message();
        stop(ec, history_info());
        return;
    }
    outpoints_ = outpoints;
    outpoints_idx_ = 0;
    load_details();
}

void query_history::load_details()
{
    if (outpoints_idx_ >= outpoints_.size())
    {
        for (const history_entry& entry: history_)
        {
            log_debug() << "tx_hash: " << entry.tx_hash;
            log_debug() << "is_input:"  << entry.is_input;
            log_debug() << "height: " << entry.height;
            log_debug() << "index: " << entry.index;
            log_debug() << "block_hash: " << entry.block_hash;
            log_debug() << "timestamp: " << entry.timestamp;
            for (const auto& out: entry.outputs)
                log_debug() << "  " << out;
            log_debug();
        }
        for (const auto& spend: spends_)
        {
            log_debug() << spend.hash << " " << spend.index;
        }
        stop(std::error_code(), history_);
        return;
    }
    const hash_digest& tx_hash = outpoints_[outpoints_idx_].hash;
    history_entry entry;
    entry.tx_hash = tx_hash;
    entry.is_input = false;
    entry.index = outpoints_[outpoints_idx_].index;
    history_.push_back(entry);
    chain_.fetch_transaction_index(tx_hash,
        strand_.wrap(std::bind(&query_history::add_depth,
            this, _1, _2, _3)));
    // start loading the tx
    // after finished, increment outpoints_idx_
    // call load_details again
}

void query_history::add_depth(const std::error_code& ec,
    size_t block_depth, size_t offset)
{
    history_.back().height = block_depth;
    chain_.fetch_block_header(block_depth,
        strand_.wrap(std::bind(&query_history::add_block_hash,
            this, _1, _2)));
}

void query_history::add_block_hash(const std::error_code& ec,
    const message::block& block_header)
{
    history_.back().block_hash = hash_block_header(block_header);
    history_.back().timestamp = block_header.timestamp;
    const hash_digest& tx_hash = history_.back().tx_hash;
    chain_.fetch_transaction(tx_hash,
        strand_.wrap(std::bind(&query_history::read_tx,
            this, _1, _2)));
}

void query_history::read_tx(const std::error_code& ec,
    const message::transaction& tx)
{
    BITCOIN_ASSERT(history_.back().index < tx.outputs.size());
    for (const message::transaction_output& output: tx.outputs)
    {
        payment_address payaddr;
        if (!extract(payaddr, output.output_script))
        {
        }
        history_.back().outputs.push_back(payaddr.encoded());
    }
    prevouts_idx_ = 0;
    for (const message::transaction_input& input: tx.inputs)
        prevouts_.push_back(input.previous_output);
    add_prevout_addr();
}

void query_history::add_prevout_addr()
{
    // find each input address, then move onto next outpoint.
    // fetch spend and try to load that if needed.
    chain_.fetch_spend(outpoints_[outpoints_idx_],
        strand_.wrap(std::bind(&query_history::store_spend,
            this, _1, _2)));
}

void query_history::store_spend(const std::error_code& ec,
    const message::input_point& inpoint)
{
    spends_.push_back(inpoint);
    outpoints_idx_++;
    load_details();
}

void payment_history(async_service& service, blockchain& chain,
    const std::string& lookup_addr, history_handler handle_finish)
{
    query_history* history = new query_history(service, chain);
    payment_address payaddr(lookup_addr);
    if (payaddr.type() != bc::payment_type::pubkey_hash)
    {
        log_error() << "Unhandled payment type";
        return;
    }
    log_info() << "Looking up " << lookup_addr;
    log_info();
    history->start(payaddr, handle_finish);
}

void lookup_finished(const std::error_code& ec, const history_info& history)
{
    if (ec)
        log_error() << ec.message();
    else
        log_debug() << "Finished.";
}

void blockchain_started(const std::error_code& ec,
    async_service& service, blockchain& chain,
    const std::string& lookup_addr)
{
    if (ec)
    {
        log_error() << "Blockchain problem!";
        return;
    }
    payment_history(service, chain, lookup_addr, lookup_finished);
}

int main(int argc, char** argv)
{
    if (argc < 2)
        return 1;
    async_service disk_service(1), query_service(1);
    bdb_blockchain chain(disk_service);
    //get_history = new get_history_action(query_service, chain);
    chain.start("/home/genjix/src/libbitcoin/database",
        std::bind(blockchain_started, _1,
            std::ref(query_service), std::ref(chain), argv[1]));
    std::cin.get();
    disk_service.stop();
    query_service.stop();
    disk_service.join();
    query_service.join();
    chain.stop();
    return 0;
}

