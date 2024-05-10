// A sample scenario for VODSim
//
0h0m0s: simulation set random seed time
0h0m0s: simulation set id time
//
//
0h0m0s: simulation set folder input/bittorrent/torrent-data
0h0m0s: simulation set file input/bittorrent/torrent-data/10MB-full.dat.torrent
0h0m0s: topology set file input/bittorrent/topology/dfn-like-large.brite
0h0m0s: topology add tracker
0h0m0s: topology set delays min 0 max 0
//
//
0h0m0s: topology add clients count 1 type point-to-point uplink 1000Mbps downlink 1000Mbps delay 0ms
0h0m0s: client 1 join group CDN
0h0m0s: topology add clients count 100 type point-to-point uplink 1000Mbps downlink 100Mbps delay 0ms
0h0m0s: clients 2 to 101 join group PCDN
//0h0m0s: group CDN leave group PCDN
0h0m0s: topology add clients count 2000 type point-to-point uplink 448kbps downlink 10Mbps delay 0ms
0h0m0s: clients 102 to 2101 join group leechers
0h0m0s: clients 102 to 301 set streamhash 2
0h0m0s: clients 302 to 501 set streamhash 3
0h0m0s: clients 502 to 701 set streamhash 4
0h0m0s: clients 702 to 901 set streamhash 5
0h0m0s: clients 902 to 1101 set streamhash 6
0h0m0s: clients 1102 to 1301 set streamhash 7
0h0m0s: clients 1302 to 1501 set streamhash 8
0h0m0s: clients 1502 to 1701 set streamhash 9
0h0m0s: clients 1702 to 1901 set streamhash 10
0h0m0s: clients 1902 to 2101 set streamhash 11
//0h0m0s: clients 80 to 101 set streamhash 8
0h0m0s: tracker set streamhash 2
0h0m0s: tracker set streamhash 3
0h0m0s: tracker set streamhash 4
0h0m0s: tracker set streamhash 5
0h0m0s: tracker set streamhash 6
0h0m0s: tracker set streamhash 7
0h0m0s: tracker set streamhash 8
0h0m0s: tracker set streamhash 9
0h0m0s: tracker set streamhash 10
0h0m0s: tracker set streamhash 11
//0h0m0s: group PCDN leave group leechers
//0h0m0s: group CDN leave group leechers
//
//
0h0m0s: group PCDN set initial bitfield full
0h0m0s: group leechers set initial bitfield empty
//
//
0h0m0s: all clients set block size request 16384 // e.g., 524288 262144 131072 32768 16384
0h0m0s: all clients set block size send 16384 // e.g., 16384
// 0h0m0s: all clients set piece max requests 1
// 0h0m0s: all clients set concurrent requests 8
0h0m0s: all clients set piece timeout 0h0m30s
//
//
0h0m0s: all clients set autoconnect 1
0h0m0s: all clients set peers 30
0h0m0s: all clients set max peers 55
//
//
0h0m0s: group PCDN set unchoked peers 3
0h0m0s: group CDN set clienttype CDN
0h0m0s: group PCDN set clienttype PCDN
0h0m0s: group leechers set clienttype CLIENT
//
//
0h0m0s: all clients set strategy live-streaming-box
0h0m0s: group leechers set autoplay 0
// The following four lines have no effect if VOD simulation is deactivated
// (e.g., when autoplay is off and "play" is never requested).
0h0m0s: group leechers set video skip inactive
0h0m0s: group leechers set video skip tolerance 0h0m2s
0h0m0s: group leechers set video skip afterwards 0h0m15s
0h0m0s: all clients set video prebuffering 0h0m15s
//
//
0h0m0s: tracker set update interval 0h0m3s
0h0m0s: tracker set strategy random
//
//

0h0m0s: client 102 leave group leechers
0h0m0s: client 302 leave group leechers
0h0m0s: client 502 leave group leechers
0h0m0s: client 702 leave group leechers
0h0m0s: client 902 leave group leechers
0h0m0s: client 1102 leave group leechers
0h0m0s: client 1302 leave group leechers
0h0m0s: client 1502 leave group leechers
0h0m0s: client 1702 leave group leechers
0h0m0s: client 1902 leave group leechers

0h0m0s: client 102  join group starters
0h0m0s: client 302  join group starters
0h0m0s: client 502  join group starters
0h0m0s: client 702  join group starters
0h0m0s: client 902  join group starters
0h0m0s: client 1102 join group starters
0h0m0s: client 1302 join group starters
0h0m0s: client 1502 join group starters
0h0m0s: client 1702 join group starters
0h0m0s: client 1902 join group starters

0h0m0s: group PCDN init
0h0m0s: group CDN init
0h0m5s: group starters init
from 0h0m12s until 0h1m17s group leechers init

//
// 0h0m16s: client 6 leave cloud
// 0h0m26s: client 6 rejoin cloud
