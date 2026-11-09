// 원본41ede0/41ef30/41f110의 맨 오 워 직접 소환 자격과 현재 자료 조회를 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o { //공통 시전/검색/수신은 기존 본문에 맡기고 실제 자격의 소환 부수 효과만 처리한다.
struct ManoSpellHooks { //선택된 실제 Runtime의 현재 전역·생성/소유자/헤더·공간 장치를 빌린다.
    std::function<double()> wallSeconds; //004bcab0이 만드는 사용하지 않는 지역 seed의 실제 현재 시각이다.
    std::function<std::uint32_t(std::uint32_t)> spellType; //00541324/328/32c를 선택 함수에 맞게 한 번 읽는다.
    std::function<std::uint32_t()> flyerType; //각 생성 직전00541298의 현재 전체 DWORD다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //4af530(현재 flyer TYPE,0)의 실제 공장이다.
    std::function<void(Sid,std::uint32_t,std::uint8_t)> owner; //프레임0 직접 쓰기 뒤 현재 가상74와 현재 운반체 owner를 사용한다.
    std::function<std::array<std::int16_t,2>(Sid,std::uint32_t,bool)> header; //현재 추상 비트/프레임0의 signed 높이·hotspotY와 선택된 원문 진단이다.
    std::function<std::array<float,2>(Sid)> center; //헤더 뒤 같은 실제4adea0으로 운반체의 현재 중심을 읽는다.
    std::function<void(Sid,std::uint32_t,float,float,std::uint32_t)> pop; //현재 새 객체의 가상90에 원주 좌표와 flags0을 전달한다.
};
class RawManoSpell { //세 자격은 소환을 수행해도 항상false를 반환하며 별도 c0는 원문 ret4다.
public:
    RawManoSpell(SidPool& pool,const bool& server,std::span<const std::int32_t> counts,ManoSpellHooks hooks); //같은 PATCH 물리 풀/현재 서버·188TYPE spawns와 장치를 빌린다.
    bool MayTarget(Sid parent,Sid target) const; //서버/QA 관문→지역 seed→현재 고정 TYPE의 반복 소환→false 순서다.
    const SidPool& Pool() const; //같은 Runtime의 물리 자료 혼용 여부를 확인한다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; //지정한 비정렬 BYTE/WORD/DWORD만 읽는다.
    std::int32_t Count(std::uint32_t type) const; //처음 선택한 TYPE 행의 현재 signed spawns를 매 사용 위치에서 읽는다.
    SidPool& pool_; //시전자/운반체/실제 새 객체가 공유하는 같은50BYTE 저장소다.
    const bool& server_; //원문540bc0이며 boss와 구별하고 최초 관문에서만 읽는다.
    std::span<const std::int32_t> counts_; //Native는 같은 장기 TYPE+1c8 배열을 직접 빌린다.
    ManoSpellHooks hooks_; //선택한 실제 하위가 없으면 앞선 부수 효과 뒤 명시 실패한다.
};
}
