// 원본 352바이트 Deck의 공유 초기화·추가와 독립 벽시각 난수 추첨을 복원한다.
#pragma once
#include "o/RawProductionDeck.h"
namespace netstorm::o {
// Draw가 선택한 장치·진단만 공급하며 순회·난수·현재 합계는 몸체가 계산한다.
struct DeckDrawHooks {
    std::function<double()> wallSeconds; // 460d70의 호출 시점 double이다.
    std::function<void(std::uint32_t)> assertion; // Deck.cpp의 e5 미개방 진단이다.
    std::function<void(std::uint32_t,std::uint32_t)> mismatch; // 현재 합계와 재계산 합계의 실제1d 로그다.
    std::function<bool(std::uint32_t)> isProduct; // optional power 출력 때만425830의 현재 네 TYPE를 검사한다.
};
// Player 또는 스택 소유자와 별개로 같은 물리 Deck을 빌린다.
class RawDeckDraw {
public:
    RawDeckDraw(std::span<std::uint8_t> raw,GameRandom& global,DeckDrawHooks hooks); // 실제352바이트와 같은 전역 난수를 빌린다.
    static void Initialize(std::span<std::uint8_t> raw); //44f200은 다섯 DWORD만 초기화한다.
    static void Append(std::span<std::uint8_t> raw,std::uint32_t type,std::int32_t chance,const std::function<void(ProductionDeckAssertion)>& assertion); //44f290의 공통 추가다.
    void Open(std::uint32_t seed) const; //44f250은0일 때만 같은 게임 난수를 다시 소비한다.
    void Close() const; //44f280은 open의bit0만 지운다.
    std::uint32_t Draw(std::int32_t* power=nullptr) const; //44f510의 현재signed power·합계·벽시각 임시난수·출력이다.
    static std::uint32_t WallSeed(double seconds); //4bcab0의1013배/ftol낮은DWORD/signed>>4+*33이다.
private:
    std::span<std::uint8_t> raw_; // 실제 항목66개와 다섯 DWORD의 단일 저장소다.
    GameRandom& global_; // Open의0seed 보충만 전역 난수를 쓴다.
    DeckDrawHooks hooks_; // 실제 시각·진단·로그·optional power 분류다.
};
}
