// 새 Native 실행 없이 실제004b6180 계약·같은 Client 두 표·기존 패킷 분배를 집중 검사한다.
#include "TestSupport.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "o/RawPacketDispatch.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace { // 실제 시계/현재 전역/상태 삭제의 호출만 관측하는 독립 장치다.
using netstorm::client::ClientConnectionActivityGlobals;
using namespace netstorm::o;

// 입력값을 원문 몸체에 공급하며 처리 순서나 표 출력을 대신 계산하지 않는다.
struct ActivityDevice {
    ClientConnectionActivityGlobals globals; // 두 표와 간극을 수신 몸체보다 오래 소유한다.
    double wall{100.75};std::uint32_t frame{7};bool server{}; // 서로 다른 실제 현재 시계/프레임/서버 입력이다.
    std::vector<std::string> trace; // 외부 효과의 실제 호출 순서를 기록한다.
    std::vector<std::array<std::uint32_t,2>> erased; // 실제 상태 삭제의 kind/player 인자만 보관한다.
    ConnectionActivityHooks Hooks() { // 실제 네 장치의 기본값이며 각 검사는 필요한 효과만 추가한다.
        ConnectionActivityHooks hooks;
        hooks.wallSeconds=[this] { trace.push_back("wall");return wall; };
        hooks.frameNumber=[this] { trace.push_back("frame");return frame; };
        hooks.server=[this] { trace.push_back("server");return server; };
        hooks.eraseStatus=[this](auto kind,auto player) { trace.push_back("erase");erased.push_back({kind,player}); };
        return hooks;
    }
};
}

// 원문 Wall의 부수 효과 뒤 현재 frame/server를 읽고 두 store의 사이에 server 조회를 둔다.
TEST_CASE(ConnectionActivity_WallFrameStoresAndCurrentServerFollowNativeOrder) {
    ActivityDevice device;device.globals.Frames().fill(0xaaaaaaaaU);device.globals.WallSeconds().fill(0xbbbbbbbbU);auto hooks=device.Hooks();
    hooks.wallSeconds=[&] { device.trace.push_back("wall");CHECK(device.globals.Frames()[9]==0xaaaaaaaaU && device.globals.WallSeconds()[9]==0xbbbbbbbbU);device.frame=0x12345678U;device.server=true;return device.wall; };
    hooks.server=[&] { device.trace.push_back("server");CHECK(device.globals.WallSeconds()[9]==100 && device.globals.Frames()[9]==0xaaaaaaaaU);device.frame=999;return device.server; };
    const auto erase=hooks.eraseStatus;hooks.eraseStatus=[&,erase](auto kind,auto player) { CHECK(device.globals.Frames()[9]==0x12345678U && device.globals.WallSeconds()[9]==100);erase(kind,player);device.globals.Frames()[9]=0x55555555U;device.globals.WallSeconds()[9]=0x66666666U; };
    RawConnectionActivity activity(device.globals.Tables(),std::move(hooks));activity.Receive(9);
    CHECK((device.trace==std::vector<std::string>{"wall","frame","server","erase"}));CHECK((device.erased==std::vector<std::array<std::uint32_t,2>>{{2,9}}));
    CHECK(device.globals.Frames()[9]==0x55555555U && device.globals.WallSeconds()[9]==0x66666666U); // 마지막 UI 효과가 바꾼 같은 표를 반환 후 사본으로 복구하지 않는다.
}

// 비서버 분기의 player는 sender와 무관하게0이며 frame0과 음수 벽시각의 하위DWORD를 보존한다.
TEST_CASE(ConnectionActivity_ClientEraseAndFrameZeroAreNotNormalized) {
    ActivityDevice device;device.wall=-1.5;device.frame=0;RawConnectionActivity activity(device.globals.Tables(),device.Hooks());activity.Receive(21);
    CHECK(device.globals.Frames()[21]==0 && device.globals.WallSeconds()[21]==0xffffffffU);CHECK((device.erased==std::vector<std::array<std::uint32_t,2>>{{1,0}}));
}

