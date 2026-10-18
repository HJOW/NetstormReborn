// 원본 Factory의 지식 선택·서버 연구 수신·생산 내용물 통지를 복원한다.
#pragma once
#include "o/RawPacketDispatch.h"

namespace netstorm::o {
// 같은 월드의 현재 전역을 참조한다. 효과 호출 뒤에도 최신 값을 다시 읽는다.
struct FactoryResearchState {
    const std::uint32_t& loadingDepth; // 005c89b8: 읽는 동안 생산 갱신을 억제하는 깊이다.
    std::uint32_t& productionDirty; // 0055a4a4: 다른 생산 경로와 공유하는 감김 카운터다.
    const bool& boss; // 00540bc4: 서버 연구를 수행할 권한이다.
    const double& now; // 0055b4d0: 선택 때 패킷에 복사하는 현재 게임 시각이다.
    const std::uint32_t& outpostType; // 00541250: 서버 연구에서 제외하는 현재 아웃포스트 타입이다.
};
// 각 하위는 실제 검색/삭제/생성/송신/효과에 연결해야 한다. 빈 성공으로 대신하지 않는다.
struct FactoryResearchHooks {
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> find; // 4ac9f0의 부모·mask·kind·순번 검색이다.
    std::function<std::int32_t(Sid)> slots; // 4752b0(parent,0,0)의 현재 남은 슬롯이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 실제 내용물의 가상 Destroy(0)이다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> create; // 4afa70의 현재 실제 생성/부착이다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> send; // 원문16바이트를 채널9에 송신한다.
    std::function<void(std::string_view,std::uint32_t)> help; // researchHelp의 실제 도움말 창 경계다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> flare; // 45a6c0(parent,6,2)의 실제 플레어 생성이다.
    std::function<void(std::uint32_t)> refresh; // 44f7c0의 같은 owner 생산 Deck 재계산이다.
    std::function<void(std::string_view)> assertion; // 원본 진단 뒤의 현재 전역 재조회도 허용한다.
};
// 40aba0 생성자와4547d0의 필드 기록이다. 뒤의 호스트 버퍼를 건드리지 않는다.
std::size_t WriteFactoryResearchCommand(Sid parent,std::uint32_t kind,double now,std::span<std::uint8_t> packet);
// 10.78의 연구 경로만 다룬다. 업그레이드 진행·다른 Factory 명령은 별도 몸체가 필요하다.
class RawFactoryResearch {
public:
    // 풀·TYPE·현재 상태·콜백 대상은 이 객체보다 오래 살아야 한다.
    RawFactoryResearch(const SidPool& pool,std::span<const RiftTypeRecord> types,FactoryResearchState state,FactoryResearchHooks hooks);
    // 두 실제 Factory VT의 내용물 +24가453b50인지 확인한다.
    bool HandlesNotice(Sid parent) const;
    // 453cc0: 예측 내용물 삭제→슬롯 재조회→동일 종류 재검색→실제 생성이다.
    void Learn(Sid parent,std::uint32_t kind,std::uint32_t attachFlags) const;
    // 4547d0의03000080만 처리한다. packedArgument의 상위 signed WORD가 선택한 종류다.
    std::uint32_t Input(Sid parent,std::uint32_t code,std::uint32_t packedArgument) const;
    // 453d70의 연구/무동작 분기다. 미복원 업그레이드 요청은 변경 전에 명시 거부한다.
    void Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const;
    // 453b50→475220: 플레어·중복 예측 삭제·현재 dirty/Deck 갱신 순서다.
    void Notice(Sid parent,Sid form,std::uint32_t reason) const;
    const SidPool& Pool() const; // 단일 월드 조립을 확인하는 같은 풀이다.
private:
    // 필요한 BYTE/WORD/DWORD만 비정렬 little endian으로 읽는다.
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const;
    // 475220은 form0도 물리0번 슬롯의 flags를 읽는다.
    void ManaNotice(Sid parent,Sid form,std::uint32_t reason) const;
    const SidPool& pool_; // 모든 현재 부모·내용물의 동일 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 genus를 읽는 PATCH188개 타입이다.
    FactoryResearchState state_; // 값 복사 대신 같은 현재 전역의 참조다.
    FactoryResearchHooks hooks_; // 실제 계산 하위와 장치 연결이다.
};
// 실제58h의 빈 observer/프로토콜1/정책0/16바이트 길이를 같은 분배기에 등록한다.
void RegisterFactoryResearchCommand(RawPacketDispatch& dispatch,RawFactoryResearch& research,std::function<void()> prepare={});
}
