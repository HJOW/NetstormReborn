// 원본 TCP/IP 로컬 검색의66h 패킷·공유 난수·두 기한을 OS 장치와 분리한다.
#pragma once
#include "o/CrtRandom.h"
#include "o/RawLocalMissionState.h"
#include <array>
#include <functional>
#include <string>

namespace netstorm::o { // 실제 Client의 현재 저장소와 외부 UDP/창/팔레트 장치를 빌린다.
struct LocalServerDiscoveryGlobals {
    std::uint32_t& token; //005c85a8의 현재 검색 토큰이며 부호 있는 동시 검색 비교에도 같은32비트를 쓴다.
    double& searchEnd; //005c8600의 현재 검색 만료 기한이며 다른 검색자의 요청은 wall+12로 갱신한다.
    double& retryEnd; //005c8608의 별도 재송신 기한이며 검색 만료와 합치지 않는다.
};
struct LocalServerDiscoveryHooks {
    std::function<double()> wall; //00460d70의 실제 벽시각이며 호출 위치마다 새로 읽는다.
    std::function<void()> open; //004e2bd0의 실제 UDP12 열기이며 생성 성공을 합성하지 않는다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; //004b3fc0의 현재12BYTE를 실제 UDP 브로드캐스트 장치에 보낸다.
    std::function<void()> progress; //State34의 현재 번역된 Searching for local TCP/IP server 안내다.
    std::function<void()> palette; //State34의 chalPal 포맷/실제 Fort 팔레트 변경 부모다.
    std::function<bool()> serverWindowExists; //00435ab0의 Netstorm Root Server 제목을 가진 실제 창 검색이다.
};
std::array<std::uint8_t,12> MakeLocalServerDiscoveryPacket(std::uint32_t command,std::uint32_t value); //0040ae40의version1/type66/길이12와+4명령/+8값을 little endian으로 쓴다.
void AnnounceLocalServer(std::uint32_t basePort,const std::function<void(std::span<const std::uint8_t>)>& broadcast); //R.exe00475e60의실제UDP노드가있을때명령1/현재basePort를같은12BYTE로세번브로드캐스트한다.장치존재선택은caller가맡는다.
void BroadcastLocalServerDiscovery(LocalServerDiscoveryGlobals globals,const LocalServerDiscoveryHooks& hooks); //004b3fc0의현재 토큰으로 명령0을 작성한다. 장치 없음의 처리도 실제 broadcast 장치에 남긴다.
void BeginLocalServerDiscovery(LocalServerDiscoveryGlobals globals,CrtRandom& random,const LocalServerDiscoveryHooks& hooks); //004b4050의열기→wall×1000 srand→네 번 rand의lowBYTE→현재 토큰 저장→첫 송신 순서다.
void StartLocalServerSearch(LocalMissionStateStorage state,LocalServerDiscoveryGlobals globals,CrtRandom& random,const LocalServerDiscoveryHooks& hooks); //State34의안내→첫 검색→5초 기한→0.500~0.599초 재송신→팔레트→immediate[0]직접35 순서다.
void ContinueLocalServerSearch(LocalMissionStateStorage state,LocalServerDiscoveryGlobals globals,CrtRandom& random,const LocalServerDiscoveryHooks& hooks); //State35의재송신/만료/서버 창 확인/즉시37와다음35를 보존한다.
struct LocalServerDiscoveryReceiveHooks {
    std::function<double()> wall; //경쟁 검색의 지연 선택에서만 호출하는 현재 벽시각이다.
    std::function<std::string(std::uint32_t)> peerAddress; //현재 sender의실제20BYTE 주소를004e0a30(1,0,0)처럼 IP:port로 읽는다.
    std::function<void()> processState; //004b8a10 마지막004b6dd0의 실제 부모이며 현재36 작성 뒤에 호출한다.
};
void ReceiveLocalServerDiscovery(LocalMissionStateStorage state,LocalServerDiscoveryGlobals globals,std::uint32_t sender,std::span<const std::uint8_t> packet,const LocalServerDiscoveryReceiveHooks& hooks); //명령1이면현재36/주소/+0x30포트를쓰고부모를호출하며그외는signed토큰이작을때만검색을12초연장한다.
}
