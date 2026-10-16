//00456460의 내부 표식과00455b20(mode1)의 화면 포스 필드 생성을 구별한다.
#pragma once
#include "o/RawGunTarget.h"

namespace netstorm::o {
//같은 실제 월드의 생성/통지/소유자/Pop 소비자다. 선택 시 누락을 거부한다.
struct FenceCreateHooks {
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //004af530(type,2)의 실제 새 자산 생성이다.
    std::function<void(Sid)> notifySurface; //표식 QA를 쓴 뒤004214a0의 현재 표면 통지를 수행한다.
    std::function<void(Sid,std::uint32_t)> setOwner; //생성 자산의 실제 가상+74 소유자 지정이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; //생성 자산의 실제 가상+90(flags40) Pop다.
};
//내부 fencemark90의 단계2 해시 구간과 끝점 태그를 생성한다.
class RawFenceMarkerCreate {
public:
    RawFenceMarkerCreate(SidPool& pool,const RawGunTarget& target,const std::uint32_t& type,FenceCreateHooks hooks); //같은 PATCH 풀/현재 중심/타입을 빌린다.
    static std::vector<std::array<float,2>> Points(std::array<float,4> centers); //원본 방향·inclusive Rect·4칸 구간 변경 뒤의 현재float 지점이다.
    void Create(Sid parent,Sid other) const; //QA/통지/QB/graph/현재owner/Pop 순서를 보존한다.
private:
    SidPool& pool_; //현재 두 끝점과 생성 표식의 실제 슬롯이다.
    const RawGunTarget& target_; //004adea0의 같은 실제 중심 조회다.
    const std::uint32_t& type_; //DAT005411c0의 현재 fencemark 번호다.
    FenceCreateHooks hooks_; //실제 생성/표시 하위의 현재 소비자다.
};
//화면의 포스 필드170을 원본100개 상한과 J/K 구간 첫 프레임으로 생성한다.
class RawFenceShieldCreate {
public:
    RawFenceShieldCreate(SidPool& pool,std::span<const RiftTypeFrames> frames,const std::uint32_t& type,FenceCreateHooks hooks); //같은 PATCH188TYPE/현재 타입을 빌린다.
    static std::vector<std::array<float,2>> Points(std::array<float,4> endpoints); //mode0과 같은 보정이나 정수 절삭 전float 지점이다.
    void Create(std::array<float,4> endpoints,std::uint32_t firstOwner,std::uint32_t secondOwner,std::int32_t firstId,std::int32_t secondId) const; //bit20 즉시 종료와 signed SID 소유자 선택을 유지한다.
private:
    SidPool& pool_; //생성 포스 필드의 현재 extra/TYPE/프레임이다.
    std::span<const RiftTypeFrames> frames_; //현재 TYPE의 J/K 첫 프레임 표다.
    const std::uint32_t& type_; //DAT00541300의 현재 fenceShield 번호다.
    FenceCreateHooks hooks_; //실제 생성/소유자/Pop이며 표면 통지는 이 경로에서 호출하지 않는다.
};
}
