// 사제 기도 입력4940b0/수신493f80/605 진행494580을 같은 현재 풀과 Host에 연결한다.
#pragma once
#include "o/RawFormPacket.h"
#include "o/SidPool.h"
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o {
// 원본 TYPE+1bc와118의 현재 정수 속성이다.
struct PriestPrayerType {
    std::int32_t prayTime{},defaultFrame{}; // 기도 완료 횟수와 완료 뒤 현재 사제의 기본 프레임이다.
};
// 해당 분기에서 선택된 큰 하위만 요구하며 원본의 재조회 순서를 유지한다.
struct PriestPrayerHooks {
    std::function<bool(Sid,std::uint32_t)> hasEvent; // 현재 첫605 Regular의 존재를 조회한다.
    std::function<std::int32_t(Sid)> direction; // 4ad710의 현재 프레임 방향0..15 또는-1이다.
    std::function<std::uint32_t(std::uint32_t)> movementType; // 488fe0의 현재 TYPE에 따른 이동 프로세스 종류다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,bool)> movement; // 4b1a90의 첫 Form 조회 또는 실제 가상 삭제다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> request; // 4e2dc0(9,packet)의 실제 요청 전송이다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; // boss가 이동 Form을 제거한 뒤 실행하는4e2e30이다.
    std::function<bool()> reserveShared; // 원본 new0x28 한 번의 성공/실패다.
    std::function<void(Sid,std::uint32_t,float)> addShared; // 확보한 같은 객체의605/payload1 SharedRegular 생성이다.
    std::function<void(Sid,std::uint32_t)> ritual; // 491cf0(parent,4)의 현재 프레임/604 공유 예약이다.
    std::function<bool(Sid)> immobile; // 427030의 현재 마비/사제 HP·공간 검사다.
    std::function<std::uint32_t(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> containedType; // 4acac0의 첫mask1/168 종류 조회다.
    std::function<void(Sid)> discard; // 4264d0의 현재 첫mask1 내용물 삭제와 필요한 너겟 보충이다.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> create; // 4afa70의 현재 희망 종류/0/1/0/0 생성·부착이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 현재 사제 TYPE의 기본 프레임과flags0이다.
    std::function<void(Sid)> panic; // 다른 명령03000103의491db0이며 미복원일 때 성공으로 대체하지 않는다.
    std::function<std::uint32_t(Sid,std::uint32_t,std::uint32_t)> parent; // 42aa50의 같은 코드/원래 사건y 위임이다.
};
// 10.78의 기도 세 경로를 복원하며 나머지 사제 Regular를 대신 처리하지 않는다.
class RawPriestPrayer {
public:
    RawPriestPrayer(SidPool& pool,std::span<const PriestPrayerType> types,const bool& boss,PriestPrayerHooks hooks); // 같은188TYPE/권한/50BYTE 풀을 빌린다.
    std::uint32_t Command(Sid parent,std::uint32_t code,std::uint32_t parameter) const; // packed y의 상위WORD 종류를6d 요청으로 보낸다.
    void Receive(std::span<const std::uint8_t> packet) const; // 희망 종류 쓰기→상태/605/방향→boss 이동 제거/배포→605/604다.
    float Tick(Sid parent,std::uint32_t count,float payload) const; // 현재count 쓰기→이동/정지→정확한 praytime 일치→내용물/기본 프레임이다.
private:
    std::uint32_t Read(Sid parent,std::size_t offset,std::size_t width=4) const; // 현재 물리 슬롯의 little endian 필드다.
    void Write(Sid parent,std::size_t offset,std::uint32_t value) const; // 예약/free 슬롯에도 원본처럼 현재 DWORD를 쓴다.
    const PriestPrayerType& Type(std::uint32_t kind) const; // 실제 타입 표 밖 손상 입력은 명시 오류다.
    SidPool& pool_; // 같은 실제 사제/내용물/Form을 보관하는 풀이다.
    std::span<const PriestPrayerType> types_; // 현재 TYPE의 praytime/defaultFrame 표다.
    const bool& boss_; // 각 원본 분기 위치에서 읽는 현재 권한이다.
    PriestPrayerHooks hooks_; // 실제 전송/이동/공유예약/의식/내용물 소비자다.
};
}
