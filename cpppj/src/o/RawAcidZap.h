// 애시드145의11바이트 포획 명령 생성과48bfe0 로컬/원격 수신을 복원한다.
#pragma once
#include "o/RawFormPacket.h"
#include "o/RiftType.h"
#include <array>
#include <functional>
#include <string_view>

namespace netstorm::o {
// 수신 시점마다 읽는 원본 두 TYPE 전역이다.
struct AcidZapState {
    std::uint32_t altar{164}; //5412e8의 포획 제외 제단이다.
    std::uint32_t growingIceTower{150}; //5412b0의 포획 제외 성장 중 아이스 타워다.
};
// 실제 소리/캡처18/가상 삭제는 같은 월드에 연결한다.
struct AcidZapHooks {
    std::function<void(float,float,std::string_view,std::uint32_t)> sound; //4a9d70의 현재 좌표/이름/반복0이다.
    std::function<void(Sid,Sid,Sid,std::int32_t)> dissolve; //450510은 signed 방향과 현재 패킷 WORD 세 개를 읽는다.
    std::function<void(Sid,std::uint32_t)> destroy; //현재 대상의 가상10이며 원소유자 플래그를 보존한다.
};
// 원본은 송신 owner/flags를 사용하지 않으며 첫 폴의 dead는 제외하지 않는다.
class RawAcidZap {
public:
    RawAcidZap(const SidPool& pool,std::span<const RiftTypeRecord> types,const AcidZapState& state,const FormPacketHeader& header,AcidZapHooks hooks); //같은10.78 raw/TYPE/현재 전역을 빌린다.
    std::array<std::uint8_t,11> Build(Sid victim,Sid first,Sid second,std::int8_t orientation) const; //40ad30의 현재 헤더와 세 WORD/방향 BYTE를 작성한다.
    void Receive(std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> packet) const; //소리→분해 생성→현재 가상 삭제의 순서다.
private:
    const SidPool& pool_; //처음 읽은 표적과 첫 폴은 효과 중 패킷이 변해도 유지한다.
    std::span<const RiftTypeRecord> types_; //현재 대상의 genus를 읽는 실제 메타다.
    const AcidZapState& state_; //현재 제단/성장형 TYPE다.
    const FormPacketHeader& header_; //선언 길이/프로토콜/메시지는 매 작성마다 다시 읽는다.
    AcidZapHooks hooks_; //같은 세션의 실제 하위 효과다.
};
}
