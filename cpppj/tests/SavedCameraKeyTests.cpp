// 최초 x86 전체1750행과 실제 Client 소유자/선택된 장치의 부분 효과를 확인한다.
#include "RawSceneSupport.h"
#include "client/SavedCameraKey.h"
#include "client/ClientGameplayGlobals.h"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { //fixture의 float/부호를 바꾸지 않고 원시 바이트로 준비한다.
template<std::size_t Size> std::array<std::uint8_t,Size> Image(std::string_view text) { //지정한 실제 폭의 hex 이미지만 읽는다.
    std::array<std::uint8_t,Size> bytes{};CHECK(text.size()==Size*2);if (text.size()!=Size*2) return bytes;
    for (std::size_t index=0;index<Size;++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index*2,2)),nullptr,16)); //한 바이트씩 원문의 자리를 보존한다.
    return bytes;
}
SavedCameraPositions ReadSaved(std::string_view text) { //전체80BYTE 저장 Point의 NaN/무효 비트도 그대로 옮긴다.
    SavedCameraPositions saved;const auto bytes=Image<80>(text);
    for (std::size_t index=0;index<10;++index) saved.points[index]={std::bit_cast<float>(Get(bytes,index*8)),std::bit_cast<float>(Get(bytes,index*8+4))}; //양 축의 네 바이트를 각각 읽는다.
    return saved;
}
std::string SavedImage(const SavedCameraPositions& saved) { //변경하지 않은 나머지 칸도 함께 원문 결과와 비교한다.
    std::array<std::uint8_t,80> bytes{};
    for (std::size_t index=0;index<10;++index) { Put(bytes,index*8,std::bit_cast<std::uint32_t>(saved.points[index].x));Put(bytes,index*8+4,std::bit_cast<std::uint32_t>(saved.points[index].y)); } //단정도를 문자열 숫자로 반올림하지 않는다.
    return Hex(bytes);
}
RawCameraViewState ReadView(std::string_view text) { const auto bytes=Image<64>(text);RawCameraViewState view;std::memcpy(&view,bytes.data(),bytes.size());return view; } //검증된 trivially-copyable16DWORD만 복사한다.
std::array<std::int32_t,4> ReadViewport(std::string_view text) { //같은 현재 LTRB의 signed DWORD를 읽는다.
    const auto bytes=Image<16>(text);std::array<std::int32_t,4> viewport{};
    for (std::size_t index=0;index<4;++index) viewport[index]=std::bit_cast<std::int32_t>(Get(bytes,index*4)); //오버플로 입력도 unsigned 파싱 뒤 같은 비트로 옮긴다.
    return viewport;
}
}

//언어/수정키/0 포함 유효성/현재 Renderer 중심과 실패 앞의 전체 저장 장부를 최초 원문에 대조한다.
TEST_CASE(SavedCameraKey_IndependentNative1750Inputs) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/saved-camera-key-x86.tsv").string().c_str());std::size_t count{};
    for (const auto& row:fixture.rows) { //원문 초기화3행과 저장/호출1747행을 빠짐없이 방문한다.
        CHECK(row.size()==16);if (row.size()!=16) continue;auto saved=ReadSaved(row[3]);auto view=ReadView(row[4]);auto viewport=ReadViewport(row[5]);auto language=std::bit_cast<std::int32_t>(Number(row[2]));std::string trace;bool completed=true;
        SavedCameraKeyHooks hooks;
        hooks.paused=[&] { trace="P:"+row[6];if (Number(row[7])!=0xfffffffeU) language=std::bit_cast<std::int32_t>(Number(row[7]));return Number(row[6])!=0; };
        hooks.language=[&] { return language; };
        hooks.sound=[&] { trace+=";S:select.wav:0:0:0:0:0";if (Number(row[8])) { view.pixels={-87,-61};view.cellFloatBits={std::bit_cast<std::uint32_t>(8.0F),std::bit_cast<std::uint32_t>(5.5F)};viewport={82,19,1000,747};saved.points[0]={150.5F,151.5F}; }if (row[9]=="Sound") throw std::runtime_error("원문 선택음 명시 중단"); };
        hooks.capture=[&] { trace+=";V";if (row[9]=="Capture") throw std::runtime_error("원문 Renderer 중심 명시 중단");return CaptureSavedCameraCenter(view,viewport); };
        hooks.center=[&](float x,float y) { trace+=";C:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y));if (row[9]=="Center") throw std::runtime_error("원문 즉시 중심 명시 중단"); };
        try { if (row[0]=="Init") InitializeSavedCameraPositions(saved);else CHECK(DispatchSavedCameraKey({Number(row[1]),0,0},saved,hooks)); }catch (const std::runtime_error&) { completed=false; }
        if (trace.empty()) trace="-";const bool matches=SavedImage(saved)==row[10] && std::bit_cast<std::uint32_t>(language)==Number(row[11]) && trace==row[12] && completed==(Number(row[13])!=0);
        if (!matches) std::cerr<<"저장 카메라 원문 행 "<<count<<" event "<<row[1]<<" language "<<row[2]<<" trace "<<trace<<" expected "<<row[12]<<'\n';CHECK(matches);++count;
    }
    CHECK(count==1750 && count==fixture.rows.size());
}

