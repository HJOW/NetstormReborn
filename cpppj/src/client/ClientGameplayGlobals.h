#include "client/ClientCameraGlobals.h"
// Client 수명의 현재 Player/커서/검색/미션 전역과 짧은 실제 Runtime 대여를 구별한다.
#pragma once
#include "client/ClientCursorGlobals.h"
#include "client/RawCursorDrop.h"
#include "client/ClientUserSelectionGlobals.h"
#include "o/RawAuthorityState.h"
#include "o/RawGameAuxState.h"
#include "o/RawCampaignWorldLoad.h"
#include "o/RawContainedFinder.h"
#include "o/RawEnergySupply.h"
#include <memory>

namespace netstorm::o { struct TerrainBitmapState; } // 미리보기의 최초 문자 분류는 Session 밖의 Client 수명으로 유지한다.
namespace netstorm::client {
class ClientSubscriberGlobals;class RawCampaignSession; // 실제 전역 소유자와 현재 Runtime을 복제하지 않는다.
class ClientGameplayGlobals {
private:
    std::shared_ptr<o::RawAuthorityState> authority_; // 물리 저장소와 공유하며 모든 bool 참조보다 먼저 구성하고 나중에 반납한다.
    std::shared_ptr<o::RawGameAuxState> gameAux_; // 모든 게임 보조 참조보다 먼저 구성하고 마지막에 반납한다.
    std::shared_ptr<ClientSubscriberGlobals> subscribers_; // world의 두 DWORD 참조보다 오래 유지한다.
public:
    explicit ClientGameplayGlobals(std::shared_ptr<ClientSubscriberGlobals> subscribers={},std::shared_ptr<o::RawAuthorityState> authority={},std::shared_ptr<o::RawGameAuxState> gameAux={}); // 최초 전역값을 한 번 만들며 독립 Session만 자체 subscriber 값을 소유한다.
    ClientGameplayGlobals(const ClientGameplayGlobals&)=delete;ClientGameplayGlobals& operator=(const ClientGameplayGlobals&)=delete; // 살아 있는 전역 주소를 복제하지 않는다.
    class Use {
    public:
        ~Use(); // 일반 대여는 기존 무효화를 유지하고 준비 상태 대여는 Runtime 연결만 반납한다.
        Use(const Use&)=delete;Use& operator=(const Use&)=delete; // 단일 Runtime 대여를 복제하지 않는다.
        void Publish(RawCampaignSession& session); // 완성한 실제 Session만 현재 실행 대상으로 연결한다.
        void Withdraw(RawCampaignSession& session); // 모듈 소멸이 시작되기 전에 실행 대상만 해제한다.
    private:
        friend class ClientGameplayGlobals;
        friend class RawCampaignSession; // 실제 Session 소멸자만 자식 정리 증빙을 확인한 뒤 보존 반납을 선택한다.
        void WithdrawForState(RawCampaignSession& session); // 정상 반납된 모든 자식의 같은 Session만 현재 공개를 닫고 전역 쓰기를 생략한다.
        explicit Use(ClientGameplayGlobals& globals,bool prepared=false); // 중복 대여를 먼저 거부하며 명시 준비 대여는 게임 값을 초기화하지 않는다.
        ClientGameplayGlobals& globals_; // Client의 실제 고정 주소다.
        bool prepared_; // Native 준비 대여 또는 실제 자식 증빙을 모두 소비한 종료에서 전역을 재초기화하지 않는 C++ 수명 정책이다.
    };
    std::unique_ptr<Use> Acquire(); // 기존 독립 Session 생성 의미를 같은 전역 주소에서 수행한다. NativeState 전체 순서와 구별한다.
    std::unique_ptr<Use> AcquirePrepared(); // 명시 Native 정리/prefix의 현재 값을 그대로 빌린다. 준비 완료 판정은 조립 호출자의 책임이다.
    RawCampaignSession& RequireSession() const; // 선택된 게임 효과가 실제 Runtime을 요구할 때만 호출한다.
    bool Borrowed() const; // 현재 대여 여부의 읽기 전용 관측이다.
    const std::shared_ptr<ClientSubscriberGlobals>& Subscribers() const; // 다른 Client의 참조를 잘못 합치지 않는다.
    o::TerrainBitmapState& TerrainBitmaps(); // 같은005c8ab0/005c8bb8 캐시와 fringe TYPE를 조회하며 Session 대여/반납에서 초기화하지 않는다.
    std::uint32_t& FullDeck() { return fullDeck_; } // 0054db14의 실제 DWORD를 생산/건설/배치/객체/HUD가 같은 주소로 빌린다.
    const std::uint32_t& FullDeck() const { return fullDeck_; } // 자연 writer의0/1 외에도 현재 비영 DWORD를 정규화 없이 보존한다.
    std::uint32_t& FortKeepGenus() { return fortKeepGenus_; } //00542640의 같은 현재 마스크를 Editor prefix/객체 읽기/해방이 공유한다.
    std::uint32_t& StateAuxFC4() { return stateAuxFC4_; } //00594fc4의 별도 현재 DWORD이며 battle/fort bool과 합치지 않는다.
    std::uint32_t& MapRevision() { return mapRevision_; } // 005949d4의 같은 전체 미니맵 갱신 DWORD를 지형/소유자/HUD/표시 소비자가 공유한다.
    const std::uint32_t& MapRevision() const { return mapRevision_; } // 단조 버전이 아니며 실제 Refresh가0으로 소비한 현재값도 즉시 읽는다.
    std::uint32_t& HideBuildings() { return hideBuildings_; } //0059a8ac의 실제 DWORD를 메뉴·Draw·State 정리가 직접 공유한다.
    const std::uint32_t& HideBuildings() const { return hideBuildings_; } // Uber 교체와 Session 반납으로 값이나 주소를 재생성하지 않는다.
    std::int32_t& DenySalvage() { return denySalvage_; } // 00595078의 signed DWORD를 Start·회수·Tutorial 소멸이 같은 Client 주소에 읽고 쓴다. 근거: docs/exe/cpp-guide-release-20261101.md.
    const std::int32_t& DenySalvage() const { return denySalvage_; } // World가 없어도 마지막 전체 DWORD를 유지하며 bool 사본으로 축소하지 않는다.
    std::int32_t& DenyAscend() { return denyAscend_; } // 0059507c의 signed DWORD를 Start·메뉴·Guide 해제가 같은 장기 저장소로 공유한다.
    const std::int32_t& DenyAscend() const { return denyAscend_; } // Session 대여나 반납 뒤에도 같은 주소와 현재 금지 값을 읽는다.
    std::uint32_t& PatchDiagnostic() { return patchDiagnostic_; } // 005318ec의 같은 DWORD를 SP 추적 쓰기와 경로·그림자·카메라 읽기가 빌린다.
    const std::uint32_t& PatchDiagnostic() const { return patchDiagnostic_; } // 진단값을 프레임별 사본이나 bool로 축소하지 않는다.
    std::int32_t& WorldCells() { return worldCells_; } //00531928의 같은 signed 세계 크기를 카메라와 마나/공급/AI가 빌린다.
    const std::int32_t& WorldCells() const { return worldCells_; } // Session 반납으로 주소나 현재값을 초기화하지 않는다.
    const std::shared_ptr<o::RawGameAuxState>& GameAux() const { return gameAux_; } // Storage와 같은17필드 owner 정체성을 빌린다.
    std::uint32_t& GeyserType() { return gameAux_->GeyserType(); } // Host/옵션/게임이 같은 TYPE DWORD를 직접 사용한다.
    const std::uint32_t& GeyserType() const { return gameAux_->GeyserType(); } // 현재 타입을 복사하지 않는다.
    const std::shared_ptr<o::RawAuthorityState>& Authority() const { return authority_; } // 같은 두 bool의 장기 소유자 정체성을 직접 제공한다.
    std::uint32_t& LocalAddress() { return localAddress_; } //005953fc의 현재 주소DWORD를 PlayerFinder/객체메뉴가 같은 Client 수명에 빌린다.
    const std::uint32_t& LocalAddress() const { return localAddress_; } //미션/메뉴 교체 뒤에도 같은 현재 주소를 읽는다.
    bool& Server() { return authority_->server; } // 실제 서버 관문/Zone 출력이 같은 현재 bool을 읽고 쓴다.
    const bool& Server() const { return authority_->server; } // 값 사본이나 Session의 fallback을 읽지 않는다.
    bool& Boss() { return authority_->boss; } // 실제 권한 관문/prefix 출력이 같은 현재 bool을 읽고 쓴다.
    const bool& Boss() const { return authority_->boss; } // 권한의 현재 논리값을 복제하지 않고 조회한다.
    ClientCameraGlobals& Camera() { return camera_; } // 같은0059a900 snapshot과 논리 Renderer의 장기 소유자를 빌린다.
    const ClientCameraGlobals& Camera() const { return camera_; } // Session 재대여가 카메라를 재생성하지 않는다.
    ClientCursorGlobals& CursorGlobals() { return cursorGlobals_; } // 같은현재커서데이터를첫메뉴/Session이직접빌린다.
    const ClientCursorGlobals& CursorGlobals() const { return cursorGlobals_; } // 실행기소멸뒤에도dataowner주소는유지한다.
    o::PlayerWorldState player; //595344/540c70/540cac 등 실제 현재 Player 조회 전역이다.
    CursorDropState cursor{.lockDepth=1}; //5325c8 초기 잠금1과 실제 현재 조각/마우스/플래그다.
    ClientUserSelectionGlobals selection; // 실제 선택/강조9DWORD와 현재 다중 배열이며 Runtime/장치 callback을 보관하지 않는다.
    o::ContainedFinderState finder; // 실제 현재 권한/TYPE6/진단을 모든 생산 소비자가 공유한다.
    o::CampaignWorldGlobals world; // 실제 두 Subscriber와 editor/missionPost를 같은 주소에서 보관한다.
    o::EnergySupplyState energy; // 공급 모듈과 메뉴가 같은5cab30을 빌린다. userCheat은 Runtime 교체로 지우지 않는다.
    std::uint32_t specialAuxiliary{}; // 원문55a484이며 설정 해석에서55a480과 함께0으로 쓴다. 별도 기능 해석은 후속이다.
private:
    std::uint32_t localAddress_{}; //보호PE005953fc의 최초BSS0이다. 이후 실제 네트워크 주소 writer는 후순위MP에서 연결한다.
    std::uint32_t fullDeck_{}; // 최초 PE BSS0이며 준비 대여/반납은 Native 부모가 쓴 현재 값을 보존한다.
    std::uint32_t fortKeepGenus_{0x40480002U},stateAuxFC4_{}; //보호 PE00542640의 초기 마스크와00594fc4의 별도 BSS0이다.
    std::uint32_t hideBuildings_{}; // 원문 최초 BSS0이며 명시 State store 외에 임의 초기화를 추가하지 않는다.
    std::int32_t denySalvage_{},denyAscend_{}; // 00595078/0059507c의 최초 BSS0이다. 대여·반납에서는 다시 쓰지 않고 Start와 Tutorial 소멸의 명시 쓰기만 공유한다.
    std::uint32_t patchDiagnostic_{6}; // 보호 10.78 PE의 초기 DWORD6이며 World/Session 대여·반납으로 다시 쓰지 않는다.
    std::int32_t worldCells_{256}; // 보호 PE 초기256이며 실제 startup의 명시 store와 Session 대여를 구별한다.
    ClientCameraGlobals camera_; // 전역snapshot과논리Renderer/목표를분리하고명시Native초기화외에는현재값을보존한다.
    ClientCursorGlobals cursorGlobals_; // callback/타이머없이Lifecycle/Placement/snap만장기소유한다.
    std::uint32_t mapRevision_{}; // 최초 BSS0이며 Session 대여/반납으로 초기화하지 않는 실제005949d4다.
    std::shared_ptr<o::TerrainBitmapState> terrainBitmaps_; // 최초 분류를 모든 편집 창/Session보다 오래 같은 주소에 소유한다.
    RawCampaignSession* current_{};bool borrowed_{}; // 게임 값의 대체가 아닌 현재 C++ Runtime 대여다.
};
}
