// 실제 로그인 마지막의7DWORD 현재 설정 송신을 복원한다.
#pragma once
#include "o/RawPacketDispatch.h"
#include <functional>

namespace netstorm::o {
// packet payload의 필드 번호다. 실제 현재 조회 순서는 이 번호 순서와 다르다.
enum class MissionSettingsField : std::uint8_t {
    GeneratorTimer, //+4,005326cc의 현재 제네레이터 타이머 설정이다.
    HackMover, //+8,0055a488의 현재 이동 디버그 설정이다.
    NoDamageDebug, //+c,0055a48c의 현재 피해 디버그 설정이다.
    ShootMyFriends, //+10,0055a490의 현재 친군 사격 설정이다.
    FillMyTank, //+14,0055a494의 현재 탱크 설정 DWORD다.
    CombatGumpColumns, //+18,005326d0의 현재 전투 패널 열 수다.
    CombatGumpRows //+1c,005326d4의 현재 전투 패널 행 수다.
};
// 실제 현재 설정과 같은 연결 gate를 통과하는 송신만 외부 경계다.
struct MissionSettingsPacketHooks {
    std::function<std::uint32_t(MissionSettingsField)> current; //각 원본 load 위치에서 같은 현재 DWORD를 읽는다.
    std::function<void(MissionSettingsField,std::uint32_t)> setCurrent; //0044dc20의 각 원본 store 위치에서 같은 현재 DWORD를 직접 쓴다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //원본004e2dc0의 실제32BYTE 송신 경계다.
};
// 설정 사본을 두지 않고 Send마다 원본 읽기 순서로 현재값을 전송한다.
class RawMissionSettingsPacket {
public:
    //현재 getter의 대상은 이 객체보다 오래 살아야 한다. 미검증CD는 명시 거부한다.
    RawMissionSettingsPacket(OriginalEdition edition,MissionSettingsPacketHooks hooks);
    //0044dbc0→0040a700/004df230의 version1/type32/length32와7DWORD를 작성해 실제 송신을 요청한다.
    void Send(std::int32_t target) const;
    //0044dc20은 현재packet+4부터 각 DWORD를 재조회하고0..6순서로 같은 설정에 쓴다. 헤더 검증은 실제 분배기가 담당한다.
    void Receive(std::span<const std::uint8_t> packet) const;
private:
    MissionSettingsPacketHooks hooks_; //현재 getter와 실제 송신 수명만 빌리며 값 캐시는 없다.
};
//0040b91a가 선택한 실제0x32/version1/정책0/자연ret4 observer와 수신기를 등록한다.
void RegisterMissionSettingsPacket(RawPacketDispatch& dispatch,RawMissionSettingsPacket& packet);
}
