// 10차 알려진 버그(NB01~NB34) 중 순수 함수/작은 경계로 확인할 수 있는 교정의 회귀 검사다. 게임/창/MP를 실행하지 않는다.
#include "TestSupport.h"
#include "client/NativeMetaParticipantName.h"
#include "client/PriestSelectionShortcut.h"
#include "o/RawCarrierPickup.h"
#include "client/StyleTextMarkup.h"

// NB02: 채팅 글자의 엠보스 오프셋 ~[E.x# y#]는 두 좌표가 모두 -4..4 안일 때만 적용한다.
// 원문은 한 좌표만 범위 안이면 다른 좌표를 임의로 키워 글자를 뒤로 밀 수 있었다.
TEST_CASE(KnownBug_NB02_EmbossOffsetRequiresBothCoordinatesInRange) {
    using netstorm::client::StyleTextOffsetAllowed;
    CHECK(StyleTextOffsetAllowed(0,0) && StyleTextOffsetAllowed(-4,4) && StyleTextOffsetAllowed(4,-4)); // 경계값은 허용한다.
    CHECK(!StyleTextOffsetAllowed(5,0) && !StyleTextOffsetAllowed(0,-5)); // 한 좌표가 범위를 벗어나면 거부한다.
    CHECK(!StyleTextOffsetAllowed(0,1000) && !StyleTextOffsetAllowed(1000,0)); // 원문 악용: 한 좌표만 정상이고 다른 좌표가 매우 큰 경우다.
    CHECK(!StyleTextOffsetAllowed(-1000,1000)); // 둘 다 범위 밖이면 당연히 거부한다.
}

// NB09: 최대 길이를 넘는 길드 태그는 이름표 버퍼를 넘어 종료하지 않고 태그를 줄여 서식에 맞춘다. 정상 길이는 그대로다.
TEST_CASE(KnownBug_NB09_LongGuildTagIsShortenedInsteadOfFailing) {
    using namespace netstorm::client;
    NativeMetaParticipantNameHooks hooks;hooks.translate=[](auto text) { return std::string(text); };hooks.weight=[](auto) { return std::uint8_t{3}; };
    std::vector<std::uint8_t> record(0xd8+80,0); // 이름표 목적 버퍼는 80바이트다.
    const char name[]="Bob";std::copy_n(name,sizeof name,record.begin()+0x26);
    WriteNativeMetaParticipantLabel(record,"~aABC:",hooks);
    const std::string normal(reinterpret_cast<const char*>(record.data()+0xd8));
    CHECK(normal.starts_with("~j~aABC:~wBob\n") && normal.size()<80); // 정상 길이는 그대로 쓴다.
    WriteNativeMetaParticipantLabel(record,"~a"+std::string(200,'T')+":",hooks); // 원문 악용/실수: 200자 길드 태그다.
    const std::string shortened(reinterpret_cast<const char*>(record.data()+0xd8));
    CHECK(shortened.size()<80 && shortened.starts_with("~j~aTTT") && shortened.find(":~wBob")!=std::string::npos); // 이름·서식은 보존하고 태그만 줄었다.
    bool failed{};try { WriteNativeMetaParticipantLabel(std::span<std::uint8_t>(record).first(0xd8+20),"",hooks); }catch (const std::out_of_range&) { failed=true; }
    CHECK(failed); // 길드 없이도 버퍼가 모자라면 원문처럼 명시 거부한다.
}

