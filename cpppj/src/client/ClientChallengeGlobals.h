// Client가 실제 CS 포인터 슬롯·할당·공통 배치 좌표를 장기 소유한다.
#pragma once
#include "o/RawChallengeState.h"
#include "o/RawTerrainBitmap.h" //같은 원문 bitmap 소유 입력을 CS Image 변환 선언에 제공한다.
#include "client/NativeMetaProjection.h"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string_view>

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
    std::uint32_t& ParticipantType();const std::uint32_t& ParticipantType() const; //00541208의현재참가자그림TYPE108을생성/카메라검색이공유한다.
    std::uint32_t& BattleType();const std::uint32_t& BattleType() const; //0054120c의현재전투섬그림TYPE109이며참가자/존그림과독립이다.
    bool Prepare(const std::function<void()>& uninitialized); //State19의 새 생성 또는 listsInitialized 진단→기존Reset이다. State28의 직접Reset 부모는 이 생성/진단 경로와 구별한다.
    std::span<std::uint8_t> Bytes();std::span<const std::uint8_t> Bytes() const; // 현재 할당21328BYTE를 직접 빌린다.0/다른 포인터는 물리 경계에서 거부한다.
    std::span<std::uint8_t> Battle(std::uint32_t index); // 실제0..9의292BYTE 전투 기록이다.
    std::span<std::uint8_t> Participant(std::uint32_t index); // 실제0..39의460BYTE 참가자 기록이다.
    std::span<const std::uint8_t> Battle(std::uint32_t index) const; //같은현재전투물리의읽기전용뷰이며별도기록을복사하지않는다.
    std::span<const std::uint8_t> Participant(std::uint32_t index) const; //같은현재참가자물리의읽기전용뷰다.
    std::optional<std::uint32_t> FindParticipantBySubscriber(std::uint32_t subscriber) const; //0042bb70:0..39의첫subscriber일치이며active/비0관문을추가하지않는다.실제0번과미발견을구별한다.
    std::span<const std::uint8_t> ParticipantNameBytes(std::int32_t participant,const std::function<void(std::string_view,std::uint32_t)>& assertion) const; //0042bbb0:1..39진단뒤현재active면같은40BYTE이름,아니면실제NUL1BYTE뷰다.읽기/문자열복사는caller가한다.
    std::uint32_t FreeBattleSlots(std::uint32_t battle) const; //0042b350:현재8개playerIds DWORD중0인개수다.별도전투active조건은없다.
    void UpdateBattleVersionBounds(std::int32_t battle,const std::function<void(std::string_view,std::uint32_t)>& assertion); //0042ba40:현재8슬롯의WORD버전필드집계→min256/max-1·최종min/max순서저장이다.
    void UpdateBattleVersionBounds(std::span<std::uint8_t> cs,std::int32_t battle,const std::function<void(std::string_view,std::uint32_t)>& assertion); //표시/진단 전 caller가 보유한 실제CS의 같은 전투/참가자 버전을 집계한다. current재조회와 구별한다.
    std::uint32_t AllocateArray(std::size_t bytes); // CS 가변 배열의 실제 할당을 별도32비트 토큰으로 소유한다. 실패 시0이며 기록 공개/원소 작성은 caller가 한다.
    std::span<std::uint8_t> ArrayBytes(std::uint32_t token); // 등록된 실제 배열 전체를 빌리고 임의 원본 주소를 호스트 포인터로 변환하지 않는다.
    void ClearArray(std::span<std::uint8_t> header); //0046d8a0의 두 비영 포인터 free 뒤 +0/+4/+c/+8 순서로 같은16BYTE를 지운다.
    void CopyImage(std::span<std::uint8_t> target,std::span<const std::uint8_t> source,const std::function<void(std::string_view,std::uint32_t)>& assertion); //0046d900의 해제 선행·현재 크기·두 실제 할당의 깊은 복사다. 자기 복사도 해제 순서를 보존한다.
    void ConvertImage(std::span<std::uint8_t> target,o::TerrainBitmapImage bitmap,bool crop,const std::function<void(std::string_view,std::uint32_t)>& assertion); //0046d790: 같은 CS20BYTE Image에 입력 bitmap→원문 VFX shape/크기→bitmap 반납을 수행한다. 기존 shape를 선행 해제하지 않는다.
    std::uint32_t& IslandAnimatorType();const std::uint32_t& IslandAnimatorType() const; //00541140의 현재 Rig TYPE52를 생성/검색이 공유한다.
    std::uint32_t& CameraAnimatorType();const std::uint32_t& CameraAnimatorType() const; //00541144의 별도 카메라 TYPE53이며 Session 교체로 다시 쓰지 않는다.
private:
    std::uint32_t islandAnimatorType_{52},cameraAnimatorType_{53}; // 보호 PE의 실제 두 프로세스 번호이며 CS 기록 Reset과 독립이다.
    struct ArrayAllocation { // CS 가변 배열 한 할당의 실제 버퍼와 물리 길이다.
        std::unique_ptr<std::uint8_t[]> bytes; //원문 포인터 토큰이 지칭하는 실제 소유 버퍼다.
        std::size_t size; //같은 할당의 실제 BYTE 길이로 조회 범위를 제한한다.
    };
    std::uint32_t current_{},nextToken_{1}; // Native 현재0과 host 포인터를 자르지 않는 별도 비영 할당 토큰이다.
    o::ChallengeGeometry geometry_; // CS 객체 재사용/교체에서도 유지하는 같은 Client BSS다.
    MetaProjectionState projection_; // 실제 메타 기울기/높이와 최초 보정값이며 기록 Reset·Session 교체로 재생성하지 않는다.
    std::uint32_t geometryMode_{},zoneGlyphType_{161}; //005467d4의BSS0/005412dc의PE161이며CS Reset·Session 교체로초기화하지않는다.
    std::uint32_t participantType_{108},battleType_{109}; //00541208/20c의보호PE초기값이며같은장기주소를실제소비자가빌린다.
    std::map<std::uint32_t,std::unique_ptr<std::uint8_t[]>> allocations_; // 교체된 실제 할당도 최종 Client 수명까지 소유한다. 명시 파생 해제 부모는 별도다.
    std::uint32_t nextArrayToken_{1};std::map<std::uint32_t,ArrayAllocation> arrays_; // CS 객체와 분리된 포인터 주소공간이며 재사용/Reset으로 실제 미반납 배열을 지우지 않는다.
};
}
