// 원본10.78 객체 메뉴의 공통 가상+ac와 객체 중심 위치 계산을 복원한다.
#pragma once
#include "client/InputEvent.h"
#include "client/RawObjectMenu.h"

namespace netstorm::client {
// 메뉴 생성·제목·실제 파생+a8·좌표 변환은 같은 Client/Session의 몸체를 공급한다.
struct ObjectMenuOpenHooks {
    std::function<o::Sid(o::Sid)> altar; // 447930: 다이스와 같은 좌표의 현재 제단을 조회한다.
    std::function<std::uint32_t(o::Sid,const InputEvent&)> delegate; // 실제 제단의 가상+ac로 같은 사건 참조를 넘긴다.
    std::function<void()> beginFancy; // 실제477ae0의 controlled 장식 메뉴 생성이다.
    std::function<void(o::Sid)> summary,populate; // 실제4ada70의 제목/정보와 현재 객체의 가상+a8 작성이다.
    std::function<std::array<std::int32_t,2>(float,float)> project; // 497220의 현재 카메라를 사용한 화면 좌표다.
    std::function<o::Sid(o::Sid)> content; // 실제4ac9f0(this,1,0,0)의 첫 종속 Form이다.
    std::function<float(std::uint32_t)> range; // 내용물 kind의 현재 TYPE+44 float다.
    std::function<float()> menuWidth; // 475c40 반환 메뉴의 현재 기본 폭+3c이며 목표 폭이 아니다.
    std::function<std::int32_t()> screenWidth; // 5c78f0의 현재 화면 폭을 폭 조회 뒤 읽는다.
    std::function<void(o::Sid,std::int32_t,std::int32_t)> attach; // 475cf0: 같은 controlled 메뉴를 객체에 부착한다.
};
// 다른 판본의 슬롯/상수를 섞지 않는다. CD 대응4aebf0/4e35f0은 별도 원본 대조가 필요하다.
class RawObjectMenuOpen {
public:
    // 풀/TYPE와 콜백의 참조 대상은 이 몸체보다 오래 살아야 한다.
    RawObjectMenuOpen(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,ObjectMenuOpenHooks hooks);
    // 4adb30: extra bit1 거부→제단 위임→생성/제목/파생 작성→현재 TYPE별 위치/부착이다.
    // 사건 좌표는 메뉴 작성 뒤 같은 참조에서 읽어 callback의 변경을 보존한다.
    std::uint32_t Open(o::Sid object,const InputEvent& event) const;
    // 425f90: 현재 객체 투영→첫 Form kind 사정거리→현재 메뉴 폭/화면 폭의 signed DWORD 계산이다.
    void Position(o::Sid object) const;
private:
    std::span<const std::uint8_t> Raw(o::Sid object) const; // 생존/void 필터 없이 실제 물리 슬롯을 읽는다.
    const o::RiftTypeRecord& Type(std::uint32_t number) const; // 현재 TYPE 표의 범위를 확인한다.
    static std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width); // 비정렬 little endian 필드다.
    static std::uint32_t Truncate(float value); // 원본_ftol signed64의 하위DWORD와 불확정값 하위0이다.
    const o::SidPool& pool_; // 같은50바이트 객체 풀을 빌린다.
    std::span<const o::RiftTypeRecord> types_; // 콜백 뒤 변경한 현재 TYPE를 다시 읽는다.
    ObjectMenuOpenHooks hooks_; // 실행 경로에서 도달한 실제 외부 몸체만 요구한다.
};
}
