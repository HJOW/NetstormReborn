// 원본10.78의4d5840 후보 순위와 이동 유닛 근접 보정을 같은 현재 풀에서 실행한다.
#pragma once
#include "o/SidPool.h"
#include <array>
#include <functional>

namespace netstorm::client { // 현재 입력의 장치 경계와 원본 후보 선택 몸체다.
struct UserPickHooks { // 큰 원본 하위는 별도로 검증한 현재 월드/장치를 빌린다.
    std::function<std::uint32_t()> paused,localOwner,metamap,hideBuildings; //460de0/540c70/594fc8/59a8ac의 현재값이다.
    std::function<std::array<float,2>(std::int32_t,std::int32_t)> project; //4971e0의 현재 화면→월드 변환이다.
    std::function<std::uint32_t(std::array<float,2>,int,int)> begin; //422230의 실수 점/두 반경이며 flags는0이다.
    std::function<std::uint32_t()> next; //4b1810의 같은 현재 finder다. 물리 범위 밖이면 원문처럼 끝난다.
    std::function<std::array<float,2>(o::Sid)> center; //4adea0의 현재 발자국 중심이다.
    std::function<bool(o::Sid,std::int32_t,std::int32_t)> hit; //각 후보의 실제+b0 경계다. 순위가 밀린 후보에는 호출하지 않는다.
};
class RawUserPick { // 클릭/우클릭/이동 커서가 같은 후보 우선순위를 사용한다.
public:
    RawUserPick(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,UserPickHooks hooks); //풀/TYPE 수명은 이 처리기보다 길어야 한다.
    o::Sid Pick(std::int32_t x,std::int32_t y,std::uint32_t force=0) const; //force는 비선택/추상 필터만 우회하며 숨김/매몰 필터와 다르다.
private:
    const o::RiftTypeRecord& Type(o::Sid sid) const; //콜백 뒤 현재 슬롯의 TYPE를 조회한다.
    const o::SidPool& pool_; //모든 후보의 현재 물리 슬롯이다.
    std::span<const o::RiftTypeRecord> types_; //현재188TYPE의 플래그다.
    UserPickHooks hooks_; //현재 시점의 조회와 원본 가상 hit 경계다.
};
}
