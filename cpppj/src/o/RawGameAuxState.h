// 게임 보조 전역의 실제 폭만 소유하며 Runtime/목록/Host를 포함하지 않는다.
#pragma once
#include <bit>
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::o { // Gameplay/Storage가 같은 주소를 pin하는 작은 게임 데이터 owner다.
class RawGameAuxState {
public:
    RawGameAuxState()=default; // PE의 BSS+0과 명시25/2000/122·세계여백2/장식타입4개를 한 번 구성한다.
    RawGameAuxState(const RawGameAuxState&)=delete;RawGameAuxState& operator=(const RawGameAuxState&)=delete; // 살아 있는 실제 참조 주소를 복제하지 않는다.
    std::uint32_t& OrdinaryGeysers() { return ordinaryGeysers_; } //005950ac: Start가 moreGeysers 부정을 쓴다.
    const std::uint32_t& OrdinaryGeysers() const { return ordinaryGeysers_; } // 현재 DWORD를 복사 없이 읽는다.
    std::uint32_t& GeyserFrameFlag() { return geyserFrameFlag_; } //005950b0:486bc0의 미복원 프레임 입력이다.
    const std::uint32_t& GeyserFrameFlag() const { return geyserFrameFlag_; } // 미복원 부모를 상수0으로 대신하지 않는다.
    std::uint32_t& SavedGeyserCount() { return savedGeyserCount_; } //00595090:SavedStateRead.tail[0]의 현재 출력 저장소다.
    const std::uint32_t& SavedGeyserCount() const { return savedGeyserCount_; } // 저장 reader 연결 전에도 동일 값을 보존한다.
    double& GeyserDeadline() { return geyserDeadline_; } //005950b8: 게임 시각의 별도8BYTE 기한이다.
    const double& GeyserDeadline() const { return geyserDeadline_; } // 다른0 기한과 합치지 않는다.
    double& CleanupDeadline() { return cleanupDeadline_; } //005950c0: 원문 cleanup이 쓰는 별도8BYTE다.
    const double& CleanupDeadline() const { return cleanupDeadline_; } // 의미 미확정의 현재 값을 그대로 빌린다.
    double& SecondaryGeyserDeadline() { return secondaryGeyserDeadline_; } //005950c8: b8과 별개인 게임 프레임 기한이다.
    const double& SecondaryGeyserDeadline() const { return secondaryGeyserDeadline_; } // 현재8BYTE를 그대로 읽는다.
    double& ScheduledDrawDeadline() { return scheduledDrawDeadline_; } //00595358: Draw/Dais 부모가 읽는 별도 게임 기한이다.
    const double& ScheduledDrawDeadline() const { return scheduledDrawDeadline_; } // 미복원 부모의 예약을 Session 생성으로 지우지 않는다.
    std::int32_t& TotalCost() { return totalCost_; } //0059508c: 실제 Pop/삭제 장부의 signed 비용 DWORD다.
    const std::int32_t& TotalCost() const { return totalCost_; } // 세 집계 배열과 구별되는 한 DWORD다.
    std::int32_t& ResourcePool() { return resourcePool_; } //00595088: Reward/LocalGeyser가 직접 공유하는 signed 물리다.
    const std::int32_t& ResourcePool() const { return resourcePool_; } // 음수 비트도 정규화하지 않는다.
    std::uint32_t ResourcePoolBits() const { return std::bit_cast<std::uint32_t>(resourcePool_); } // Login/Bomb의 unsigned 조회만 명시 비트 변환한다.
    void StoreResourcePoolBits(std::uint32_t value) { resourcePool_=std::bit_cast<std::int32_t>(value); } // 다른 타입의 참조를 alias하지 않고 같은 DWORD를 쓴다.
    std::uint32_t& RefundPercent() { return refundPercent_; } //005424bc: Reward/Login/BattleOptions가 공유하는 초기25 DWORD다.
    const std::uint32_t& RefundPercent() const { return refundPercent_; } // signed 산술 해석은 각 원문 소비자가 수행한다.
    std::int32_t& AllowAnyCapture() { return allowAnyCapture_; } //005453e4: atol 결과와 pickup 비0 관문이 같은 DWORD다.
    const std::int32_t& AllowAnyCapture() const { return allowAnyCapture_; } // bool로 줄이기 전 현재 raw 값을 읽는다.
    std::uint32_t& Archipelago() { return archipelago_; } //005c8bb0: 실제 FarmCluster 모드이며 Login의bb4와 다르다.
    const std::uint32_t& Archipelago() const { return archipelago_; } // 비정규 비영 DWORD도 보존한다.
    float& LostX() { return lostX_; } //005c848c: 공통 삭제가 소리 뒤 기록하는 최근 손실X다.
    const float& LostX() const { return lostX_; } // 카메라/건설 이력과 별개다.
    float& LostY() { return lostY_; } //005c8490: 최근 손실Y이며 cleanup은X보다 먼저 쓴다.
    const float& LostY() const { return lostY_; } // 저장된 float 비트를 그대로 읽는다.
    std::int32_t& DefaultSp() { return defaultSp_; } //00540bdc: 초기2000 signed 정수이며 TYPE.cost에는 float로 변환해 쓴다.
    const std::int32_t& DefaultSp() const { return defaultSp_; } // 미션 AI의 별도 시작SP와 합치지 않는다.
    std::uint32_t& GeyserType() { return geyserType_; } //00541240: 실제 현재 간헐천 TYPE이며 Host pollType도 이 주소를 pin한다.
    const std::uint32_t& GeyserType() const { return geyserType_; } // 옵션/생성/목록의 같은 현재 TYPE다.
    std::uint32_t& TerrainType() { return terrainType_; } //005411dc의 같은 현재 지면 TYPE를 영역 조회/진단이 빌린다.
    const std::uint32_t& TerrainType() const { return terrainType_; } // Session 교체로97의 새 사본을 만들지 않는다.
    std::uint32_t& EdgeFarmType() { return edgeFarmType_; } //0054123c의 같은 현재 가장자리 농장 TYPE다.
    const std::uint32_t& EdgeFarmType() const { return edgeFarmType_; } // 객체 덤프의 단순 좌표 행도 이 주소를 읽는다.
    std::int32_t& WorldGeyserMargin() { return worldGeyserMargin_; } //00542654의 현재 여백은 전체/분할 범위와 후속 생성이 같은 주소를 쓴다.
    const std::int32_t& WorldGeyserMargin() const { return worldGeyserMargin_; } //Session 교체로 새2를 쓰지 않고 현재값을 보존한다.
    std::span<std::uint32_t,4> WorldDecorationTypes() { return worldDecorationTypes_; } //005411e8/00541338/005411ec/0054133c의 현재 네 타입이다.
    std::span<const std::uint32_t,4> WorldDecorationTypes() const { return worldDecorationTypes_; } //생성 부모는 Local 뒤 snapshot과 반복 중 현재 Monument를 구별한다.
    std::uint32_t& SubscriberMarker() { return subscriberMarker_; } //005467bc: Subscriber의 별도 표시 DWORD다.
    const std::uint32_t& SubscriberMarker() const { return subscriberMarker_; } // 다른 Subscriber ID와 합치지 않는다.
    bool& PriestQuarterHp() { return priestQuarterHp_; } //005ca8f0의 같은 현재 0/1을 생성·HP·연소와 State 정리가 공유한다.
    const bool& PriestQuarterHp() const { return priestQuarterHp_; } // Session을 교체하거나 Frame을 기다리지 않고 현재값을 읽는다.
    void ResetForGeneralWorld() { // 기존 지역 Runtime의 fresh 입력만 호환하며 Native cleanup 전체가 아니다.
        ordinaryGeysers_=0;totalCost_=0;resourcePool_=0;refundPercent_=25;allowAnyCapture_=0;archipelago_=0;lostX_=0.0F;lostY_=0.0F;geyserType_=122;priestQuarterHp_=false;
    }
    void ResetLostPositionForCleanup() { // Native 두 store만 Y→X로 실행하며 일반 fresh의+0과 구별한다.
        lostY_=std::bit_cast<float>(0xc61c4000U);lostX_=std::bit_cast<float>(0xc61c4000U);
    }
private:
    bool priestQuarterHp_{}; // 원문 최초 BSS0이며 일반 fresh의 기존0만 호환한다. prepared와 반납은 현재값을 보존한다.
    std::uint32_t ordinaryGeysers_{},geyserFrameFlag_{},savedGeyserCount_{}; // 실제 세 BSS DWORD다.
    double geyserDeadline_{},cleanupDeadline_{},secondaryGeyserDeadline_{},scheduledDrawDeadline_{}; // 실제 서로 다른 네 BSS double이다.
    std::int32_t totalCost_{},resourcePool_{}; // 같은 signed 비용/자원 DWORD다.
    std::uint32_t refundPercent_{25}; // 보호 PE의 초기25다.
    std::int32_t allowAnyCapture_{}; // bool과 alias하지 않는 raw 허용 DWORD다.
    std::uint32_t archipelago_{}; // 원본bb0의 별도 모드다.
    float lostX_{},lostY_{}; // 최초BSS+0이며 cleanup의-10000 입력과 구별한다.
    std::int32_t defaultSp_{2000}; // 보호 PE의 signed 기본SP다.
    std::uint32_t geyserType_{122},subscriberMarker_{}; // 보호 PE TYPE122와 별도BSS 표지다.
    std::uint32_t terrainType_{97},edgeFarmType_{121}; // 보호 PE 두 TYPE 전역이며 현재 Runtime 대여/반납은 초기화하지 않는다.
    std::int32_t worldGeyserMargin_{2}; //보호 PE00542654의 실제 초기 DWORD다.
    std::array<std::uint32_t,4> worldDecorationTypes_{100,184,101,185}; //실제 treeTwo/RUIN/treeThree/Monument 타입 순서다.
};
static_assert(sizeof(std::uint32_t)==4 && sizeof(std::int32_t)==4 && sizeof(float)==4 && sizeof(double)==8); // 각 Native폭만 보증하며 클래스가17주소의 연속 image라는 주장은 아니다.
}
