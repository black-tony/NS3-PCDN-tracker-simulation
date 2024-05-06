check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(bittorrent_with_tap_source)
set(bittorrent_with_tap_header)
set(bittorrent_with_tap_lib)
if(${ENABLE_TAP})
    set(bittorrent_with_tap_source
        helper/brite-tap-helper.cc
    )
    set(bittorrent_with_tap_header
        helper/brite-tap-helper.h
    )
    set(bittorrent_with_tap_lib
        ${libtap-bridge}
    )
else()
    set(bittorrent_with_tap_source
        helper/no-tap/brite-tap-helper.cc
        )
    set(bittorrent_with_tap_source
            helper/no-tap/brite-tap-helper.h
        )
endif()

build_lib(
    LIBNAME bittorrent
    SOURCE_FILES    model/common/3rd-party/sha1.cc
                    model/common/BitTorrentUtilities.cc
                    model/common/GlobalMetricsGatherer.cc
                    model/common/Torrent.cc
                    model/common/TorrentFile.cc
                    ## Client ##
                    model/client/AbstractStrategy.cc
                    model/client/BitTorrentClient.cc
                    model/client/BitTorrentVideoClient.cc
                    model/client/BitTorrentHttpClient.cc
                    model/client/BitTorrentPacket.cc
                    model/client/BitTorrentPeer.cc
                    model/client/BitTorrentVideoMetricsBase.cc
                    model/client/ChokeUnChokeStrategyBase.cc
                    model/client/PartSelectionStrategyBase.cc
                    model/client/PeerConnectorStrategyBase.cc
                    model/client/ProtocolFactory.cc
                    model/client/RequestSchedulingStrategyBase.cc
                    model/client/StorageManager.cc
                    model/client/strategies/RarestFirstPartSelectionStrategy.cc
                    model/client/streaming/PeerConnectorStrategyLive.cc
                    model/client/streaming/RequestSchedulingStrategyLive.cc
                    #model/client/strategies/vod/bitos/BiToS-PartSelectionStrategy.cc
                    #model/client/strategies/vod/gtg/GTG-ChokeUnChokeStrategy.cc
                    #model/client/strategies/vod/gtg/GTG-PartSelectionStrategy.cc
                    #model/client/strategies/vod/rf-vod/RF-VoD-PartSelectionStrategy.cc
                    ## Tracker ##
                    model/tracker/BitTorrentTracker.cc
                    model/tracker/BitTorrentHttpServer.cc
                    ## Helpers ##
                    helper/VODSimBriteTopologyHelper.cc
                    helper/Story.cc
                    ${bittorrent_with_tap_source}
    HEADER_FILES    model/common/3rd-party/sha1.h
                    model/common/BitTorrentDefines.h
                    model/common/BitTorrentUtilities.h
                    model/common/GlobalMetricsGatherer.h
                    model/common/Torrent.h
                    model/common/TorrentFile.h
                    ## Client ##
                    model/client/AbstractStrategy.h
                    model/client/BitTorrentClient.h
                    model/client/BitTorrentVideoClient.h
                    model/client/BitTorrentHttpClient.h
                    model/client/BitTorrentPacket.h
                    model/client/BitTorrentPeer.h
                    model/client/BitTorrentVideoMetricsBase.h
                    model/client/ChokeUnChokeStrategyBase.h
                    model/client/PartSelectionStrategyBase.h
                    model/client/PeerConnectorStrategyBase.h
                    model/client/ProtocolFactory.h
                    model/client/RequestSchedulingStrategyBase.h       
                    model/client/StorageManager.h
                    model/client/strategies/RarestFirstPartSelectionStrategy.h
                    model/client/streaming/PeerConnectorStrategyLive.h
                    model/client/streaming/RequestSchedulingStrategyLive.h
                    #model/client/strategies/vod/bitos/BiToS-PartSelectionStrategy.h
                    #model/client/strategies/vod/gtg/GTG-ChokeUnChokeStrategy.h
                    #model/client/strategies/vod/gtg/GTG-PartSelectionStrategy.h
                    #model/client/strategies/vod/rf-vod/RF-VoD-PartSelectionStrategy.h
                    ## Tracker ##
                    model/tracker/BitTorrentTracker.h
                    model/tracker/BitTorrentHttpServer.h
                    ## Helpers ##
                    helper/VODSimBriteTopologyHelper.h
                    helper/Story.h
                    ${bittorrent_with_tap_header}
    LIBRARIES_TO_LINK ${libcore}
                      ${libinternet}
                      ${libnetwork}
                      ${libnix-vector-routing}
                      ${libpoint-to-point}
                      ${libtopology-read}
                      ${libcsma}
                      ${libapplications}
                      ${bittorrent_with_tap_lib}
)
