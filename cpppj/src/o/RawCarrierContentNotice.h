// PATCH00429560: 사제/Walker/Carrier의 내용물 가상+24 전체 분기를 복원한다.
#pragma once
#include "o/RawContentParentNotice.h"

namespace netstorm::o {
// 해당 원본 읽기 위치에서 같은 전역을 다시 읽는 참조다.
struct CarrierContentNoticeState {
    const bool& savedGame; // 005c85d4: 저장 게임이면 모든 처리를 생략한다.
    const std::uint32_t& loadingDepth; // 005c89b8: 로딩 중이면 모든 후속 효과를 생략한다.
    const std::uint32_t& bombBlocked; // 005453e8: 폭탄 획득의 번개/음향/숫자 효과만 억제한다.
    const std::uint32_t& localOwner; // 00540c70: 현재 부모 owner BYTE와 비교한다.
    const std::uint32_t& priestType; // 005412d0: 포획/해방 사제 kind다.
    const std::int32_t &cameraX,&cameraY; // 0059a91c/920의 현재 화면 원점이며 번개는 투영된 정수 좌표를 받는다.
    const std::uint32_t& freeSoundBlockerType; // 005412c8: 해방 소리를 억제하는 현재 rainWalker156 타입이다.
};
// 번개·음향·할당/생성·타이머·공간 조회는 실제 같은 월드의 소비자를 연결한다.
struct CarrierContentNoticeHooks {
    std::function<std::array<float,2>(Sid)> center; // 실제004adea0이며 두 호출을 합치지 않는다.
    std::function<void(Sid,std::array<std::int32_t,2>,std::array<std::int32_t,2>,float)> bolt; //00472770의 부모/시작·종료/5.0이다.
    std::function<void(float,float,std::string_view)> soundAt; //004a9d70의 좌표/thunderCrack.wav다.
    std::function<std::uint32_t(std::uint32_t,std::size_t)> typeWord; // 현재 TYPE+6c/+7c의 실제 추가DWORD다.
    std::function<std::uint32_t(std::uint32_t)> allocate; // 원본 new40의 실제 호스트 토큰이며0이면 생성자를 호출하지 않는다.
    std::function<std::uint32_t(std::uint32_t,Sid,std::uint32_t,std::uint32_t,std::uint32_t)> create; //4ab240(this,parent,value,0,12)다.
    std::function<void(std::uint32_t,float)> duration; // 생성 결과의+20 타이머에1.5를 설정한다.
    std::function<void(std::uint32_t)> sound; //4a9550의 현재 부모 TYPE+7c다.
    std::function<void(Sid)> checkCarrier; //00426080의 실제 현재 지면/내용물 처리다.
    std::function<Sid(float,float,std::uint32_t)> genusAt,typeAt; //4b2070/4b1fa0의 실제 첫 객체 조회다.
    std::function<void(std::string_view)> freeSound; //4a9cb0의 priestFree.wav/0/0/0/1/0이다.
};
// 내용물 kind/상태와 부모/전역은 외부 효과 사이에서 원본 순서대로 다시 읽는다.
class RawCarrierContentNotice {
public:
    // 실제 참조 대상은 이 객체보다 오래 살아야 한다. CD 대응은 별도 복원한다.
    RawCarrierContentNotice(const SidPool& pool,std::span<const RiftTypeRecord> types,
        CarrierContentNoticeState state,CarrierContentNoticeHooks hooks);
    bool Handles(Sid parent) const; // 실제 현재 VT+24가429560인 부모인지 조회한다.
    void Notice(Sid parent,Sid form,std::uint32_t reason) const; // 원본 reason의 하위BYTE 비트를 유지한다.
private:
    std::uint32_t Word(Sid sid,std::size_t offset,std::size_t width=2) const; // 현재 little endian 슬롯 필드를 읽는다.
    const RiftTypeRecord& ContentType(Sid form) const; // 현재 Form의 kind WORD를 타입 표에서 조회한다.
    std::array<std::int32_t,2> Project(std::array<float,2> point) const; //497220/41cc50의16/11배·0.5·ftol·현재 원점이다.
    std::array<float,2> Position(Sid parent) const; // 현재 raw float 좌표를 비트로 읽는다.
    const SidPool& pool_; // 실제 PATCH50바이트 풀이다.
    std::span<const RiftTypeRecord> types_; // 현재 kind의 flags2를 직접 읽는다.
    CarrierContentNoticeState state_; // 원본 전역의 장기 참조다.
    CarrierContentNoticeHooks hooks_; // 외부 몸체의 실제 연결이다.
    RawContentParentNotice dispatch_; // 동일 부모 VT의 실제 가상 진입점을 조회한다.
};
}
