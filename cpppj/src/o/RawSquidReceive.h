// 공통 객체35BYTE 수신의 실제 슬롯 쓰기·첫 배치·기술 경고와 검증 전역을 공유한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::o { //00433c30의 같은 장기 전역/현재 가상 자식을 빌린다.
struct SquidReceiveState {
    std::uint32_t& verifySid; //0054d4d8이 비영이면 쓰기 대신 다섯 항목만 검증한다.
    std::uint32_t& typeWas; //0054d4e4는 Take 전 TYPE를 받고 정상 Pop 뒤0이 된다.
    std::span<std::uint32_t> noMakeWarnings; //0054d4e8의 실제 타입별255DWORD다.
};
struct SquidReceiveHooks {
    std::function<Sid(std::uint32_t,Sid)> take; // 같은 Factory.Take의 해제/생성자/가상 후처리다.
    std::function<void(Sid,std::uint16_t)> word; //004ac220의 WORD 저장/현재 다리 통지다.
    std::function<void(Sid,std::uint32_t)> owner; // 현재 가상+74의 파생 소유자 몸체다.
    std::function<std::uint32_t(std::uint32_t)> flags1; //0049a8e0이 읽는 수신 원래 TYPE의 flags1이다.
    std::function<std::uint32_t(Sid)> firstPop; // 현재 가상+30의 최초 배치 flags다.
    std::function<bool(std::uint32_t,std::uint32_t)> hasTechnology; //0048f110의 실제 Core 장부 bit 조회다.
    std::function<void(Sid)> noMake; // 현재 이름/TYPE 설명의 실제 Tell이다.
    std::function<void(Sid)> markNoMake; // 첫 안내 여부와 무관한 현재 Player400 표식이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 현재 가상+90의 같은 공간 부모다.
    std::function<void(Sid,Sid,std::uint32_t)> containedPop; // 진단 복귀 뒤 실제 가상+50을 실행한다.
    std::function<std::string(Sid)> typeName; // 검증 로그의 TYPE+4 이름이다. 표시 설명과 다르다.
    std::function<void(std::int32_t,std::string_view)> log; // 원문 수준1d의 실제 로그다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // Clientboard.cpp 진단의 정상 복귀도 유지한다.
};
void ReceiveRawSquid(SidPool& pool,std::span<const std::uint8_t> packet,SquidReceiveState state,const SquidReceiveHooks& hooks); //00433c30의 PATCH35BYTE 본문이다. CD487d40의 별도 폭은 이 진입에 혼합하지 않는다.
}
