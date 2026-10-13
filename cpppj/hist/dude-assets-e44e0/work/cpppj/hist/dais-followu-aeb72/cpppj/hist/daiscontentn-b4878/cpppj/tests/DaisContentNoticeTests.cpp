// 실제 다이스 내용물 알림의 typed 조회/plain 프레임·WORD/현재 cue를 독립 PE와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDaisContentNotice.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 반환형과 관계없이 미수행 Rune 생성/예약은 명시적으로 거부한다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> NoticeForbidden() {
    return [](Arguments...)->Result { throw std::logic_error("내용물 알림의 미선언 Rune 효과"); };
}
// 관찰 경계의 가상 공간/cue 요청을 원본 구분자로 묶는다.
std::string NoticeEffects(const std::vector<std::string>& effects) {
    std::string value;
    // 요청이 없는 원본의 '-'도 보존한다.
    for (const auto& effect:effects) { if (!value.empty()) value+=';';value+=effect; }
    return value.empty() ? "-" : value;
}
}
// 가상 공간과 장치는 명시 경계다. 전체 희생/Pop의 소비자 완료로 세지 않는다.
TEST_CASE(rawDaisContentNotice_native_attach_remove_plain_frame_word_and_current_cue) {
    const auto fixture=LoadFixture(NETSTORM_DAISCONTENTNOTICE_FIXTURE);std::size_t checked=0;
    // 각 입력의 풀/해시/프레임/전역을 새로 만들어 이전 결과를 사용하지 않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==4);std::map<std::string,std::string> input;
        // 평평한 JSON의 정수 입력만 읽고 결과를 계산하지 않는다.
        for (const auto& field:Split(std::string_view(row[1]).substr(1,row[1].size()-2),',')) { const auto split=field.find(':');input.emplace(field.substr(1,split-2),field.substr(split+1)); }
        const auto number=[&](const char* name) { return Number(input.at(name)); };const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;
        SidPool pool(edition,24000,true);SquidHash hash;std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::array<std::span<std::uint8_t>,12> slots;
        // 비교 슬롯 외의 raw도 원본 입력처럼 zero로 둔다.
        for (std::uint16_t sid=5;sid<=11;++sid) { CHECK(pool.Allocate(2)==Sid{sid});slots[sid]=pool.AllocatedBytes(Sid{sid});std::fill(slots[sid].begin(),slots[sid].end(),std::uint8_t{}); }
        // 실제 입력 체인의 Dais→사제→알터 삽입 순서를 그대로 준비한다.
        const auto insert=[&](Sid sid,std::uint32_t type) { auto raw=slots[sid.value];Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);Put(raw,14,std::bit_cast<std::uint32_t>(40.75F));Put(raw,18,std::bit_cast<std::uint32_t>(41.25F));auto& head=hash.Bucket(1,40.75F,41.25F);Put(raw,4,head,2);head=sid.value;raw[patch ? 33 : 31]=1;raw[patch ? 34 : 32]=1; };
        insert(Sid{5},165);insert(Sid{6},158);insert(Sid{7},164);Put(slots[5],0,patch ? kPatchDaisVtable : kCdDaisVtable);Put(slots[7],0,patch ? kPatchAltarVtable : kCdAltarVtable);Put(slots[5],patch ? 36 : 34,14,patch ? 4 : 1);Put(slots[7],patch ? 36 : 34,13,patch ? 4 : 1);Put(slots[7],12,2,2);slots[5][patch ? 34 : 32]=static_cast<std::uint8_t>(number("owner"));types[165].footX=types[165].footY=3;types[164].footX=types[164].footY=1;
        if (!number("exists")) hash.Bucket(1,40.75F,41.25F)=static_cast<std::uint16_t>(Get(slots[7],4,2));
        Put(slots[10],0,kVtable);slots[10][10]=6;Put(slots[10],12,number("word"),2);Put(slots[10],18,number("kind"),2);Put(slots[10],20,36,2);
        const auto pattern=std::string(number("first"),'A')+std::string(4,'O')+std::string(8,'P');std::vector<FrameCode> codes;
        // 현재 타입의 실제 첫표는 기존 RiftTypeFrames가 입력 코드에서 만들며 기대 프레임을 직접 계산하지 않는다.
        for (std::size_t index=0;index<pattern.size();++index) codes.push_back({static_cast<std::uint8_t>(pattern[index]),'P',static_cast<std::uint8_t>(index+1),0});
        frames[164]=RiftTypeFrames(std::move(codes));SquidPostPopState book;SquidPostPop base(pool,types,book);RunePostPopState runeState;RunePostPopHooks runeHooks;
        runeHooks.reserveRegular=NoticeForbidden<bool>();runeHooks.addRegular=NoticeForbidden<void,Sid,std::uint32_t,float>();runeHooks.startConstruction=NoticeForbidden<void,Sid,float>();RawRunePostPop rune(pool,hash,types,runeState,base,std::move(runeHooks));std::uint32_t priestType=158,localOwner=number("local");DaisContentNoticeState state{priestType,localOwner};state.cue=99;std::vector<std::string> effects;DaisContentNoticeHooks hooks;
        // 명시 Native 경계와 같은 인자를 기록하며 실제 공간 완료를 주장하지 않는다.
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==Sid{7} && flags==0);effects.push_back("unpop:7,0"); };
        hooks.repop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==Sid{7} && flags==0);effects.push_back("repop:7,0"); };
        hooks.stopVoice=[&](std::uint32_t cue,std::uint32_t zero,std::uint32_t flags) { CHECK(cue==99 && zero==0 && flags==0);effects.push_back("V:99,0,0"); };
        hooks.notifySurface=[](Sid) { throw std::logic_error("현재 알터 타입은 다리 WORD 표면 통지 대상이 아니다"); };
        RawDaisContentNotice notice(pool,frames,rune,state,std::move(hooks));notice.Notice(Sid{5},Sid{10},number("reason"));CHECK(NoticeEffects(effects)==row[2]);const auto images=Split(row[3],';');CHECK(images.size()==3);
        // 주입한 실제 VT 주소 DWORD 외의 raw 전체를 독립 원본과 비교한다.
        for (std::size_t index=0;index<3;++index) { const auto sid=std::array<std::uint16_t,3>{{5,7,10}}[index];const auto image=Hex(pool.Slot(Sid{sid}));CHECK(image.size()==images[index].size() && image.substr(8)==images[index].substr(8)); }
        ++checked;
    }
    CHECK(checked==1444);
}