//선택하지 않은 장치는 요구하지 않고 소리 뒤 캡처 실패의 선행 변경도 보존한다.
TEST_CASE(SavedCameraKey_LazyDevicesAndPartialWrites) {
    SavedCameraPositions saved;InitializeSavedCameraPositions(saved);const auto initial=SavedImage(saved);SavedCameraKeyHooks hooks;
    for (const auto code:std::array{InputCode::kRelease|std::uint32_t('1'),InputCode::kOutside|std::uint32_t('1'),InputCode::kCommand|std::uint32_t('1'),0x310000U,std::uint32_t('C')}) CHECK(!DispatchSavedCameraKey({code,0,0},saved,hooks)); //다른 종류의 사건에 정지/언어/장치를 요구하지 않는다.
    hooks.paused=[] { return true; };CHECK(DispatchSavedCameraKey({'1',0,0},saved,hooks) && SavedImage(saved)==initial);
    hooks.paused=[] { return false; };hooks.language=[] { return 0; };CHECK(DispatchSavedCameraKey({'1',0,0},saved,hooks)); //무효 저장 위치는 카메라가 없어도 자연 생략한다.
    bool threw=false;try { static_cast<void>(DispatchSavedCameraKey({InputCode::kShift|'!',0,0},saved,hooks)); }catch (const std::logic_error&) { threw=true; }CHECK(threw && SavedImage(saved)==initial);
    hooks.sound=[&] { saved.points[4]={12,34}; };threw=false;try { static_cast<void>(DispatchSavedCameraKey({InputCode::kShift|'!',0,0},saved,hooks)); }catch (const std::logic_error&) { threw=true; }CHECK(threw && saved.points[4].x==12 && saved.points[0].x==-10000.0F);
    saved.points[0]={0.0F,-0.0F};threw=false;try { static_cast<void>(DispatchSavedCameraKey({'1',0,0},saved,hooks)); }catch (const std::logic_error&) { threw=true; }CHECK(threw); //유효한0은 중심 장치를 실제 요구한다.
}

//저장10칸/최근5칸의 주소와 초기화가 독립이고 일반 대여/Renderer 재시작이 지우지 않는다.
TEST_CASE(SavedCameraKey_ClientOwnerAcrossRendererAndWorld) {
    ClientGameplayGlobals gameplay;auto& camera=gameplay.Camera();auto& saved=camera.SavedPositions();auto& recent=camera.ConstructionHistory();CHECK(saved.points[0].x==0.0F);recent.points[0]={70,80};recent.index=4;InitializeSavedCameraPositions(saved);CHECK(recent.points[0].x==70 && recent.index==4);saved.points[9]={123.25F,145.5F};const auto* owner=&saved;
    { auto use=gameplay.Acquire();CHECK(&gameplay.Camera().SavedPositions()==owner && saved.points[9].x==123.25F); }
    std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);CHECK(&camera.SavedPositions()==owner && saved.points[9].y==145.5F && recent.index==4);InitializeSavedCameraPositions(saved);CHECK(saved.points[9].x==-10000.0F && recent.points[0].y==80);
}