// 기존 실제_ftol 관측과 같은 signed64/하위DWORD 계약을 double 입력에서 검증하며 float으로 줄이지 않는다.
TEST_CASE(ConnectionActivity_Signed64WallConversionKeepsLowDword) {
    struct WallCase { double value;std::uint32_t expected; }; // 독립 입력과 signed64 절삭/하위DWORD의 명시 기대값이다.
    const std::array cases{WallCase{0.0,0},WallCase{-0.0,0},WallCase{0.75,0},WallCase{-0.75,0},WallCase{1.5,1},WallCase{-1.5,0xffffffffU},
        WallCase{2147483648.75,0x80000000U},WallCase{-2147483649.75,0x7fffffffU},WallCase{4294967303.75,7},WallCase{-4294967303.75,0xfffffff9U},
        WallCase{std::nextafter(0x1p63,0.0),0xfffffc00U},WallCase{std::nextafter(-0x1p63,0.0),0x400U},WallCase{0x1p63,0},WallCase{-0x1p63,0},
        WallCase{std::nextafter(-0x1p63,-std::numeric_limits<double>::infinity()),0},WallCase{1e100,0},WallCase{-1e100,0},
        WallCase{std::numeric_limits<double>::infinity(),0},WallCase{-std::numeric_limits<double>::infinity(),0},WallCase{std::numeric_limits<double>::quiet_NaN(),0},WallCase{std::bit_cast<double>(std::uint64_t{0x7ff0000000000001ULL}),0}};
    ActivityDevice device;device.frame=17;RawConnectionActivity activity(device.globals.Tables(),device.Hooks());
    for (const auto& sample:cases) { // 변환마다 같은 원문 몸체와 현재 표를 사용하며 예전 반환값을 재활용하지 않는다.
        device.wall=sample.value;activity.Receive(9);CHECK(device.globals.WallSeconds()[9]==sample.expected && device.globals.Frames()[9]==17);
    }
    CHECK(device.erased.size()==cases.size());
}

// 최초 BSS0·두 표의 경계·사이 Zone DWORD와 다음 몸체에 대여한 동일 주소를 함께 검사한다.
TEST_CASE(ConnectionActivity_IndependentTablesAndExplicitGapSurviveAcrossBodies) {
    static_assert(!std::is_copy_constructible_v<ClientConnectionActivityGlobals> && !std::is_move_constructible_v<ClientConnectionActivityGlobals>); // 실제 Client 주소를 복제/이동하지 않는다.
    ActivityDevice device;CHECK(device.globals.ActiveStamp()==0 && device.globals.ActiveZoneWord()==0);
    for (std::size_t index=0;index<kConnectionActivityEntries;++index) { // 실제22칸만 초기화/관측하며 인접 Native 필드를 표 항목으로 합치지 않는다.
        CHECK(device.globals.Frames()[index]==0 && device.globals.WallSeconds()[index]==0);device.globals.Frames()[index]=static_cast<std::uint32_t>(0x1000+index);device.globals.WallSeconds()[index]=static_cast<std::uint32_t>(0x2000+index);
    }
    device.globals.ActiveStamp()=0x12345678U;device.globals.ActiveZoneWord()=0x87654321U;const auto beforeFrames=device.globals.Frames();const auto beforeWall=device.globals.WallSeconds();const auto* frames=device.globals.Frames().data();const auto* wall=device.globals.WallSeconds().data();
    { RawConnectionActivity first(device.globals.Tables(),device.Hooks());first.Receive(21); }
    for (std::size_t index=0;index<kConnectionActivityEntries-1;++index) CHECK(device.globals.Frames()[index]==beforeFrames[index] && device.globals.WallSeconds()[index]==beforeWall[index]); // 마지막 정상 인덱스의 이웃88BYTE 밖을 건드리지 않는다.
    device.wall=111.75;device.frame=88;{ RawConnectionActivity second(device.globals.Tables(),device.Hooks());second.Receive(0); }
    CHECK(device.globals.Frames().data()==frames && device.globals.WallSeconds().data()==wall && device.globals.Frames()[0]==88 && device.globals.WallSeconds()[0]==111);
    CHECK(device.globals.Frames()[21]==7 && device.globals.WallSeconds()[21]==100 && device.globals.ActiveStamp()==0x12345678U && device.globals.ActiveZoneWord()==0x87654321U);
}

