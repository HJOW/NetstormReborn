// 원문00421f90의 현재 checking/다리 genus 진단 뒤 지연 낙하 예약 순서다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // 기존00421530 좌표 포장/Regular 생성을 그대로 재사용한다.
class SquidProcessHost;class RegularProcess; // 원문 자식 예약의 실제 호스트와 반환값을 빌린다.
struct BridgeFallParentHooks { // 선택된 진단/현재 타입/예약만 필수다.
    std::function<std::uint32_t(Sid)> genus; // checking이 비영일 때만 같은 현재 부모 타입을 읽는다.
    std::function<void(Sid,float,float)> schedule; // 진단 복귀 뒤에도 같은 SID와 원래 float 비트로21530을 호출한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // Bridge.cpp323의 실제 조건/파일/줄이며 복귀를 허용한다.
};
void RunBridgeFallParent(Sid bridge,float x,float y,RawCheckingFlag checking,const BridgeFallParentHooks& hooks); // 원문Patch전용 진단 부모이며 추가free/dead/종류 관문을 넣지 않는다.
RegularProcess* ScheduleBridgeFallChecked(SquidProcessHost& host,const SidPool& pool,std::span<const RiftTypeRecord> types,
    Sid bridge,float x,float y,RawCheckingFlag checking,
    const std::function<void(std::string_view,std::string_view,std::uint32_t)>& assertion={}); // 실제같은풀/현재TYPE를연결하며CD는기존직접자식계약을유지한다.
}
