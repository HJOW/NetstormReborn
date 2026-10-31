// 같은 실제 Client 음향이 World 통지 없이 Native snapshot 변경과 재초기화를 즉시 읽는지 검사한다.
#include "TestSupport.h"
#include "client/ClientAudio.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include <filesystem>
#include <string>

using namespace netstorm::client;
namespace { // 실제 Client 소유자만 조립하며 창·소리 장치는 초기화하지 않는다.
ClientOptions SoundViewClientOptions(netstorm::o::OriginalEdition edition) { // 보호 게임은 읽기 경로만 지정하며 기존 파일이나 설정을 저장하지 않는다.
    ClientOptions options;options.edition=edition;options.noAudio=true;
    options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(edition==netstorm::o::OriginalEdition::Patch1078 ? "originals" : "originalCD");return options;
}
}
TEST_CASE(ClientSoundView_ActualOwnerReadsSnapshotAndNativeResetWithoutWorld) { // 메뉴 복귀의 실제 Renderer 초기화가 위치 음향의 과거 World 사본에 가려지지 않아야 한다.
    Client client(SoundViewClientOptions(netstorm::o::OriginalEdition::Patch1078));client.InitializeNativeAudio();auto& audio=client.WorldAudio();auto& camera=client.GameplayGlobals()->Camera();
    CHECK(!client.Audio() && !audio.DeviceReady());audio.SetView({999,777,11,13,511,413}); // 원문 현재값과 의도적으로 다른 기존 어댑터 사본이다.
    camera.Snapshot().pixels={137,251};camera.RendererView().pixels={713,929}; // 원문497220의 snapshot과 현재 Renderer를 구별한다.
    auto view=audio.View();CHECK(view.cameraX==137 && view.cameraY==251 && view.left==0 && view.top==0 && view.right==kDefaultScreenWidth && view.bottom==kDefaultScreenHeight);
    CHECK(client.CurrentSoundView().cameraX==137 && audio.Describe().find("soundCameraX\t137\n")!=std::string::npos);
    camera.Snapshot().pixels={-29,43};CHECK(audio.View().cameraX==-29 && audio.View().cameraY==43); // 장치/World/프레임 통지가 없어도 같은 현재 필드를 읽는다.
    client.InitializeNativeRendererView();view=audio.View();CHECK(view.cameraX==0 && view.cameraY==0 && view.right==kDefaultScreenWidth && view.bottom==kDefaultScreenHeight); // State3가 사용하는 동일4993c0 본체다. 별도 SetView0을 보내지 않는다.
    CHECK(camera.RendererView().pixels[0]==0 && camera.Snapshot().pixels[0]==0 && !audio.DeviceReady());
}
TEST_CASE(ClientSoundView_CdRetainsExplicitAdapterOutsidePatchBinding) { // 10.72 현재 조회를 분석한 것처럼 새10.78 provider를 다른 판본에 강제하지 않는다.
    Client client(SoundViewClientOptions(netstorm::o::OriginalEdition::Cd1072));client.InitializeNativeAudio();auto& audio=client.WorldAudio();audio.SetView({17,29,3,5,803,605});
    client.GameplayGlobals()->Camera().Snapshot().pixels={137,251};const auto view=audio.View();CHECK(view.cameraX==17 && view.cameraY==29 && view.left==3 && view.top==5 && view.right==803 && view.bottom==605 && !audio.DeviceReady());
}
