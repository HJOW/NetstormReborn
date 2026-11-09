// 원본41ff20/41ff90의 현재 자격·소유권/받침/이웃 전파를 같은 물리 자료에 적용한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/RiftType.h"
#include <array>

namespace netstorm::o { //실제 가상 소유권/표시와 공간·로그 장치는 기존 Runtime을 빌린다.
enum class BombTreasonRole { NoIsland,EmptyGeyser,Fence,Bridge,Connector,Flare }; //비교/마지막 생성 위치에서 읽는 여섯 현재 DWORD다.
struct BombTreasonHooks { //선택된 효과 누락은 앞선 부수 효과 뒤 명시 실패로 알린다.
    std::function<std::uint32_t(BombTreasonRole)> role; //5412cc/5411a8/541278/5411a0/5412c4/541200을 현재 값으로 읽는다.
    std::function<void(Sid,std::uint32_t,std::uint8_t)> owner; //현재 가상74에 현재 시전자 owner BYTE를 전달한다.
    std::function<void(Sid,std::uint32_t)> refresh; //소유권 효과 뒤 다시 읽은 가상88의 실제 표시 갱신이다.
    std::function<void(Sid,bool)> fence; //false는457e20 정리, true는457890 재생성이다.
    std::function<Sid(float,float,std::uint32_t)> find; //4b1fa0의 같은 현재 좌표/TYPE 검색이다.
    std::function<void(float,float,std::uint8_t)> support; //44bd20의 실제 받침/섬/종유석 소유권 전파다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //원문 진단이 정상 복귀하면 같은 위치에서 계속한다.
    std::function<void(std::int32_t,std::string_view)> trace; //원문 수준4와 실제 TYPE+4 이름/좌표/owner 문구를 전달한다.
    std::function<void(Sid,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> flare; //마지막45a480(target,1,3,현재TYPE,3)이다.
};
class RawBombTreason { //동맹/HP/free/dead의 추가 관문 없이 원문 자격과 효과를 분리한다.
public:
    RawBombTreason(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,BombTreasonHooks hooks); //같은PATCH 풀/해시·현재188TYPE/프레임과 장치를 빌린다.
    bool MayTarget(Sid parent,Sid target) const; //owner 다름→지면/고갈/일반 genus→보행 제외 순서다.
    void Apply(Sid parent,Sid target) const; //QA 제외 뒤 현재 대상/받침/칸 위 대상/다리 연결 객체를 원문 순서로 갱신한다.
    const SidPool& Pool() const; //현재 Runtime의 같은 실제 풀을 확인한다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; //비정렬 BYTE/WORD/DWORD의 원본 폭만 읽는다.
    const RiftTypeRecord& Type(Sid sid) const; //각 효과 뒤 같은 슬롯의 현재 TYPE를 읽는다.
    bool Is(Sid sid,BombTreasonRole role) const; //현재 BYTE 종류와 현재 전체 DWORD 역할을 비교한다.
    int Side(Sid sid) const; //현재 TYPE/프레임에서 원문 signed 방향 BYTE를 읽는다.
    std::array<float,2> Position(Sid sid) const; //원문 Pop 좌표를 스냅 없이 두 float로 읽는다.
    void Set(Sid parent,Sid target) const; //현재 owner/가상74 호출 뒤 현재 가상88을 다시 읽는다.
    void At(Sid parent,float x,float y,bool connectors) const; //실제4202f0(flags0)의 선저장 next·현재 후보 순회다.
    const SidPool& pool_; //시전자/대상과 전파 대상이 공유하는 같은50BYTE 풀이다.
    const SquidHash& hash_; //같은 현재 네 단계 공간 해시를 빌린다.
    std::span<const RiftTypeRecord> types_; //현재 genus/TYPE+4 이름을 읽는188행 표다.
    std::span<const RiftTypeFrames> frames_; //현재 방향 판정의 실제 프레임 코드를 빌린다.
    BombTreasonHooks hooks_; //외부 효과가 바꾸는 현재 전역과 실제 Runtime 장치다.
};
}
