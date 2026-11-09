// 원본41e6a0의 주문 초기화·시전 시간·대상 패킷 배포를 처리한다.
#pragma once
#include "o/RawFormPacket.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 기존 TYPE 레코드에 없는 이 흐름의 두 DWORD만 같은 번호로 빌린다.
struct BombRegularType {
    std::int32_t firstFrame{}; // TYPE+130의 첫 A 프레임이다.
    std::int32_t castTime{}; // TYPE+1b8의 signed 시전 시간이다.
};
// 외부 효과 뒤 현재 값을 재조회하는 원본 세션 전역이다.
struct BombRegularState {
    std::uint32_t boss{},localOwner{1}; // 540bc4 권한과540c70의 전체 소유자 DWORD다.
    FormPacketHeader effect{0x5f,1,10}; // 52ee3a/3b·544c6c의 현재 가변 패킷 헤더다.
    const bool* sharedBoss{}; // Native는 같은 실제 Client 권한을 프레임 효과 뒤 다시 읽는다.
    std::function<std::uint32_t()> currentLocalOwner; // 방송 효과 뒤 현재 전체 Player DWORD를 읽는 실제 장치다.
    bool Boss() const { return sharedBoss ? *sharedBoss:boss!=0; } // 독립 typed 입력의 기존 DWORD 비영 의미를 보존한다.
    std::uint32_t LocalOwner() const { const auto read=currentLocalOwner;return read ? read():localOwner; } // 선택한 함수 사본을 호출 종료까지 유지한다.
};
// 큰 월드/경제/검색/실제 효과는 같은 세션의 몸체로 연결한다.
struct BombRegularHooks {
    std::function<void(Sid,std::uint32_t,std::uint32_t)> notice; // 45a6c0(QA,6,casttime*2)이다. 내용물 삭제를 대신하지 않는다.
    std::function<void(Sid,std::uint32_t,std::uint32_t)> frame,animate; // 4acee0(first,0)와4afc90(1,0)의 실제 처리다.
    std::function<bool(std::uint32_t)> allocate; // new400의 실제 확보 성공을 보고한다.
    std::function<std::uint32_t(Sid,std::span<std::uint32_t>)> targets; // 41e4b0의 현재 대상 검색이며 최대100개다.
    std::function<void(std::span<std::uint8_t>)> broadcast; // 4e2e30의210바이트 버퍼다. 실제 송신의 길이 마무리/효과를 허용한다.
    std::function<void(std::uint32_t,std::uint32_t,std::span<const std::uint8_t>)> receive; // 배포 뒤 현재owner로420bf0(owner,0,packet)를 호출한다.
    std::function<void(std::uint32_t)> release; // 확보한 대상 배열400바이트를 해제한다.
    std::function<std::span<std::uint32_t>()> targetBuffer; // Native는 방금 확보한 실제 DWORD100개를 반환하며 기존 typed만 지역 배열을 유지한다.
};
// 피해와 내용물 소비는 별도 수신 몸체이며 이 처리기는 Regular 순서만 복원한다.
class RawBombRegular {
public:
    RawBombRegular(const SidPool& pool,std::span<const BombRegularType> types,BombRegularState& state,BombRegularHooks hooks); // 같은10.78 풀/188TYPE/현재 전역을 빌린다.
    float Handle(Sid parent,std::uint32_t event,std::uint32_t count,float payload) const; // count0 초기화 뒤 권한·casttime을 비교하고 필요 시 효과 패킷을 작성한다.
    RawBombRegular(const SidPool& pool,BombRegularState& state,BombRegularHooks hooks,std::function<BombRegularType(std::uint32_t)> read); // Native는 실제 현재 TYPE reader만 받으며 미사용 TYPE 사본을 만들지 않는다.
    const SidPool& Pool() const; // 다른 세션의 객체/연결을 혼합하지 않도록 확인한다.
private:
    BombRegularType Type(Sid parent) const; // 매 호출에서 현재 raw TYPE를 다시 읽는다.
    const SidPool& pool_; // 현재 주문 객체의 물리50바이트 저장소다.
    std::span<const BombRegularType> types_; // 현재TYPE별 첫프레임/casttime이다.
    BombRegularState& state_; // 외부 효과가 변경할 수 있는 권한/소유자/헤더다.
    std::function<BombRegularType(std::uint32_t)> currentType_; // 같은 실제 장기 TYPE 자료의 조회 장치이며 독립 span은 기존대로 보존한다.
    BombRegularHooks hooks_; // 도달한 큰 하위만 요구하는 실제 연결점이다.
};
}
