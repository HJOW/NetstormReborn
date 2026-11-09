// 실제 Session/World 상태를 기계가 읽는 TSV로 내보내는 검사 전용 관측이다. 원본 게임에는 없는 분석 기능이며 게임 상태를 바꾸지 않는다.
// webpj·dotnetpj의 L2(Load/Start 뒤 시작 상태)·L3(같은 입력 단계표 진행 뒤 상태) 대조에 쓴다.
// 사용: --run <자료> ... --ui-script steps.tsv 의 단계 "프레임⇥session-dump⇥출력경로" 가 그 프레임 그리기 뒤 현재 상태를 파일로 쓴다.
// 2026-11-09: webpj(LEFT_JOBS.webpj.md 3.5절 1번)·dotnetpj(이식 계획 8절 요청 1)가 요청한 cpppj 실제 Session 덤프로 추가했다.
#pragma once
#include "o/Graph.h"
#include "o/RiftType.h"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace netstorm::client {
class Client;
class RawCampaignSession;

// 덤프 형식 판번호다. 행 종류나 열 의미가 바뀌면 올린다. (2026-11-09 추가)
inline constexpr std::uint32_t kSessionStateDumpVersion=1;

// Kernel에 등록된 프로세스 한 개의 관측이다. SquidProcess가 아니면 form/parent는 0이다. (2026-11-09 추가)
struct SessionDumpProcess {
    std::uint32_t pid{}; // 원본 Kernel 슬롯 번호(1부터)다.
    std::uint32_t form{},parent{}; // 프로세스 Form SID와 부착 부모 SID다.
    bool regular{}; // RegularProcess이면 아래 정기 사건 칸이 유효하다.
    std::uint32_t event{},count{},payloadBits{}; // 정기 사건 번호·반복 수·float payload 비트다.
    std::uint64_t timeBits{}; // 다음 실행 시각 double의 비트다.
};

// 순수 서식 함수가 읽는 입력이다. 모든 span은 호출 동안만 살아 있으면 되며 사본을 보관하지 않는다. (2026-11-09 추가)
struct SessionStateDumpInput {
    o::OriginalEdition edition{o::OriginalEdition::Patch1078}; // 슬롯 필드 배치(소유자/프레임/HP 폭)를 고르는 판본이다.
    std::uint64_t frame{}; // 현재 프레임 번호다.
    double gameTime{}; // 현재 게임 시각(초)이며 비트 그대로 쓴다.
    std::uint32_t gameRandom{},crtRandom{}; // 게임 난수(00532710)와 CRT holdrand의 현재 상태다.
    bool world{}; // 실제 Session이 있을 때만 아래 월드 칸을 쓴다.
    std::span<const std::uint8_t> poolBytes; // SID 풀 전체 원문(capacity×stride)이다.
    std::uint32_t poolCapacity{},stride{},freeCount{},firstFreeServer{},firstFreeClient{},predictableCursor{}; // 풀 메타다.
    std::size_t kernelSize{}; // Kernel 등록 수다.
    std::uint32_t kernelCapacity{}; // Kernel 끝 번호다.
    std::vector<SessionDumpProcess> processes; // pid 오름차순 프로세스 목록이다.
    std::span<const std::uint8_t> players; // Player 9기록 원문(9×180)이다.
    std::span<const std::uint8_t> hiddenPlayers; // Chat 숨김 BYTE 9개다.
    std::span<const std::uint32_t> alliances; // 방향별 동맹 9×9 DWORD다.
    bool spInitialized{}; // 분산 SP 저장소 초기화 여부다. 미초기화면 값 칸을 "-"로 쓴다.
    std::uint32_t spOffset{}; // 분산 SP 비트 위치 offset이다.
    std::span<const std::uint32_t> spPools; // 32개 배열×9 DWORD 분산 원문이다.
    std::span<const std::int16_t> spPermutation; // 비트별 배열 순열 32칸이다.
    std::span<const o::GraphRecord> graph; // Graph 표 251칸이다.
    std::int32_t totalCost{}; // 장부 총 비용(0059508c)이다.
    std::uint32_t postPopDepth{},productionDirty{}; // 장부 깊이와 생산 재계산 알림이다.
    std::span<const std::uint32_t> totalLocal,totalBuilt,totalGlobal; // TotalMade 세 표(각 256칸)다.
};

// 분산 SP 원문에서 한 소유자의 32비트 값을 읽는다. ScrambledSpStore::Get과 같은 계산이지만 초기화/난수를 실행하지 않는 순수 함수다. (2026-11-09 추가)
std::uint32_t DecodeScrambledSp(std::span<const std::uint32_t> pools,std::span<const std::int16_t> permutation,std::uint32_t offset,std::uint32_t owner);
// 풀 전체 비교용 FNV-1a 64비트 해시다. 빈 입력은 0xcbf29ce484222325다. (2026-11-09 추가)
std::uint64_t SessionDumpHash(std::span<const std::uint8_t> bytes);
// 입력을 판번호 1 TSV 문자열로 만든다. 행 종류와 열은 docs/exe/cpp-session-state-dump-20261109.md에 정리했다. (2026-11-09 추가)
std::string FormatSessionStateDump(const SessionStateDumpInput& input);
// 실제 Client와 현재 Session(없으면 nullptr)에서 값을 읽어 덤프 문자열을 만든다. 상태를 바꾸는 호출(SP 초기화·난수 소비 등)은 하지 않는다. (2026-11-09 추가)
std::string DumpSessionState(Client& client,RawCampaignSession* session);
}
