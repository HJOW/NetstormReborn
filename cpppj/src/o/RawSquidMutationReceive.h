// 내용물2dh와 삭제2eh가 같은 풀의 실제 생성/부착/환불·삭제를 사용한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::o { // 원문 receiver가 선택한 현재 자식만 요구한다.
struct SquidMutationHooks {
    std::function<std::uint32_t()> contentType,dependType; // 각각 현재00541088/00541074다.
    std::function<bool()> boss; // 삭제 입구의 현재00540bc4다.
    std::function<Sid(std::uint32_t,Sid)> take; // 같은 Factory.Take다.
    std::function<void(Sid,std::uint16_t)> word; // 같은004ac220 WORD setter다.
    std::function<std::uint32_t(Sid)> firstPop; // 현재 가상+30이다.
    std::function<void(Sid,Sid,std::uint32_t)> attach; // 현재 가상+50의 실제 파생 부착이다.
    std::function<void(Sid,std::uint32_t)> refund; //0044c2f0(sid,owner,1)의 실제 HP 비례 환불이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 현재 가상+10의 실제 삭제다.
    std::function<std::string(Sid)> typeName; // 로그의 현재 TYPE+4 이름이다.
    std::function<void(std::int32_t,std::string_view)> log; // 실제 수준1d 로그다.
    std::function<void()> auditSp; // 삭제 후 현재 분산 SP/추적 offset/검사 횟수를 재조회한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 boss 진단의 정상 복귀를 유지한다.
};
void ReceiveContentSquid(SidPool& pool,std::span<const std::uint8_t> packet,const SquidMutationHooks& hooks); //004327b0의 PATCH18BYTE 내용물 수신이다.
void ReceiveDeletedSquid(const SidPool& pool,std::span<const std::uint8_t> packet,const SquidMutationHooks& hooks); //00433170/CD00488270 비교의 PATCH8BYTE 삭제 수신이다.
}
