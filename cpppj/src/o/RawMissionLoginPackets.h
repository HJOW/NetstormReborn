// 현재Template SaveImage와로그인sendFort의실제0x2a예약/필드/길이확정을연결한다.
#pragma once
#include "o/RawFortImage.h"

namespace netstorm::o {
// 원본동적stack예약은wire보다3바이트크며그tail도ff로보존한다.
struct MissionFortPacket {
    std::vector<std::uint8_t> storage; //imageSize+20바이트의실제예약이다.최종wire는imageSize+17이다.
    // 현재길이WORD만빌린다.길이확정전/변조된범위밖선언은거부한다.
    std::span<const std::uint8_t> Bytes() const;
};
//004df590/004a7240의실제예약을작성한다.길이observer는별도원본문체로수행한다.
MissionFortPacket MakeMissionFortPacket(std::uint32_t owner,std::uint32_t predictable,std::uint8_t watcher,
    std::span<const std::uint8_t> image,std::uint16_t initialLength);
//0040a540의실제bool인자/ffffffff진단/하위WORD감김을수행한다.물리packet은17바이트이상이어야한다.
void FinalizeMissionFortPacket(std::span<std::uint8_t> packet,bool writeLength,std::function<void()> assertion={});
// Template선택은같은현재레코드를각원문위치에서읽으며VFS/파일로더를이모듈이대신하지않는다.
struct MissionFortPacketReads {
    std::function<const FortTemplate*(std::uint32_t)> current; //실제9 Template의현재manager이며NULL은미설치다.
    std::function<std::uint16_t()> initialLength; //004df590직전에읽는00544c48의현재WORD다.
};
// 실제선택된로그/송신/누락manager진단만외부경계다.
struct MissionFortPacketHooks {
    std::function<void(std::uint32_t,std::int32_t)> log; //004a68e0("sendFort from %d to %d\n",owner,target)이다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //현재연결gate를통과할실제최종wire송신이다.
    std::function<void()> missingTemplate; //004bf520의manager누락진단후실제현재manager를다시읽는다.
};
// 실효image와현재Player+58 watcher를작성한다.메타데이터/다른Player패킷을합성하지않는다.
class RawMissionLoginPackets {
public:
    // 같은9Player와현재Template/타입개수/송신수명은이객체보다오래살아야한다.
    RawMissionLoginPackets(OriginalEdition edition,std::span<const std::uint8_t> players,MissionFortPacketReads reads,
        FortImageHooks image,MissionFortPacketHooks hooks);
    //004a7240:Template없음은자연false,그외현재image작성→현재watcher→실제길이확정/송신이다.
    bool SendFort(std::uint32_t owner,std::int32_t target,std::uint32_t predictable) const;
private:
    std::span<const std::uint8_t> players_; //동일9×0xb4의현재+58 BYTE를읽으며watcher사본을두지않는다.
    MissionFortPacketReads reads_; //원문위치별현재Template/길이조회다.
    FortImageHooks image_; //실제Save의현재타입개수/선택진단이다.
    MissionFortPacketHooks hooks_; //현재로그/실제송신/누락manager진단경계다.
};
}
