// 10.78 Money 저장은 현재SP 대신 같은 게임난수에서 만든 float32 네 바이트를 기록한다.
#pragma once
#include "o/SpStore.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
// 0044cce0의 실제 일반 stream writer다. 현재 필드/용량 진단은 이 하위 몸체가 수행한다.
using FortMoneySaveWrite=std::function<void(std::uint32_t,std::span<const std::uint8_t>)>;
// 단독 Template 부모의 현재manager/Create/실제 일반 쓰기 경계다.
struct FortMoneySaveHooks {
    std::function<std::uint32_t(std::int32_t)> manager; // 고정this 슬롯의 현재 첫DWORD를 한 번 읽는다. 0도 부모 자체는 진단하지 않는다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> create; // 현재 manager의 새Money stream을 만들고 실제토큰을 반환한다.
    FortMoneySaveWrite write; // 반환된같은stream에4BYTE를 쓰며 진단뒤현재필드재조회도생략하지않는다.
};
// 004832c0: 같은sharedrandom.Next(0x3f2)→signed정수→333.0f가산→float32→일반4BYTE쓰기다.
void SaveFortMoney(std::uint32_t stream,GameRandom& random,const FortMoneySaveWrite& write);
// 004beec0: 현재manager→Create("Money")→저장이다. isMine/모드/manager진단은 전체snapshot부모의별도책임이다.
void SnapshotFortMoney(std::int32_t slot,GameRandom& random,const FortMoneySaveHooks& hooks);
}