// NB20: 건설 준비 조각(건물/유닛)을 커서에 붙인 채 사제 선택 키(P/R)를 눌러도 조각이 남던 결함의 회귀 검사다.
// 조각을 들고 있으면 조회·선택에 앞서 취소(원문 00446e70)를 먼저 실행하고, 조각이 없으면 취소하지 않는다.
TEST_CASE(KnownBug_NB20_PriestKeyCancelsHeldBuildPiece) {
    using namespace netstorm::client;
    std::uint32_t selected{},repeated{};std::string trace;bool held=true; // trace: C=취소, F=참여 조회, S=선택, Q=View 큐다.
    PriestSelectionShortcutHooks hooks;hooks.localOwner=[] { return 1U; };hooks.viewCommand=[] { return 0x020000b8U; };
    hooks.find=[&](auto) { trace+="F";return 50U; };
    hooks.select=[&](auto sid,auto,auto) { trace+="S";selected=sid; };hooks.queue=[&](auto,auto,auto) { trace+="Q"; };
    hooks.pieceHeld=[&] { return held; };hooks.cancelPiece=[&] { trace+="C";held=false; };
    SelectPriestByKey(selected,repeated,hooks);
    CHECK(trace=="CFS" && !held && selected==50 && repeated==1); // 조각을 먼저 돌려놓고 사제를 선택한다.
    trace.clear();SelectPriestByKey(selected,repeated,hooks);
    CHECK(trace=="FQ" && repeated==1); // 조각이 없으면 취소하지 않고 같은 키 반복은 원문처럼 View 명령만 큐에 넣는다.
    held=true;trace.clear();SelectPriestByKey(selected,repeated,hooks);
    CHECK(trace=="CFQ" && !held); // 사제가 이미 선택된 뒤 다시 조각을 집었어도 키를 누르면 조각을 취소한다.
    PriestSelectionShortcutHooks original=hooks;original.pieceHeld={};original.cancelPiece={};held=true;trace.clear();selected=0;repeated=0;
    SelectPriestByKey(selected,repeated,original);
    CHECK(trace=="FS" && held); // 장치가 없으면 원문 10.78의 호출 순서 그대로다(독립 원문 fixture 보존).
}

// NB18: 너겟 같은 내용물을 이미 든 운반체(벌프)가 사제를 줍지 못한다. 요청(클라이언트)과 수신(서버)이 같은 조건으로 거부하며,
// 제품 스위치(refuseHeldPriest)가 꺼져 있으면 독립 원문 검사처럼 기존 동작을 유지한다.
TEST_CASE(KnownBug_NB18_CarrierHoldingContentRefusesPriestPickup) {
    using namespace netstorm::o;
    SidPool pool{OriginalEdition::Patch1078,24000,true};
    std::vector<RiftTypeRecord> types(188);ContainedFinderState contained;bool boss=true,single=false;CarrierPickupState state;
    auto raw=[&](Sid sid) { auto bytes=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(bytes.data()),bytes.size()); };
    for (const auto sid:{Sid{50},Sid{60},Sid{81}}) { auto bytes=raw(sid);std::fill(bytes.begin(),bytes.end(),std::uint8_t{0}); } // 부모(50)·사제 대상(60)·새 내용물(81) 슬롯이다.
    raw(Sid{50})[10]=6;raw(Sid{60})[10]=20;types[6].flags2=0x30000;types[20].flags2=0x200000; // 20번 TYPE을 사제(genus 0x200000)로 둔다.
    int sent=0,queued=0;bool holding=true;
    CarrierPickupHooks hooks;
    hooks.candidate=[](Sid,float,float) { return std::pair<Sid,bool>{Sid{60},true}; };
    hooks.send=[&](std::int32_t,std::span<const std::uint8_t>) { ++sent; };
    hooks.assertion=[](std::uint32_t) {};hooks.reveal=[](Sid sid) { return sid; };
    hooks.queue=[&](std::uint32_t,std::int32_t,std::int32_t) { ++queued; };
    hooks.find=[&](Sid,std::uint32_t,std::uint32_t,std::uint32_t) { return holding ? Sid{52} : Sid{0}; };
    hooks.afterDiscard=[] {};hooks.create=[](Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return Sid{81}; };
    hooks.destroy=[](Sid,std::uint32_t) {};hooks.detach=[](Sid,std::uint32_t) {};hooks.attach=[](Sid,Sid,std::uint32_t) {};
    RawCarrierPickup pickup(pool,types,contained,boss,single,state,hooks);
    std::array<std::uint8_t,9> packet{0x4f,1,9,0,50,0,60,0,20}; // 부모 50이 대상 60(TYPE 20)을 줍는 요청 패킷이다.
    pickup.Request(Sid{50});CHECK(sent==1); // 스위치가 꺼져 있으면 원문처럼 내용물을 들고도 요청한다.
    pickup.Receive(0,0,packet);CHECK(queued==1);
    state.refuseHeldPriest=true;sent=queued=0;
    pickup.Request(Sid{50});pickup.Receive(0,0,packet);CHECK(sent==0 && queued==0); // 내용물을 든 채 사제를 줍는 요청/수신을 모두 거부한다.
    holding=false;pickup.Request(Sid{50});pickup.Receive(0,0,packet);CHECK(sent==1 && queued==1); // 빈 손이면 정상 포획 요청은 그대로다.
}
