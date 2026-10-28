//004227e0/00421c30의 다리 수명·직접 프레임 쓰기와 실제 효과 호출 순서를 복원한다.
#pragma once
#include "o/RawBridgeDecayScan.h"
#include "o/Graph.h"

namespace netstorm::o { //원문 슬롯/표면 지도는 호출 중에도 현재값을 읽는다.
struct RawAuthorityState; //현재 Client 서버 전역의 주소만 빌리며 별도 사본을 만들지 않는다.
//가상 삭제/표시·무상태 표면 진단·이동체 확인·위치 소리만 같은 현재 월드에서 공급한다.
struct RawBridgeDecayHooks {
    std::function<void(Sid,std::uint32_t)> destroy; //004220f0의 실제 가상10이며 거부/재귀 삭제도 호출자가 처리한다.
    std::function<void(Sid)> notifySurface; //004214a0은 수명 WORD를 쓴 뒤 호출한다.
    std::function<void(Sid)> refresh; //가상88이며 프레임 Set/Unpop/Pop으로 바꾸지 않는다.
    std::function<void(Sid)> carriers; //004202f0 이후 현재 칸 위 이동체를 확인한다.
    std::function<void(Sid)> crackSound; //방문 칸 처리 뒤 원천 현재 위치에서 bridgeCrack.wav를 재생한다.
};
//원문 한 칸은 이웃/방문 목록을 먼저 모은 뒤 실제 슬롯에 순서대로 수명을 적용한다.
class RawBridgeDecay {
public:
    RawBridgeDecay(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
        std::span<const std::uint16_t> surface,std::span<const std::uint8_t> spots,std::span<const GraphRecord> graphs,
        const BridgeDecayMode& mode,RawBridgeDecayHooks hooks,const RawAuthorityState* authority=nullptr); //실제 Client는 같은 현재 서버 주소를 빌리며 기존 독립 입력만 mode fallback을 쓴다.
    void Cell(Sid root) const; //작은 Graph→Hard→이웃/열린 끝→방문 목록→현재 수명/삭제 순서다. 원문 예외는 바깥 스캔이 처리한다.
    void ApplyLife(Sid sid,std::int32_t reduction) const; //정상7 초과를 미리 거부하지 않고 원문의 signed 산술/4비트 쓰기를 보존한다.
private:
    std::span<std::uint8_t> Bytes(Sid sid) const; //예약/반납 상태의 새 관문 없이 실제 물리 슬롯만 범위 검사한다.
    FrameCode Code(Sid sid) const; //현재 TYPE와 DWORD 프레임을 같은 실제 코드 표에서 읽는다.
    std::vector<Sid> Neighbors(Sid sid) const; //실제 float 원천의 flag8 표면 순서를 같은 현재 지도에서 구한다.
    BridgeDecayWalk Collect(Sid root) const; //원문100칸 목록과 경계 삭제/접합 재귀를 보존한다.
    SidPool& pool_; //WORD/프레임이 실제로 바뀌는 단일50바이트 풀이다.
    std::span<const RiftTypeRecord> types_; //현재 genus/발자국 표다.
    std::span<const RiftTypeFrames> frames_; //현재 TYPE의 방향·금/Hard 프레임 코드다.
    std::span<const std::uint16_t> surface_; //flag8과 열린 끝이 함께 읽는 실제 hash0 머리다.
    std::span<const std::uint8_t> spots_; //source 내부 AND/후보 기준점의 현재 비트다.
    std::span<const GraphRecord> graphs_; //Graph 분할/반납의 변경도 바로 보이는255행 배열이다.
    const BridgeDecayMode& mode_; //같은 현재 server/authority/editor/개발용 표시 상태다.
    RawBridgeDecayHooks hooks_; //필요한 효과는 도달 시점에 검사하며 앞선 실제 쓰기를 보존한다.
    const RawAuthorityState* authority_{}; //수명0의 제거 관문은 효과 뒤에도 현재 server를 직접 읽는다.
};
}
