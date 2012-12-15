default: server

schain.o: sync_blockchain.cpp
	g++ -c -ggdb -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -o schain.o -Igen-cpp sync_blockchain.cpp `pkg-config --cflags --libs thrift libbitcoin`

gensrc1.o: chain_service.thrift
	g++ -c -ggdb -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -o gensrc1.o -Igen-cpp gen-cpp/chain_service_types.cpp `pkg-config --cflags --libs thrift libbitcoin`
gensrc2.o: chain_service.thrift
	g++ -c -ggdb -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -o gensrc2.o -Igen-cpp gen-cpp/csBlockchainService.cpp `pkg-config --cflags --libs thrift libbitcoin`

server: query.cpp schain.o gensrc1.o gensrc2.o
	g++ -ggdb -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -o query-server -Igen-cpp schain.o gensrc1.o gensrc2.o query.cpp `pkg-config --cflags --libs thrift libbitcoin`

