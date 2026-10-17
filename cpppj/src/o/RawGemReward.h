// 오벨리스크의 현재 QA 보상 또는 실제 가중 Deck 추첨을 생성·배치·삭제에 연결한다.
#pragma once
#include "o/RawDeckDraw.h"
namespace netstorm::o {
// 효과 뒤 다시 읽는 원본 타입 개수와 오벨리스크 타입이다.
struct GemRewardState {
    std::int32_t typeCount{188}; //541348의 현재signed TYPE 개수다.
    std::uint32_t buriedType{112}; //541218의 현재 오벨리스크 타입이다.
};
// 게임 계산은 본문이 소유하고 현재 시각·진단·실제 큰 월드 호출만 공급한다.
struct GemRewardHooks {
    RawProductionDeck::Fill fill; // 초기화 전 스택 Deck의 물리 미정 바이트 입력이다.
    DeckDrawHooks deck; // 실제 벽시각/Deck 진단·로그다.
    std::function<void(std::uint32_t)> assertion; //Gem.cpp의f6/104/10a 진단이다.
    std::function<void(ProductionDeckAssertion)> deckAssertion; //실제 항목 추가의 원본 진단이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //4af530의 실제 자산 생성이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; //생성 결과의 현재 가상+90이다.
    std::function<void(Sid,std::uint32_t)> destroy; //원오벨리스크의 현재 가상+10이다.
};
// 동일 풀/TYPE/전역 난수/제외 목록으로461f00과4620c0 전체를 수행한다.
class RawGemReward {
public:
    RawGemReward(SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,const SquidPostPopList& excluded,GemRewardState& state,GemRewardHooks hooks); //모든 장기 참조는 이 객체보다 오래 살아야 한다.
    std::uint32_t Choose(std::uint32_t unused=5) const; //원본은 전달된5를 읽지 않고 현재TYPE 표 전체로 Deck을 만든다.
    Sid Reveal(Sid buried) const; //현재QA 검증→선택→생성→현재좌표Pop→원객체삭제다.
    const SidPool& Pool() const; //같은실제월드 조립 검사용이다.
private:
    const RiftTypeRecord& Type(std::uint32_t type) const; //현재TYPE 범위를 넘는 손상 입력은 읽기 전에 거부한다.
    bool Excluded(std::uint32_t type) const; //실제 제외목록의BYTE 항목과 전체TYPE 인자를 비교한다.
    SidPool& pool_; //실제 오벨리스크/생성객체의 유일한 슬롯 저장소다.
    std::span<const RiftTypeRecord> types_; //효과 뒤 현재레벨/genus를 다시 읽는다.
    GameRandom& random_; //원본00532710의 동일 게임 난수다.
    const SquidPostPopList& excluded_; //건설 소비자와 같은 현재TYPE 제외 목록이다.
    GemRewardState& state_; //다음 반복 및 공개변환의 현재전역이다.
    GemRewardHooks hooks_; //선택된 실제 하위가 없으면 명시 거부한다.
};
}
