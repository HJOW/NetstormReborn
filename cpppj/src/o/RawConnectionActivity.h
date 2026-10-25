// 10.78의004b6180 수신 활동 기록과 실제0x64 등록만 기존 분배기에 연결한다.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o { // Native의 현재 표와 시계/상태 삭제 장치를 직접 빌리는 원문 몸체다.
class RawPacketDispatch; // 기존 버전/길이/observer 분배를 다시 구현하지 않는다.
inline constexpr std::size_t kConnectionActivityEntries=22; //005c84e8의 cleanup22DWORD와 같은 정상 연결 인덱스0..21이다.

// 두 표 사이의005c8540/544는 포함하지 않는 별개의 현재 DWORD 배열이다.
struct ConnectionActivityTables {
    std::span<std::uint32_t,kConnectionActivityEntries> frames; //005c84e8..005c853c의 마지막 수신 프레임이며0도 그대로 기록한다.
    std::span<std::uint32_t,kConnectionActivityEntries> wallSeconds; //005c8548..005c859c의 signed64 ftol 하위DWORD이며 CRT clock이 아니다.
};

// 원문 위치에서 현재값을 조회하며 UI adapter는 고정된나머지인자0/0/빈문자열을 보완한다.
struct ConnectionActivityHooks {
    std::function<double()> wallSeconds; //00460d70을 매 호출 새로 읽으며 분배기의 앞선 now를 재사용하지 않는다.
    std::function<std::uint32_t()> frameNumber; //0055b4a8의 현재 하위DWORD이며 확장된FrameTime.number를 여기서 축소한다.
    std::function<bool()> server; //wall표 저장 뒤00540bc0을 읽어 두 상태 삭제 인자 가운데 하나를 고른다.
    std::function<void(std::uint32_t,std::uint32_t)> eraseStatus; //004d3090(2,sender,0,0,빈문자열) 또는(1,0,0,0,빈문자열)의 실제 부모 adapter다.
};

// packet payload/connection을 읽지 않는004b6180과 Timer의 같은 직접 호출을 공유한다.
class RawConnectionActivity {
public:
    RawConnectionActivity(ConnectionActivityTables tables,ConnectionActivityHooks hooks); // 표/장치는 이 몸체보다 오래 살아야 하며 모든 효과를 실제 장치로 제공한다.
    void Receive(std::int32_t sender) const; // 정상0..21만 처리한다. 범위밖은 원문 메모리 덮기를 흉내 내지 않고 첫 효과 전에 명시 거부한다.
private:
    ConnectionActivityTables tables_; // 같은 Client owner의 두 실제 표를 빌리며 복사본을 동기화하지 않는다.
    ConnectionActivityHooks hooks_; // wall/frame/server/상태 삭제의 원래 조회·쓰기 순서를 유지한다.
};

void RegisterConnectionActivity(RawPacketDispatch& dispatch,RawConnectionActivity& activity); //0x64/version1/정책0/빈0040b380 observer이며 현재 길이표를 덮지 않는다. 등록 해제 전까지 activity가 살아 있어야 한다.
}