// 원문이 잘못된 주소를 쓰는 범위는 정상 실행으로 가장하지 않고 첫 Wall 효과 전에 host 계약 오류로 거부한다.
TEST_CASE(ConnectionActivity_InvalidSenderRejectsBeforeWallOrStores) {
    ActivityDevice device;device.globals.Frames().fill(0x11111111U);device.globals.WallSeconds().fill(0x22222222U);device.globals.ActiveStamp()=33;device.globals.ActiveZoneWord()=44;
    const auto frames=device.globals.Frames();const auto wall=device.globals.WallSeconds();RawConnectionActivity activity(device.globals.Tables(),device.Hooks());
    for (const auto sender:{-1,-22,22,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}) { // 정상22칸 앞/뒤와 signed 양끝을 별도로 확인한다.
        bool rejected{};try { activity.Receive(sender); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && device.trace.empty() && device.erased.empty());
        CHECK(device.globals.Frames()==frames && device.globals.WallSeconds()==wall && device.globals.ActiveStamp()==33 && device.globals.ActiveZoneWord()==44);
    }
}

// 기존 dispatcher의 길이/버전/기록/본문 순서를 사용하며 별도 동기 receive 우회나 중복 record를 만들지 않는다.
TEST_CASE(ConnectionActivity_RegistrationUsesExistingDispatcherAndPreservesLength) {
    ActivityDevice device;device.server=true;RawConnectionActivity activity(device.globals.Tables(),device.Hooks());PacketDispatchHooks hooks;
    hooks.now=[&] { device.trace.push_back("now");return 77U; };hooks.record=[&](PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t length) { CHECK(connection.sender==9 && packet[1]==0x64 && length==5);device.trace.push_back("record"); };
    hooks.terminate=hooks.close=hooks.release=[](PacketConnection&) { throw std::logic_error("활동 등록 검사의 미선택 연결 종료"); };
    RawPacketDispatch dispatch(OriginalEdition::Patch1078,std::move(hooks));RegisterConnectionActivity(dispatch,activity);const auto& registration=dispatch.Registration(0x64);
    CHECK(registration.version==1 && registration.mismatch==0 && registration.length==5 && bool(registration.receive) && bool(registration.observer));registration.observer(1);CHECK(device.trace.empty());
    PacketConnection connection;connection.sender=9;const std::array<std::uint8_t,5> packet{1,0x64,5,0,0xa5};CHECK(dispatch.Dispatch(connection,packet));
    CHECK((device.trace==std::vector<std::string>{"now","record","wall","frame","server","erase"}));CHECK(connection.received==77 && connection.depth==0 && device.globals.Frames()[9]==7 && device.globals.WallSeconds()[9]==100);
    device.trace.clear();const std::array<std::uint8_t,5> badLength{1,0x64,4,0,0};CHECK(dispatch.Dispatch(connection,badLength) && device.trace.empty() && device.erased.size()==1); // 선언 길이가 다르면 기존 분배기가 본문 앞에서 자연 생략한다.
    const std::array<std::uint8_t,5> toleratedVersion{0x7f,0x64,5,0,0};CHECK(dispatch.Dispatch(connection,toleratedVersion) && device.erased.size()==2); // 원문 정책0은 다른 version도 분배한다.
    dispatch.RequireLength(0x64,7);RegisterConnectionActivity(dispatch,activity);CHECK(dispatch.Registration(0x64).length==7); // 실제 등록은 현재 length 표를 암묵 초기화하지 않는다.
}
