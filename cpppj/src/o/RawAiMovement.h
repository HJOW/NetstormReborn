// AI의 실제 경로 조회/정지·대상/좌표 이동·자기 신전 복귀를 같은 본체에서 처리한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawBoardTarget.h"
#include "o/RawCarrierMove.h"
#include <string>
#include <string_view>

namespace netstorm::o {
//0041d840의축별signed거리다.x차이만float에저장하고unordered에서는x를선택한다.
double AiDistanceToFootprint(RawPathPoint point,SquidFootprint bounds);
// 기존 실제 경로/의식/이름과 전송·로그 경계를 빌린다.
struct AiMovementHooks {
    std::function<std::uint32_t(std::uint32_t)> movement; //00488fe0의현재TYPE별실제경로TYPE다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,bool)> lookup; //004b1a90의같은종속조회/삭제다.1은getter인자가아닌remove다.
    std::function<SquidFootprint(Sid)> footprint; //004ade70(target,rectangle,0)의현재실제발자국이다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //004e2dc0(9,18BYTE)의실제요청송신이다.
    std::function<void(Sid,Sid)> sacrifice; //00494eb0의실제의식시작이다.
    std::function<std::uintptr_t(Sid,Sid,std::uint32_t,std::uint32_t)> target; //004894e0(actor,target,0,0)의실제접근/BFS/Host다.
    std::function<std::uintptr_t(RawPathStartRequest)> start; //00489080의실제부모/목적지/나머지인자0이다.
    std::function<std::string(Sid)> name; //현재가상68이름이다.디버그여부조회전원본순서대로실행한다.
    std::function<void(std::uint32_t,std::string_view,std::string_view)> logTarget; //현재AI/Move로그종류로Move source to target을출력한다.
    std::function<void(std::uint32_t,std::string_view,std::int32_t,std::int32_t)> logPoint; //원본ftol후Move source to (x,y)를출력한다.
    std::function<double(RawPathPoint,Sid)> distance; //0041d840의현재목표발자국거리다.출발좌표는효과전에저장한다.
};
// 사용자 이동의 선택해제/동일목적지관문을 섞지 않고 AI 원문 흐름만 처리한다.
class RawAiMovement {
public:
    RawAiMovement(RawAiBrainMemory& memory,const SidPool& pool,const bool& boss,const RawBoardTarget& board,const CarrierMoveState& packet,AiMovementHooks hooks); //같은PATCH풀/현재권한/기하/헤더를빌린다.
    Sid MovementForm(Sid actor) const; //00411630:현재TYPE의첫현재경로Form을조회한다.
    void Stop(Sid actor) const; //00411680:actor0무동작,그외같은현재TYPE/순번0/remove1이다.
    void ToTarget(std::uint32_t token,Sid actor,Sid target,std::uint32_t sacrifice) const; //004116d0의비서버송신/서버의식·대상접근/이름·로그순서다.
    void ToPoint(std::uint32_t token,Sid actor,RawPathPoint point,std::uint32_t sacrifice) const; //004117e0의현재권한재조회/실제경로시작/좌표로그다.
    void Home(std::uint32_t token,Sid actor) const; //004118a0:신전존재/거리2이상또는unordered면현재신전으로이동한다.
    const SidPool& Pool() const; //같은실제월드연결을확인한다.
private:
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; //현재살아있는실제0xb08본체DWORD만읽는다.
    RawPathPoint Position(Sid actor) const; //현재raw의두float좌표를읽는다.
    Sid PhysicalSid(std::uint32_t value) const; //현재본체의DWORD목표를물리검사뒤WORD로좁힌다.
    void Send(Sid actor,RawPathPoint point,std::uint32_t sacrifice) const; //같은현재헤더로18BYTE를작성해채널9에보낸다.
    RawAiBrainMemory& memory_; //생성자/전략과같은실제본체저장소다.
    const SidPool& pool_; //현재부모/목표의실제원문이다.
    const bool& boss_; //00540bc4의현재값을원본위치에서재조회한다.
    const RawBoardTarget& board_; //원본41d530의현재일반접근점기하다.
    const CarrierMoveState& packet_; //사용자이동과같은현재40h헤더다.
    AiMovementHooks hooks_; //선택된실제경로/의식/이름/로그효과다.
};
}
