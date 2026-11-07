// Client가 실제 CS 포인터 슬롯·할당·공통 배치 좌표를 장기 소유한다.
#pragma once
#include "o/RawChallengeState.h"
#include "client/NativeMetaProjection.h"
#include <functional>
#include <map>
#include <memory>

namespace netstorm::client { // 메타 서비스 객체와 지형 TYPE Outpost를 구별하는 원문 소유자다.
class ClientChallengeGlobals {
public:
    ClientChallengeGlobals()=default; // 새 Client의 CS/좌표 BSS0만 만들고 실제 객체는 Native 진입 때 확보한다.
    ClientChallengeGlobals(const ClientChallengeGlobals&)=delete;ClientChallengeGlobals& operator=(const ClientChallengeGlobals&)=delete; // 현재 슬롯/할당/빌린 물리를 복제하지 않는다.
    ClientChallengeGlobals(ClientChallengeGlobals&&)=delete;ClientChallengeGlobals& operator=(ClientChallengeGlobals&&)=delete; // 이미 빌린 DWORD/좌표 주소를 이동하지 않는다.
    std::uint32_t& Current();const std::uint32_t& Current() const; //005467d8의 실제 같은 DWORD를 Session/메타 입력이 빌린다.
    o::ChallengeGeometry& Geometry();const o::ChallengeGeometry& Geometry() const; // 같은 최초 초기화 표와 현재 표시 크기를 반환한다.
    MetaProjectionState& Projection();const MetaProjectionState& Projection() const; //00544d6c/5930a0/540adc·ae0의 같은 장기 투영/기준값을 빌린다.
    std::uint32_t& GeometryMode();const std::uint32_t& GeometryMode() const; //005467d4의현재DWORD이며0은고정22링/비영은공통CS배치반경을선택한다.
    std::uint32_t& ZoneGlyphType();const std::uint32_t& ZoneGlyphType() const; //005412dc의현재원격존그림TYPE이며보호PE초기161을같은장기주소에서읽는다.
    bool Prepare(const std::function<void()>& uninitialized); //State19의 새 생성 또는 listsInitialized 진단→기존Reset이다. State28의 직접Reset 부모는 이 생성/진단 경로와 구별한다.
    std::span<std::uint8_t> Bytes();std::span<const std::uint8_t> Bytes() const; // 현재 할당21328BYTE를 직접 빌린다.0/다른 포인터는 물리 경계에서 거부한다.
    std::span<std::uint8_t> Battle(std::uint32_t index); // 실제0..9의292BYTE 전투 기록이다.
    std::span<std::uint8_t> Participant(std::uint32_t index); // 실제0..39의460BYTE 참가자 기록이다.
    std::uint32_t AllocateArray(std::size_t bytes); // CS 가변 배열의 실제 할당을 별도32비트 토큰으로 소유한다. 실패 시0이며 기록 공개/원소 작성은 caller가 한다.
    std::span<std::uint8_t> ArrayBytes(std::uint32_t token); // 등록된 실제 배열 전체를 빌리고 임의 원본 주소를 호스트 포인터로 변환하지 않는다.
    void ClearArray(std::span<std::uint8_t> header); //0046d8a0의 두 비영 포인터 free 뒤 +0/+4/+c/+8 순서로 같은16BYTE를 지운다.
private:
    struct ArrayAllocation { // CS 가변 배열 한 할당의 실제 버퍼와 물리 길이다.
        std::unique_ptr<std::uint8_t[]> bytes; //원문 포인터 토큰이 지칭하는 실제 소유 버퍼다.
        std::size_t size; //같은 할당의 실제 BYTE 길이로 조회 범위를 제한한다.
    };
    std::uint32_t current_{},nextToken_{1}; // Native 현재0과 host 포인터를 자르지 않는 별도 비영 할당 토큰이다.
    o::ChallengeGeometry geometry_; // CS 객체 재사용/교체에서도 유지하는 같은 Client BSS다.
    MetaProjectionState projection_; // 실제 메타 기울기/높이와 최초 보정값이며 기록 Reset·Session 교체로 재생성하지 않는다.
    std::uint32_t geometryMode_{},zoneGlyphType_{161}; //005467d4의BSS0/005412dc의PE161이며CS Reset·Session 교체로초기화하지않는다.
    std::map<std::uint32_t,std::unique_ptr<std::uint8_t[]>> allocations_; // 교체된 실제 할당도 최종 Client 수명까지 소유한다. 명시 파생 해제 부모는 별도다.
    std::uint32_t nextArrayToken_{1};std::map<std::uint32_t,ArrayAllocation> arrays_; // CS 객체와 분리된 포인터 주소공간이며 재사용/Reset으로 실제 미반납 배열을 지우지 않는다.
};
}
