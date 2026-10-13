// 다이스에 포획 내용물이 부착될 때 실제 알터 O프레임/원래owner WORD와 cue를 갱신한다.
#pragma once
#include "o/RawDaisRegular.h"

namespace netstorm::o {
// 최초 내용물 알림이 매 분기 읽는 원본 전역이다. 희생 완료의 voiceCue와 다른 cue다.
struct DaisContentNoticeState {
    std::uint32_t& priestType; // patch005412d0/CD0051cbbc의 현재 사제 kind다.
    std::uint32_t& localOwner; // 현재 다이스 owner와 비교하는 patch00540c70/CD0050f6c8이다.
    std::uint32_t cue{}; // patch00542a50/CD00512bd8의 포획 사제 알림 cue다.
    std::uint32_t bridgeType{82}; // 공통 WORD 지정이 표면 통지를 추가하는 현재 다리 타입이다.
};
// 현재 알터의 가상 공간과 cue/다리 WORD 후속 효과를 실제 같은 월드에 연결한다.
struct DaisContentNoticeHooks {
    std::function<void(Sid,std::uint32_t)> unpop,repop; // 실제 알터 가상 +48/+4c flags0이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> stopVoice; // 원본 cue/0/0의 실제 전역 알림 중지다.
    std::function<void(Sid)> notifySurface; // 공통 WORD setter의 현재 다리 타입에서만 실행하는 표면 후속이다.
};
// ContentFormHost의 reason1/2를 실제 Dais vtable+24 몸체로 전달한다.
class RawDaisContentNotice {
public:
    // 공유 전역/프레임/typedAltar와 모든 효과는 이 객체보다 오래 살아야 한다.
    RawDaisContentNotice(SidPool& pool,std::span<const RiftTypeFrames> frames,const RawRunePostPop& rune,
        const DaisContentNoticeState& state,DaisContentNoticeHooks hooks);
    bool Handles(Sid dais) const; // 실제 다이스 생성자 VT를 검사하며 원본 body에 없는 타입 필터를 넣지 않는다.
    void Notice(Sid dais,Sid form,std::uint32_t reason) const; // reason1/현재사제kind이면 알터 재등록과 로컬cue 중지를 수행한다.
    const SidPool& Pool() const; // 실제 ContentFormHost/가상 공간의 동일 풀을 확인한다.
private:
    void WriteFrame(Sid altar,std::int32_t frame) const; // setter/표시 효과를 추가하지 않는 DWORD/BYTE 직접쓰기다.
    void WriteWord(Sid altar,std::uint16_t word) const; // 원본 공통 WORD 쓰기 뒤 현재 다리 타입만 통지한다.
    SidPool& pool_; // 단일 현재 raw 저장소다.
    std::span<const RiftTypeFrames> frames_; // 가상 Unpop 뒤 현재 알터 타입의 O첫프레임을 읽는다.
    const RawRunePostPop& rune_; // 현재 같은 typedAltar finder를 재사용한다.
    const DaisContentNoticeState& state_; // 현재 사제/로컬/cue/다리 타입 전역이다.
    DaisContentNoticeHooks hooks_; // 실제 알터 가상 공간·cue·표면 효과다.
};
}
