//게임 보조 전역과 NUL 종료 타입 사용256BYTE를 소유하며 Runtime/Host 객체는 포함하지 않는다.
#pragma once
#include "o/RawPathData.h" //Path의현재DWORD선언길이와헤더owner를Session밖에유지한다.
#include "o/RawSquidTransmission.h" // 같은Client의공통버퍼/압축전역을World/Session과공유한다.
#include <bit>
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::o { // Gameplay/Storage가 같은 주소를 pin하는 작은 게임 데이터 owner다.
class RawGameAuxState {
public:
    std::array<std::uint8_t,256>& TypeUse() { return typeUse_; } //005451f8의 NUL 종료 타입 사용 목록이며 Login/69h가 같은 장기256BYTE를 직접 빌린다.
    const std::array<std::uint8_t,256>& TypeUse() const { return typeUse_; } //World/Session 대여와 반납이 현재 목록을 복사하거나 초기화하지 않는다.
    std::array<std::uint8_t,2>& Packet0cHeader() { return packet0cHeader_; } //0052ed54/55의현재0ch메시지/프로토콜이며원문버전조회/디버그burst의공용전역을보관한다.
    const std::array<std::uint8_t,2>& Packet0cHeader() const { return packet0cHeader_; } //Session/World교체뒤에도같은현재BYTE두개를읽는다.
    std::uint16_t& Packet0cInitialLength() { return packet0cInitialLength_; } //00544c24의BSS WORD0을생성순간읽으며실제송신observer의최종길이와구별한다.
    const std::uint16_t& Packet0cInitialLength() const { return packet0cInitialLength_; } //설정키나성공토글없이현재공용생성길이만빌린다.
    std::uint32_t& StateNotifyType() { return stateNotifyType_; } //005411a0:004ac220이 상태WORD 변경 뒤 표면 통지를 선택하는 현재 TYPE다.
    const std::uint32_t& StateNotifyType() const { return stateNotifyType_; } // World/Session 교체에서82로 다시 쓰지 않는다.
    RawGameAuxState()=default; // PE의 BSS+0과 명시25/2000/122·세계여백2/장식타입4개를 한 번 구성한다.
    RawGameAuxState(const RawGameAuxState&)=delete;RawGameAuxState& operator=(const RawGameAuxState&)=delete; // 살아 있는 실제 참조 주소를 복제하지 않는다.
    std::uint32_t& OrdinaryGeysers() { return ordinaryGeysers_; } //005950ac: Start가 moreGeysers 부정을 쓴다.
    const std::uint32_t& OrdinaryGeysers() const { return ordinaryGeysers_; } // 현재 DWORD를 복사 없이 읽는다.
    std::uint32_t& GeyserFrameFlag() { return geyserFrameFlag_; } //005950b0:486bc0이 SP 지급 가지를 생략하는 현재 입력이다.
    const std::uint32_t& GeyserFrameFlag() const { return geyserFrameFlag_; } // 같은 프레임 관문을 상수0으로 대신하지 않는다.
    std::uint32_t& SavedGeyserCount() { return savedGeyserCount_; } //00595090:SavedStateRead.tail[0]의 현재 출력 저장소다.
    const std::uint32_t& SavedGeyserCount() const { return savedGeyserCount_; } // 저장 reader 연결 전에도 동일 값을 보존한다.
    std::uint32_t& SpellCount() { return savedGeyserCount_; } //00595090은0041cab0/00485f50의 스펠 수다. 기존 저장 reader의 역사적 getter와 같은 DWORD를 빌린다.
    const std::uint32_t& SpellCount() const { return savedGeyserCount_; } // 같은 실제 DWORD의 현재 의미를 별도 사본 없이 읽는다.
    double& SpellDeadline() { return spellDeadline_; } //005950d0: 일반 초기화가 현재 게임 시각+180을 쓰는 별도 예약이다.
    const double& SpellDeadline() const { return spellDeadline_; } // b8/c0/c8 기한과 합치지 않는다.
    std::uint32_t& IncomePrimed() { return incomePrimed_; } //00595124의 별도 BSS0이며 최초 SP 만료는 지급 대신1만 쓴다.
    double& GeyserDeadline() { return geyserDeadline_; } //005950b8: 게임 시각의 별도8BYTE 기한이다.
    const double& GeyserDeadline() const { return geyserDeadline_; } // 다른0 기한과 합치지 않는다.
    double& CleanupDeadline() { return cleanupDeadline_; } //005950c0: 원문 cleanup이 쓰는 별도8BYTE다.
    const double& CleanupDeadline() const { return cleanupDeadline_; } // 의미 미확정의 현재 값을 그대로 빌린다.
    double& SecondaryGeyserDeadline() { return secondaryGeyserDeadline_; } //005950c8: b8과 별개인 게임 프레임 기한이다.
    const double& SecondaryGeyserDeadline() const { return secondaryGeyserDeadline_; } // 현재8BYTE를 그대로 읽는다.
    std::uint8_t& BattleFinishFrames() { return battleFinishFrames_; } //0055a44c의BSS0 BYTE이며44ac30의signed비교/증가와실제결과확정만쓴다.
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
    std::uint32_t& TerrainEnabled() { return terrainEnabled_; } //00540a2c/CD00535ae0의 현재 생성/Zone 보정 DWORD다.
    const std::uint32_t& TerrainEnabled() const { return terrainEnabled_; } // 설정 뒤 최초1이며 Session 교체로 새 값을 쓰지 않는다.
    std::int32_t& ArchipelagoMinimum() { return archipelagoMinimum_; } //0054264c/CD0053717c의 현재 밀도 하한이다.
    const std::int32_t& ArchipelagoMinimum() const { return archipelagoMinimum_; } // 난수 범위를 계산하는 시점의 같은 signed DWORD다.
    std::int32_t& ArchipelagoMaximum() { return archipelagoMaximum_; } //00542650/CD00537180의 현재 밀도 상한이다.
    const std::int32_t& ArchipelagoMaximum() const { return archipelagoMaximum_; } // 최초35와 이후 원문 writer의 현재값을 구별한다.
    std::uint32_t& ArchipelagoPercent() { return archipelagoPercent_; } //005c8bb4/CD0053718c의 강제 밀도이며 bb0 결과와 별개다.
    const std::uint32_t& ArchipelagoPercent() const { return archipelagoPercent_; } // Login metadata와 생성 root가 같은 현재 DWORD를 읽는다.
    float& LostX() { return lostX_; } //005c848c: 공통 삭제가 소리 뒤 기록하는 최근 손실X다.
    const float& LostX() const { return lostX_; } // 카메라/건설 이력과 별개다.
    float& LostY() { return lostY_; } //005c8490: 최근 손실Y이며 cleanup은X보다 먼저 쓴다.
    const float& LostY() const { return lostY_; } // 저장된 float 비트를 그대로 읽는다.
    std::int32_t& DefaultSp() { return defaultSp_; } //00540bdc: 초기2000 signed 정수이며 TYPE.cost에는 float로 변환해 쓴다.
    const std::int32_t& DefaultSp() const { return defaultSp_; } // 미션 AI의 별도 시작SP와 합치지 않는다.
    std::uint32_t& GeyserType() { return geyserType_; } //00541240: 실제 현재 간헐천 TYPE이며 Host pollType도 이 주소를 pin한다.
    const std::uint32_t& GeyserType() const { return geyserType_; } // 옵션/생성/목록의 같은 현재 TYPE다.
    std::uint32_t& FieryTailProcessType() { return fieryTailProcessType_; } //0054109c: 불꽃 생성자가 공통 부착에 전달하는 현재 TYPE다.
    const std::uint32_t& FieryTailProcessType() const { return fieryTailProcessType_; } // 현재 생성 함수 선택과 별개인 생성자 내부 전역을 보존한다.
    std::uint32_t& StreakProcessType() { return streakProcessType_; } //005410a0: Teleport 생성 함수 선택·표시 부착·도착 삭제가 공유하는 현재 TYPE다.
    const std::uint32_t& StreakProcessType() const { return streakProcessType_; } // Runtime 교체/일반 월드 준비로12의 새 사본을 만들지 않는다.
    std::uint32_t& RegularProcessType() { return regularProcessType_; } //00541128: 일반 예약 생성과 종속 조회가 공유하는 현재 DWORD다.
    std::uint32_t& GunProcessType() { return gunProcessType_; } //005410f8의 현재 총 프로세스 TYPE를 신규 생성/전체 수신이 함께 읽는다.
    std::uint32_t& DamageRegularEvent() { return damageRegularEvent_; } //005326c0의실제현재6699이며장기Handle이현재비교위치에서빌린다.
    const std::uint32_t& DamageRegularEvent() const { return damageRegularEvent_; } //World교체로다시초기화하지않는다.
    std::uint32_t& ConstructionRegularEvent() { return constructionRegularEvent_; } //005326c4의실제현재6700이며abstract건설재시도의현재비교값이다.
    const std::uint32_t& ConstructionRegularEvent() const { return constructionRegularEvent_; } //현재DWORD를상수사본으로동기화하지않는다.
    std::uint32_t& ConstructionProcessType() { return constructionProcessType_; } //005410e8의 현재30이며 일반 생성/수신/볼텍스가 같은 DWORD를 읽는다.
    std::uint32_t& DaisProcessType() { return daisProcessType_; } //00541158의 현재58이며 asset DaisType165와 별개인 수신 부착 TYPE다.
    const std::uint32_t& DaisProcessType() const { return daisProcessType_; } // World/Session 교체에서 같은 장기 DWORD를 재초기화하지 않는다.
    std::span<std::uint8_t,2> GunProcessHeader() { return gunProcessHeader_; } //005409b4/5의현재메시지/프로토콜두BYTE를실제writer가읽는다.
    std::span<const std::uint8_t,2> GunProcessHeader() const { return gunProcessHeader_; } //같은장기두BYTE의읽기뷰이며Session교체로초기화하지않는다.
    std::span<std::uint8_t,2> FlyerProcessHeader() { return flyerProcessHeader_; } //00532780/1의현재메시지/프로토콜두BYTE를실제writer가읽는다.
    std::span<const std::uint8_t,2> FlyerProcessHeader() const { return flyerProcessHeader_; } //같은장기두BYTE의읽기뷰이며Session교체로초기화하지않는다.
    std::span<std::uint8_t,2> SunAviaryHeader() { return sunAviaryHeader_; } //00532786/7의현재메시지/프로토콜두BYTE를실제writer가읽는다.
    std::span<const std::uint8_t,2> SunAviaryHeader() const { return sunAviaryHeader_; } //같은장기두BYTE의읽기뷰이며Session교체로초기화하지않는다.
    std::span<std::uint8_t,2> WindAviaryHeader() { return windAviaryHeader_; } //0053278a/b의현재메시지/프로토콜두BYTE를실제writer가읽는다.
    std::span<const std::uint8_t,2> WindAviaryHeader() const { return windAviaryHeader_; } //같은장기두BYTE의읽기뷰이며Session교체로초기화하지않는다.
    std::span<std::uint8_t,2> RainAviaryHeader() { return rainAviaryHeader_; } //0053278e/f의현재메시지/프로토콜두BYTE를실제writer가읽는다.
    std::span<const std::uint8_t,2> RainAviaryHeader() const { return rainAviaryHeader_; } //같은장기두BYTE의읽기뷰이며Session교체로초기화하지않는다.
    std::span<std::uint8_t,2> BombTimerHeader() { return bombTimerHeader_; } //0052ee9a/b의현재메시지/프로토콜두BYTE를실제writer가읽는다.
    std::span<const std::uint8_t,2> BombTimerHeader() const { return bombTimerHeader_; } //같은장기두BYTE의읽기뷰이며Session교체로초기화하지않는다.
    std::uint32_t& BombTimerInitialLength() { return bombTimerInitialLength_; } //00544c98의현재DWORD를최소4비교전에WORD로좁히지않는다.
    const std::uint32_t& BombTimerInitialLength() const { return bombTimerInitialLength_; } //독립반환크기22+2count와구별되는현재선언입력이다.
    RawPathDataHeader& PathPacketHeader() { return pathPacketHeader_; } //0052edce/cf·00544c50의같은현재두BYTE/DWORDowner다.
    const RawPathDataHeader& PathPacketHeader() const { return pathPacketHeader_; } //현재Path헤더를복제/재초기화하지않는읽기뷰다.
    std::span<std::uint8_t,2> DaisProcessHeader() { return daisProcessHeader_; } //00532690/91의 현재6a/message·1/protocol BYTE를 그대로 빌린다.
    std::span<const std::uint8_t,2> DaisProcessHeader() const { return daisProcessHeader_; } // writer의 protocol bit7도 지우지 않는 같은 두BYTE다.
    std::span<std::uint8_t,2> ConstructionHeader() { return constructionHeader_; } //005325b8/b9의 현재 메시지/프로토콜BYTE를 그대로 빌린다.
    std::int32_t& PathTraceLevel() { return pathTraceLevel_; } //00540c68의 현재 경로 수신 로그 수준이며 FILE 포인터와 다르다.
    std::uint32_t& IslandDropBlocked() { return islandDropBlocked_; } //0054db84의 섬 정리 억제 DWORD이며 로딩 깊이와 독립인 장기 BSS다.
    std::uint32_t& IslandStumpType() { return islandStumpType_; } //005411d8의 현재 그루터기 TYPE이며 섬 정리의 가상 삭제 비교가 읽는다.
    std::array<std::uint32_t,7>& MissionSettings() { return missionSettings_; } //32h의 현재7DWORD이며 각 원문 전역을 같은 Client 수명에서 순서대로 읽고 쓴다.
    const std::array<std::uint32_t,7>& MissionSettings() const { return missionSettings_; } // 진단/송신도 Login이나 Session 사본을 만들지 않는다.
    std::uint32_t& BuildReplayProcessType() { return buildReplayProcessType_; } //00541134의 현재 프로세스 TYPE이며 재생 생성자가 판독 뒤 읽는다.
    std::array<std::uint8_t,2>& ConstructionRequestHeader() { return constructionRequestHeader_; } //0052ede6/7의 실제3e/type·version BYTE 두 개다.
    std::uint16_t& ConstructionRequestLength() { return constructionRequestLength_; } //0052ede8의 현재 생성 길이WORD이며 송신 observer의 다른 길이와 구별한다.
    std::uint32_t& QuickBuild() { return quickBuild_; } //0055a49c의 현재 건설 즉시 완료 DWORD다.
    std::uint32_t& QuickVortex() { return quickVortex_; } //0055a498의 현재 신전 건설 즉시 완료 DWORD다.
    std::int32_t& DeveloperScatterRow() { return developerScatterRow_; } //005326d8의 현재 signed 행 번호이며 실제 scatter의 CRT 사용과 함께 유지한다.
    std::uint32_t& DeveloperCombatResizeCommand() { return developerCombatResizeCommand_; } //0054288c의 현재 입력 명령 DWORD를 cr 분기가 큐에 넣는다.
    std::uint32_t& FireAnimationProcessType() { return fireAnimationProcessType_; } //005410fc의 현재35를 전체 발사 수신 부모가 조회한다.
    std::uint32_t& RotateAnimationProcessType() { return rotateAnimationProcessType_; } //00541100의 현재36이며 발사와 별개인 장기 DWORD다.
    std::uint32_t& SunArcherType() { return sunArcherType_; } //00541174의 현재71이며 수신 공장의 첫 발사 TYPE 비교다.
    std::uint32_t& SunCannonType() { return sunCannonType_; } //00541258의 현재128이며 발사/회전 수신이 같은 번호를 읽는다.
    std::uint32_t& WindArcherType() { return windArcherType_; } //00541288의 현재140이며 회전 수신 공장의 두 번째 비교다.
    std::span<std::uint8_t,2> RotateAnimationHeader() { return rotateAnimationHeader_; } //00540468/69의 현재 메시지/프로토콜 두BYTE만 빌린다. 선언 길이는 이 writer가 읽지 않는다.
    std::span<std::uint8_t,2> FireAnimationHeader() { return fireAnimationHeader_; } //0054046e/6f의 현재 두BYTE이며 회전 헤더와 합치지 않는다.
    std::uint32_t& SunAviaryProcessType() { return sunAviaryProcessType_; } //005410f0의 현재 훨리베이스 프로세스 TYPE다.
    std::uint32_t& WindAviaryProcessType() { return windAviaryProcessType_; } //0054115c의 현재 데빌메이커 프로세스 TYPE다.
    std::uint32_t& RainAviaryProcessType() { return rainAviaryProcessType_; } //00541160의 현재 맨 오 워 메이커 프로세스 TYPE다.
    std::uint32_t& ProcessFormType() { return processFormType_; } //00541078/CD0051c968: 공통 Process 부모의 Create/Take가 읽는 현재 DWORD다.
    std::uint32_t& DependFormType() { return dependFormType_; } //00541074/CD0051c964: 감시 쌍의 두 Create와 검색이 각각 읽는 현재 전체 DWORD다.
    const std::uint32_t& DependFormType() const { return dependFormType_; } // World 교체/일반 준비에서도 같은 장기 주소를 유지한다.
    const std::uint32_t& ProcessFormType() const { return processFormType_; } // World 교체 뒤에도 같은 주소를 유지하며 BYTE로 줄이지 않는다.
    const std::uint32_t& RegularProcessType() const { return regularProcessType_; } // Session 교체 중에도 같은 주소를 빌린다.
    std::uint32_t& SharedRegularProcessType() { return sharedRegularProcessType_; } //00541164: 공유 예약의 현재 번호이며 일반 예약과 별개다.
    const std::uint32_t& SharedRegularProcessType() const { return sharedRegularProcessType_; } // getter에서 BYTE로 줄이지 않고 전체 DWORD를 반환한다.
    std::uint32_t& ProcessTypeEnd() { return processTypeEnd_; } //0054116c: 원문 signed 비교를 위한 상한 비트를 보존한다.
    const std::uint32_t& ProcessTypeEnd() const { return processTypeEnd_; } // 원문 소비자가 필요할 때 signed로 해석하는 같은 물리다.
    std::uint32_t& EvilRayProcessType() { return evilRayProcessType_; } //00541098의 현재10이며 선택 생성 함수와 생성자 부착 번호를 구분한다.
    const std::uint32_t& EvilRayProcessType() const { return evilRayProcessType_; } // 실제 생성자의 같은 장기 DWORD를 읽는다.
    std::uint32_t& ParticleSource() { return particleSource_; } //00595128/FUN00487ee0의 현재 전체 DWORD이며 SID로 줄이지 않는다.
    const std::uint32_t& ParticleSource() const { return particleSource_; } // 생성자의 공통 등록 뒤 현재 원천을 복사 없이 읽는다.
    std::uint32_t& FlyerProcessType() { return flyerProcessType_; } //005410ec의 현재 비행 프로세스 TYPE를 생성/홈 조회가 같은 주소에서 읽는다.
    const std::uint32_t& FlyerProcessType() const { return flyerProcessType_; } // Session 교체나 일반 월드 준비로31의 새 사본을 만들지 않는다.
    std::uint32_t& AltarType() { return altarType_; } //005412e8의 현재 제단 TYPE를 보상/HP가 같은 주소에서 읽는다.
    const std::uint32_t& AltarType() const { return altarType_; } // 일반 월드 준비와 Session 교체로164를 다시 쓰지 않는다.
    std::uint32_t& DaisType() { return daisType_; } //005412ec의 현재 다이스 TYPE를 비용 개수/HP 예외가 공유한다.
    const std::uint32_t& DaisType() const { return daisType_; } // 최초165를 현재 owner의 값과 구별한다.
    std::uint32_t& PriestType() { return priestType_; } //005412d0/CD0051cbbc의 같은 현재 사제 TYPE를 HP/이동/회복이 공유한다.
    const std::uint32_t& PriestType() const { return priestType_; } // Session 교체/일반 월드 준비로158을 다시 쓰지 않는다.
    std::uint32_t& FakeSurfaceType() { return fakeSurfaceType_; } //005412e0의 현재 DWORD를 Create 입구 진단이 읽는다.
    const std::uint32_t& FakeSurfaceType() const { return fakeSurfaceType_; } // 일반 월드 준비와 Session 교체로162 사본을 새로 쓰지 않는다.
    std::uint32_t& RuneType() { return runeType_; } //005412f0의 현재 Rune 번호를 조회/생성/주변 정리가 같은 장기 주소에서 읽는다.
    const std::uint32_t& RuneType() const { return runeType_; } // 최초166 사본을 Session 교체나 일반 준비에서 새로 만들지 않는다.
    std::uint32_t& RainBlockerType() { return rainBlockerType_; } //005412a4의 현재 레인 바리케이드 TYPE를 보상/일반 삭제가 공유한다.
    const std::uint32_t& RainBlockerType() const { return rainBlockerType_; } // 성장형 비용 별칭도 같은 현재 DWORD를 읽는다.
    std::uint32_t& GrowingRainBlockerType() { return growingRainBlockerType_; } //005412b0의 현재 성장형 TYPE이며 비용 별칭/환불 제외를 선택한다.
    const std::uint32_t& GrowingRainBlockerType() const { return growingRainBlockerType_; } // 현재150 초기값의 사본을 Session마다 만들지 않는다.
    std::uint32_t& NuggetType() { return nuggetType_; } //005412c0의 현재 너겟 TYPE이며 보상 환불의 전액 단가 관문이다.
    const std::uint32_t& NuggetType() const { return nuggetType_; } // 같은 장기 owner를 후속 운반/채집 소비자도 빌릴 수 있다.
    std::uint32_t& VerifySid() { return verifySid_; } //0054d4d8의 BSS0이며 전체 객체 수신/검증 부모가 같은 DWORD를 쓴다.
    std::uint32_t& PreviousVerifySid() { return previousVerifySid_; } //0054d4dc의 별도 BSS0이며4ah/4bh 간격 순회의 현재 위치다.
    std::uint32_t& SupportTypeWas() { return supportTypeWas_; } //0054d4e4: 실제 전체 객체 수신이 쓰고 받침 진단이 읽는 같은 DWORD다.
    const std::uint32_t& SupportTypeWas() const { return supportTypeWas_; } // 일반 월드 준비/Session 교체로 현재 진단 문맥을 초기화하지 않는다.
    std::uint32_t& FirstTimerFactoryType() { return firstTimerFactoryType_; } //005411ac의 현재 DWORD를 Factory 최초 event2가 읽는다.
    const std::uint32_t& FirstTimerFactoryType() const { return firstTimerFactoryType_; } // Runtime을 교체해도 같은85 초기값의 주소를 유지한다.
    std::uint32_t& SecondTimerFactoryType() { return secondTimerFactoryType_; } //005411b4의 현재 DWORD를 Factory 최초 event1이 읽는다.
    const std::uint32_t& SecondTimerFactoryType() const { return secondTimerFactoryType_; } // 첫 예약 콜백 뒤에도 같은 현재87 저장소를 읽는다.
    std::uint32_t& OutpostType() { return outpostType_; } //00541250의 현재 제외 TYPE를 Factory 프레임·연구·업그레이드가 공유한다.
    const std::uint32_t& OutpostType() const { return outpostType_; } //126 초기값을 미션 생성/반납에서 새로 쓰지 않는다.
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
    RawSquidTransmissionState& SquidTransmission() { return squidTransmission_; } //005c7c50/00531850/0054d8e4등의현재전송물리다.
    const RawSquidTransmissionState& SquidTransmission() const { return squidTransmission_; } //상태조회는같은물리를읽으며세션교체로지우지않는다.
    std::array<float,2>& CentralPosition() { return centralPosition_; } //00568afc/00568b00의 실제 중앙 X/Y를 일반 중앙섬/후속 메타 부모가 공유한다.
    const std::array<float,2>& CentralPosition() const { return centralPosition_; } // Session 교체 때 값 사본으로 초기화하지 않는다.
    void ResetForGeneralWorld() { // 기존 지역 Runtime의 fresh 입력만 호환하며 Native cleanup 전체가 아니다.
        ordinaryGeysers_=0;totalCost_=0;resourcePool_=0;refundPercent_=25;allowAnyCapture_=0;archipelago_=0;lostX_=0.0F;lostY_=0.0F;geyserType_=122;priestQuarterHp_=false;
    }
    void ResetLostPositionForCleanup() { // Native 두 store만 Y→X로 실행하며 일반 fresh의+0과 구별한다.
        lostY_=std::bit_cast<float>(0xc61c4000U);lostX_=std::bit_cast<float>(0xc61c4000U);
    }
private:
    std::array<std::uint8_t,256> typeUse_{}; //005451f8의 최초 BSS0이며 명시 ClearTypes만 정확256BYTE를0으로 쓴다.
    std::uint8_t battleFinishFrames_{}; //0055a44c를같은Client수명으로보관하며World/Session교체로초기화하지않는다.
    std::array<std::uint8_t,2> packet0cHeader_{0x0c,1}; //보호10.78 PE0052ed54/55의초기헤더이며일반월드fresh에서도덮어쓰지않는다.
    std::uint16_t packet0cInitialLength_{}; //00544c24의최초BSS0이며버전조회/디버그burst의현재생성WORD다.
    std::uint32_t stateNotifyType_{82}; // 보호10.78 PE005411a0의 초기 DWORD이며 실제 상태 변경 부모가 현재 번호를 읽는다.
    std::array<float,2> centralPosition_{}; // 원문 최초 BSS0이며 중앙섬 배치가 X→Y 순서로 쓴다.
    RawSquidTransmissionState squidTransmission_; //최초한번의PE/BSS초기값이며일반월드fresh와별개로유지한다.
    bool priestQuarterHp_{}; // 원문 최초 BSS0이며 일반 fresh의 기존0만 호환한다. prepared와 반납은 현재값을 보존한다.
    std::uint32_t ordinaryGeysers_{},geyserFrameFlag_{},savedGeyserCount_{}; // 실제 세 BSS DWORD다.
    double geyserDeadline_{},cleanupDeadline_{},secondaryGeyserDeadline_{},scheduledDrawDeadline_{}; // 실제 서로 다른 네 BSS double이다.
    double spellDeadline_{}; //005950d0의 실제 별도 BSS double이며 Session 교체로 초기화하지 않는다.
    std::uint32_t incomePrimed_{}; // 비전투의 실제 SP 기한 가지가0을 쓰며 Session 생성/반납은 초기화하지 않는다.
    std::int32_t totalCost_{},resourcePool_{}; // 같은 signed 비용/자원 DWORD다.
    std::uint32_t refundPercent_{25}; // 보호 PE의 초기25다.
    std::int32_t allowAnyCapture_{}; // bool과 alias하지 않는 raw 허용 DWORD다.
    std::uint32_t archipelago_{}; // 원본bb0의 별도 모드다.
    std::uint32_t terrainEnabled_{1},archipelagoPercent_{}; // 보호 PE의 설정 후1과 강제 밀도BSS0을 Client 수명에 보관한다.
    std::int32_t archipelagoMinimum_{15},archipelagoMaximum_{35}; // 양판본 PE의 최초15/35이며 생성 때 복제하지 않는다.
    float lostX_{},lostY_{}; // 최초BSS+0이며 cleanup의-10000 입력과 구별한다.
    std::int32_t defaultSp_{2000}; // 보호 PE의 signed 기본SP다.
    std::uint32_t geyserType_{122},subscriberMarker_{}; // 보호 PE TYPE122와 별도BSS 표지다.
    std::uint32_t fieryTailProcessType_{11},streakProcessType_{12}; // 보호10.78의 두 TYPE 전역 초기값이며 Session보다 오래 유지한다.
    std::uint32_t regularProcessType_{46},sharedRegularProcessType_{61},processTypeEnd_{63}; // 일반/공유 예약과 프로세스 상한의 실제 초기 DWORD 세 개다.
    std::uint32_t processFormType_{2}; // 두 보호 PE의 최초 ProcessForm 번호이며 일반 World 준비에서도 유지한다.
    std::uint32_t dependFormType_{1}; // 두 판본의 최초 감시 Form 번호이며 ResetForGeneralWorld에서 덮어쓰지 않는다.
    std::uint32_t evilRayProcessType_{10},particleSource_{}; // 보호 PE00541098의10과00595128의BSS0이며 일반 fresh에서 덮어쓰지 않는다.
    std::uint32_t flyerProcessType_{31}; // 보호 PE005410ec의 초기 DWORD이며 홈/생성·후속 writer가 공유할 장기 물리다.
    std::uint32_t gunProcessType_{34}; // 보호 PE005410f8의 초기 DWORD다. World/Session 교체에서 새34를 쓰지 않는다.
    std::uint32_t damageRegularEvent_{6699},constructionRegularEvent_{6700}; //보호PE005326c0/c4의초기DWORD이며Session생성/정리가새값을쓰지않는다.
    std::uint32_t constructionProcessType_{30}; // 보호 PE005410e8의 최초30이며 World/Session 교체에서 초기화하지 않는다.
    std::uint32_t daisProcessType_{58}; // 보호 PE00541158의 최초3ah이며 Regular/Dais asset 번호와 공유하지 않는다.
    std::array<std::uint8_t,2> gunProcessHeader_{0x3a,1}; //보호PE005409b4/5의최초두BYTE이며같은현재owner를공유한다.
    std::array<std::uint8_t,2> flyerProcessHeader_{0x39,1}; //보호PE00532780/1의최초두BYTE이며같은현재owner를공유한다.
    std::array<std::uint8_t,2> sunAviaryHeader_{0x38,1}; //보호PE00532786/7의최초두BYTE이며같은현재owner를공유한다.
    std::array<std::uint8_t,2> windAviaryHeader_{0x74,1}; //보호PE0053278a/b의최초두BYTE이며같은현재owner를공유한다.
    std::array<std::uint8_t,2> rainAviaryHeader_{0x73,1}; //보호PE0053278e/f의최초두BYTE이며같은현재owner를공유한다.
    std::array<std::uint8_t,2> bombTimerHeader_{0x7d,1}; //보호PE0052ee9a/b의최초두BYTE이며같은현재owner를공유한다.
    std::uint32_t bombTimerInitialLength_{}; //보호PE00544c98은DWORD0이며writer가df590에서최소4로비교한다.
    RawPathDataHeader pathPacketHeader_; //보호PE0052edce/cf=3001·00544c50=DWORD0이며현재값을Session밖에유지한다.
    std::array<std::uint8_t,2> daisProcessHeader_{0x6a,1}; // 보호 PE00532690/91의 현재 writer 헤더 두BYTE다. 별도 길이 초기값은 없다.
    std::array<std::uint8_t,2> constructionHeader_{0x37,1}; // 보호 PE005325b8/b9의 최초 두BYTE다. 선언 길이는 별도 값으로 추가하지 않는다.
    std::int32_t pathTraceLevel_{99}; // 보호 PE00540c68의 초기99이며 실제 공통 로그의 현재 임계값과 별개다.
    std::uint32_t islandDropBlocked_{},islandStumpType_{96}; // 보호 PE0054db84의 BSS0과005411d8의 최초96이며 Session 대여로 초기화하지 않는다.
    std::array<std::uint32_t,7> missionSettings_{1,0,0,0,0,1,8}; //5326cc/55a488/55a48c/55a490/55a494/5326d0/5326d4의 실제 최초값이며 월드 교체에서 재설정하지 않는다.
    std::uint32_t buildReplayProcessType_{49}; // 보호 PE00541134의 최초49이며 World 대여/반납에서 지우지 않는다.
    std::array<std::uint8_t,2> constructionRequestHeader_{0x3e,1}; // 보호 PE의 현재 건설 요청 생성자 헤더이며 원문 wire는 version/type 순서다.
    std::uint16_t constructionRequestLength_{25}; // 원문 생성자 길이25WORD이며 높은 플래그를 추가하지 않는다.
    std::uint32_t quickBuild_{},quickVortex_{}; // 두 보호 PE BSS0을 새 Client에서만 만들고 Session 교체로 지우지 않는다.
    std::int32_t developerScatterRow_{3}; // 보호 PE005326d8의 최초3이며 scatter 뒤 signed (행+2)%10으로 갱신한다.
    std::uint32_t developerCombatResizeCommand_{0x02000065}; // 보호 PE0054288c의 최초 입력 코드이며 현재 원문 명령 주소와 대응한다.
    std::uint32_t fireAnimationProcessType_{35},rotateAnimationProcessType_{36}; // 보호 PE의 서로 다른 두 TYPE이며 World 교체에서 초기화하지 않는다.
    std::uint32_t sunArcherType_{71},sunCannonType_{128},windArcherType_{140}; // 수신 공장의 실제 세 비교 전역 초기값이며 기본 생성자 선택과 구별한다.
    std::array<std::uint8_t,2> rotateAnimationHeader_{0x3c,1},fireAnimationHeader_{0x3b,1}; // 보호 PE의 두 현재 헤더이며 World/Session 교체에서 초기화하지 않는다.
    std::uint32_t sunAviaryProcessType_{32},windAviaryProcessType_{59},rainAviaryProcessType_{60}; // 보호 PE의 서로 다른 세 DWORD다. 생성·검색·수신이 같은 장기 주소를 쓴다.
    std::uint32_t altarType_{164},daisType_{165}; // 보호 PE005412e8/005412ec의 두 초기 DWORD이며 일반 월드 준비에서 초기화하지 않는다.
    std::uint32_t priestType_{158}; // 두 보호 PE의 최초158이며 실제 HP/이동 소비자가 같은 장기 주소를 빌린다.
    std::uint32_t fakeSurfaceType_{162}; // PATCH005412e0의 최초162이며 실제 생성 부모가 장기 주소를 빌린다.
    std::uint32_t rainBlockerType_{147},growingRainBlockerType_{150}; //005412a4/005412b0의 현재 두 TYPE 번호이며 TYPE 기록 표와 별개다.
    std::uint32_t runeType_{166}; // 보호 PE005412f0의 초기 DWORD이며 ResetForGeneralWorld는 현재 번호를 유지한다.
    std::uint32_t nuggetType_{154}; //005412c0의 최초154를 한 번만 소유하고 Session 재대여로 덮어쓰지 않는다.
    std::uint32_t verifySid_{}; //0054d4d8의 최초 BSS0을 Session/World 교체에서 초기화하지 않는다.
    std::uint32_t previousVerifySid_{}; //0054d4dc의 최초 BSS0이며 현재 검증 SID의0 저장과 별도로 유지한다.
    std::uint32_t supportTypeWas_{}; //0054d4e4의 최초 BSS0이며 진단 소비자와 실제 수신 writer가 공유할 장기 물리다.
    std::uint32_t firstTimerFactoryType_{85},secondTimerFactoryType_{87},outpostType_{126}; // 보호10.78 PE의 세 TYPE DWORD 초기값이며 일반 Runtime 초기화와 구별한다.
    std::uint32_t terrainType_{97},edgeFarmType_{121}; // 보호 PE 두 TYPE 전역이며 현재 Runtime 대여/반납은 초기화하지 않는다.
    std::int32_t worldGeyserMargin_{2}; //보호 PE00542654의 실제 초기 DWORD다.
    std::array<std::uint32_t,4> worldDecorationTypes_{100,184,101,185}; //실제 treeTwo/RUIN/treeThree/Monument 타입 순서다.
};
static_assert(sizeof(std::uint32_t)==4 && sizeof(std::int32_t)==4 && sizeof(float)==4 && sizeof(double)==8); // 각 Native폭만 보증하며 클래스가17주소의 연속 image라는 주장은 아니다.
}
