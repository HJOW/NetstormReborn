// 원본 Factory 업그레이드00453bf0 요청과00453d70의58h mode1 수신 접두를 복원한다.
#pragma once
#include "o/RawFactoryResearch.h"

namespace netstorm::o {
// 같은 원본 권한/검사/현재 시각을 복사하지 않고 빌린다.
struct FactoryUpgradeCommandState {
    const bool& boss; //00540bc4:수신 뒤 현재 권한이 있을 때만 원문을 방송한다.
    const bool& checking; //005e4794:수신의 Factory genus 진단을 선택한다.
    const double& now; //0055b4d0:요청 완료 시각과 수신 지연을 계산한다.
};
// 실제 분산SP/SharedRegular/전송과 원본 로그·진단을 연결한다.
struct FactoryUpgradeCommandHooks {
    std::function<std::int32_t(std::uint32_t)> cost; //0041df60의 현재 TYPE별 복호화 비용 단위다.
    std::function<void(std::uint32_t,float)> addSp; //0041dff0의 실제 로컬/원격 SP·같은 AI 지갑 가산이다.
    std::function<bool()> reserveShared; //원본 new(0x28)의 성공/실패이며 실패해도 나머지 요청/방송은 계속한다.
    std::function<void(Sid,std::uint32_t,float)> addShared; //예약 메모리에TYPE61 SharedRegular을 구성·부착하고 현재 시각을 쓴다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> send; //004e2dc0의 채널9 실제16바이트 송신이다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; //004e2e30의 현재 원문16바이트 방송이다.
    std::function<void(std::uint32_t,std::string_view)> log; //원본 종류4의 SharedRegular 예약 로그다.
    std::function<void(std::string_view)> assertion; //검사 모드의 원본 진단 뒤 현재 genus를 다시 읽는다.
};
// 실제 요청/수신은 복원하되 event3 진행/완료/예측 그림은 별도 가상 몸체에 맡긴다.
class RawFactoryUpgradeCommand {
public:
    RawFactoryUpgradeCommand(const SidPool& pool,std::span<const RiftTypeRecord> types,FactoryUpgradeCommandState state,FactoryUpgradeCommandHooks hooks); //같은 PATCH 풀/TYPE만 먼저 확인한다.
    void Request(Sid factory) const; //00453bf0:event4/20초 예약→현재now+15초의58h kind0/mode1 송신이다.
    void Receive(std::span<const std::uint8_t> packet) const; //00453d70의업그레이드 분기:비용→event3/최소1초·unordered 보존 예약→현재boss 방송이다.
    const SidPool& Pool() const; //같은 실제 월드 조립을 확인한다.
private:
    std::uint32_t Read(Sid factory,std::size_t offset) const; //현재 raw BYTE를 물리 검사 뒤 읽는다.
    std::uint32_t Genus(Sid factory) const; //현재 BYTE TYPE의flags2를 다시 읽는다.
    const SidPool& pool_; //현재 Factory의 상태/타입/소유자를 읽는 실제 풀이다.
    std::span<const RiftTypeRecord> types_; //같은 현재 TYPE 비용 분류다.
    FactoryUpgradeCommandState state_; //현재 원본 전역의 참조다.
    FactoryUpgradeCommandHooks hooks_; //선택된 실제 저장소/Host/전송/장치 효과다.
};
}
