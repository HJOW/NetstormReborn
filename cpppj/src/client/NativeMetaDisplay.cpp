// 원문 CS 기록/객체/Gump 주소를 유지하고 가상 효과 뒤 필요한 현재 값만 재조회한다.
#include "client/NativeMetaDisplay.h"
#include "client/ClientNsInfoGlobals.h"
#include <bit>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client { // 전체 표시/CS 수신 부모에 연결하기 전에도 선택된 하위 미연결을 명시 거부한다.
namespace { // 비정렬 원문 필드와 이름표 위치 계산을 호스트 배치에서 분리한다.
// 도달한 하위만 요구하여 원문 조기 반환의 효과/호출 부재를 보존한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("메타 표시의 실제 하위 장치 미연결");return function; }
// 실제 CS/객체에서 지정 BYTE 폭만 읽으며 범위 밖 포인터를 호스트 예외로 거부한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 표시의 실제 필드 읽기 부족");std::uint32_t value{};
    // 원문 little endian 순서로 하위 BYTE부터 조립한다.
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);
    return value;
}
// 실제 원문 필드의 해당 폭만 기록하고 주위 DWORD/플래그는 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 표시의 실제 필드 쓰기 부족");
    // 지정한 폭까지만 순서대로 저장한다.
    for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte));
}
// x86 NEG의 하위 DWORD를 유지하여 INT_MIN의 C++ signed overflow를 피한다.
std::int32_t Neg(std::int32_t value) { return std::bit_cast<std::int32_t>(0U-std::bit_cast<std::uint32_t>(value)); }
// 원문 이름 C문자열은 현재 참가자 기록의 남은 물리에서만 NUL을 찾는다.
std::string_view Name(std::span<const std::uint8_t> record) {
    const auto tail=record.subspan(0xd8);std::size_t length{};
    // 첫 NUL까지만 읽으며 정상 이름의 공백/마크업을 변경하지 않는다.
    while (length<tail.size() && tail[length]) ++length;
    if (length==tail.size()) throw std::out_of_range("메타 이름표의 실제 이름 NUL 없음");return {reinterpret_cast<const char*>(tail.data()),length};
}
// append용 notice 문맥은004248a0(0,font,0,0,0,0)의 실제8DWORD다.
std::array<std::uint32_t,8> Notice(std::uint32_t font) { return {0,0,font,0,0,0,1,1}; }
// 기존 위치는 mask6이 꺼졌거나 알 수 없는 슬롯일 때 그대로 보존한다.
std::array<float,2> LabelPosition(std::span<const std::uint8_t> record,const GumpCreateNode& label,std::uint8_t mask) {
    std::array<float,2> result{label.x,label.y};if (!(mask&6U)) return result;
    const auto width=std::bit_cast<std::int32_t>(Read(record,0xc4));const auto height=std::bit_cast<std::int32_t>(Read(record,0xc8));
    const auto centerX=static_cast<float>(static_cast<double>(Neg(width/2))-static_cast<double>(label.baseWidth)*0.5);
    if (!Read(record,0xa4)) return {centerX,static_cast<float>(static_cast<double>(Neg(height/2))-static_cast<double>(label.baseHeight)*0.5)};
    const auto left=static_cast<float>(static_cast<double>(Neg(width))-label.baseWidth);const auto top=static_cast<float>(static_cast<double>(Neg(height))-label.baseHeight);
    switch (Read(record,0xa8)) {
    case 0:return {centerX,0.0F};case 1:return {centerX,top};case 2:return {left,static_cast<float>(Neg(height))};case 3:return {0.0F,static_cast<float>(Neg(height))};
    case 4:return {-label.baseWidth,top};case 5:return {static_cast<float>(Neg(width)),0.0F};case 6:return {-label.baseWidth,0.0F};case 7:return {static_cast<float>(Neg(width)),top};default:return result;
    }
}
}
// 실제 보이는 참가자/유효 SID/raw+0B bit0 관문을 통과한 뒤에만 가상 객체와 Gump를 갱신한다.
void UpdateMetaParticipant(std::uint32_t participant,std::uint8_t mask,NativeMetaDisplayGlobals g,const NativeMetaDisplayHooks& h) {
    if (!g.playerKnown) return;const auto record=g.challenge.Participant(participant);if (!(Read(record,0x64,1)&1U)) return;
    const auto sid=Read(record,0xb8);if (sid<1 || sid>=g.capacity) return;const auto raw=Need(h.raw)(sid);if (Read(raw,0xb,1)&1U) return;
    std::uint32_t clipped{};
    if ((mask&1U) && !(Read(raw,0x28,1)&8U)) {
        const bool placed=(Read(raw,0xb,1)&4U)==0;if (placed) Need(h.unpop)(sid,0);Need(h.setOwner)(sid,participant);
        if (!(Read(raw,0xb,1)&4U)) Need(h.assertion)("isVoid()","c:\\ns\\o\\squid.h",0x282);
        Put(raw,0x28,Read(raw,0x28,1)&0xfeU,1);Put(raw,0x1a,0);Put(raw,0x23,0,1);Need(h.paintOwner)(sid,participant);
        const auto point=ProjectMetaObject(raw,g.challenge.Projection(),g.view,g.screenFlags,std::bit_cast<float>(Read(record,0x9c)),std::bit_cast<float>(Read(record,0xa0)),0.0F,false);clipped=point.clipped;
        if (placed) Need(h.move)(sid,point.position[0],point.position[1],0);
    }
    const auto token=Read(record,0xd4);const bool own=Read(raw,0x22,1)==g.localOwner;auto& label=Need(h.gump)(token);
    if (Read(record,0xd0)!=label.serial) Need(h.assertion)("c->gumpHandle == tg->uniqueHandle",".\\Metadisplay.cpp",0x55d);
    Need(h.push)(token);const auto color=g.textColor;const auto ownFont=g.ownFont;const auto normalFont=g.normalFont;
    const std::array<std::uint32_t,8> ownContext{2,0,ownFont,0,color,0,1,1},normalContext{2,0,normalFont,0,color,0,1,1};
    if (mask&4U) {
        Need(h.replaceText)(token,"",0);const auto flags=Read(record,0x64);
        if (flags&0x1000U) Need(h.appendText)(token,"BETA:",Notice(g.noticeFont),-1,0);
        else if (flags&0x100U) Need(h.appendText)(token,"DEMO:",Notice(g.noticeFont),-1,0);
        label.textContext=own && g.metadataMode ? ownContext:normalContext;Need(h.parseName)(token,Name(record));
        if (Read(record,0x64,1)&4U) { const auto text=Need(h.translate)("BATTLEMASTER\n");Need(h.appendText)(token,text,Notice(g.noticeFont),-1,0); }
        Need(h.fit)(token,0,0);Need(h.scroll)(token,0);
    }
    const auto position=LabelPosition(record,label,mask);if (!clipped) label.flags|=1U;else label.flags&=~1U;
    Need(h.attach)(token,position[0],position[1],g.attachParent,1);
}
// 전투0을 별도 필터링하지 않고 실제 caller가 준 기록/상태 비트만 따른다.
void UpdateMetaBattle(std::uint32_t battle,NativeMetaDisplayGlobals g,const NativeMetaDisplayHooks& h) {
    const auto record=g.challenge.Battle(battle);if (!(Read(record,0,1)&1U)) return;const auto sid=Read(record,0x20);const auto raw=Need(h.raw)(sid);if (Read(raw,0x28,1)&8U) return;
    const bool placed=(Read(raw,0xb,1)&4U)==0;if (placed) Need(h.unpop)(sid,0);Need(h.setOwner)(sid,battle);
    Put(raw,8,0,2);Put(raw,0x1a,0);Put(raw,0x23,10,1);
    const auto point=ProjectMetaObject(raw,g.challenge.Projection(),g.view,g.screenFlags,std::bit_cast<float>(Read(record,0x10)),std::bit_cast<float>(Read(record,0x14)),0.0F,false);
    if (placed) Need(h.move)(sid,point.position[0],point.position[1],0);
}
// 기존00479520의 같은 물리/가상 삭제→SID0 본체를 재사용하며 상태 필드는 지우지 않는다.
void ClearMetaDisplayZones(std::span<std::uint8_t> records,const NativeMetaDisplayHooks& h) {
    // 실제0..50의51기록을 원문 포인터 순서대로 방문한다.
    for (std::uint32_t index=0;index<o::kMetaZoneCleanupCount;++index) o::ClearMetaZoneMarker(records,index,{h.destroy});
}
namespace { // 전체 Poll의 현재 존 물리와 종속 finder를 원문 단계에서만 읽는다.
// 카메라+0c는 float 비교가 아닌 원문 DWORD 비영 판정이며 음수0 비트도 유지한다.
bool Moving(const MetaProjectionState& projection,std::uint32_t flags) { return std::bit_cast<std::uint32_t>(projection.camera.reserved)!=0 || (flags&0x10U)!=0; }
// 실제0..50의 고정 기록 물리만 검사하며 상태/현재 존 관문은 caller가 판단한다.
std::span<std::uint8_t> Zone(std::span<std::uint8_t> records,std::uint32_t index) {
    const auto at=std::size_t(index)*o::kMetaZoneCleanupStride;
    if (at>records.size() || o::kMetaZoneCleanupStride>records.size()-at) throw std::out_of_range("메타 Poll의 실제 존 기록 부족");return records.subspan(at,o::kMetaZoneCleanupStride);
}
// 기본 true Form finder의 next 선저장/TYPE<70→현재 excluded/process 조건과 handle 조회를 보존한다.
void ZoneLabels(std::uint32_t parent,std::uint32_t clipped,const NativeMetaPollGlobals& g,const NativeMetaDisplayHooks& h) {
    const auto owner=Need(h.raw)(parent);
    if (g.checking && !g.metamap && (Read(owner,0xb,1)&2U) && !g.boss)
        Need(h.assertion)("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy","\\Ns\\O\\Squidfinder.cpp",0x158);
    auto current=Read(owner,6,2);std::unordered_set<std::uint32_t> visited;
    // 원문은 효과 전에 next를 저장한다. 손상된 순환만 호스트 예외로 거부한다.
    while (current) {
        if (!visited.insert(current).second) throw std::logic_error("메타 존 종속 finder의 순환");const auto raw=Need(h.raw)(current);const auto next=Read(raw,4,2);const auto type=Read(raw,10,1);
        if (type<70 && type!=g.excludedType && !(type>=10 && static_cast<std::int32_t>(type)<g.processTypeEnd)) {
            if (auto* label=Need(h.findGump)(Read(raw,0x12))) { if (clipped) label->flags&=~1U;else label->flags|=1U; }
        }
        current=next;
    }
}
}
// 현재 PlayerKnown을 한 번 확인한 뒤 각 가상 효과 다음의 카메라/표/포트가 아닌 표시 전역을 다시 읽는다.
void PollMetaDisplay(NativeMetaPollGlobals g,const NativeMetaDisplayHooks& h) {
    auto d=g.display;if (!d.playerKnown) return;CalibrateMetaProjection(d.challenge.Projection(),d.view,[&h](auto expression,auto line) { Need(h.assertion)(expression,".\\Metadisplay.cpp",line); });
    Need(h.denominator)(MetaProjectionDenominator(d.challenge.Projection().camera));if (!Moving(d.challenge.Projection(),d.screenFlags)) return;
    if (d.screenFlags&2U) Need(h.dirty)(d.view.viewport,0);
    // 실제 참가자1..39를 방문한다. 개별0047d8c0과 달리 SID/raw+0B bit0 관문과 특수BYTE0 쓰기를 추가하지 않는다.
    for (std::uint32_t participant=1;participant<o::kChallengeParticipantCount;++participant) {
        const auto record=d.challenge.Participant(participant);if (!(Read(record,0x64,1)&1U)) continue;const auto sid=Read(record,0xb8);const auto raw=Need(h.raw)(sid);std::uint32_t clipped{};
        if (!(Read(raw,0x28,1)&8U)) {
            const bool placed=(Read(raw,0xb,1)&4U)==0;if (placed) Need(h.unpop)(sid,0);Need(h.setOwner)(sid,participant);
            if (!(Read(raw,0xb,1)&4U)) Need(h.assertion)("isVoid()","c:\\ns\\o\\squid.h",0x282);
            Put(raw,0x28,Read(raw,0x28,1)&0xfeU,1);Put(raw,0x1a,0);Need(h.paintOwner)(sid,participant);
            const auto point=ProjectMetaObject(raw,d.challenge.Projection(),d.view,d.screenFlags,std::bit_cast<float>(Read(record,0x9c)),std::bit_cast<float>(Read(record,0xa0)),0.0F,false);clipped=point.clipped;
            if (placed) Need(h.move)(sid,point.position[0],point.position[1],0);
        }
        const auto token=Read(record,0xd4);auto& label=Need(h.gump)(token);if (Read(record,0xd0)!=label.serial) Need(h.assertion)("c->gumpHandle == tg->uniqueHandle",".\\Metadisplay.cpp",0x491);
        Need(h.push)(token);if (clipped) label.flags&=~1U;else label.flags|=1U;Need(h.attach)(token,label.x,label.y,d.attachParent,1);
    }
    // 전투1..9의 일반 투영은00479c40의 특수BYTE10 쓰기를 하지 않는다.
    for (std::uint32_t battle=1;battle<o::kChallengeBattleCount;++battle) {
        const auto record=d.challenge.Battle(battle);if (!(Read(record,0,1)&1U)) continue;const auto sid=Read(record,0x20);const auto raw=Need(h.raw)(sid);if (Read(raw,0x28,1)&8U) continue;
        const bool placed=(Read(raw,0xb,1)&4U)==0;if (placed) Need(h.unpop)(sid,0);Need(h.setOwner)(sid,battle);Put(raw,8,0,2);Put(raw,0x1a,0);
        const auto point=ProjectMetaObject(raw,d.challenge.Projection(),d.view,d.screenFlags,std::bit_cast<float>(Read(record,0x10)),std::bit_cast<float>(Read(record,0x14)),0.0F,false);
        if (placed) Need(h.move)(sid,point.position[0],point.position[1],0);
    }
    if (!Moving(d.challenge.Projection(),d.screenFlags)) return;
    // 현재 양수·자기 존 아님·marker 비영인1..50 존만 실제 객체/종속 이름표를 갱신한다.
    for (std::uint32_t index=1;index<o::kMetaZoneCleanupCount;++index) {
        const auto record=Zone(g.zones,index);const auto zone=Read(record,0);if (std::bit_cast<std::int32_t>(zone)<=0 || zone==g.zone) continue;const auto sid=Read(record,0x4c);if (!sid) continue;
        const auto raw=Need(h.raw)(sid);Need(h.setOwner)(sid,index);Need(h.unpop)(sid,0);
        const float height=(d.screenFlags&0x10U) ? 4.0F:0.0F;
        const auto point=ProjectMetaObject(raw,d.challenge.Projection(),d.view,d.screenFlags,std::bit_cast<float>(Read(record,0x88)),std::bit_cast<float>(Read(record,0x8c)),height,true);
        Need(h.move)(sid,point.position[0],point.position[1],0);ZoneLabels(sid,point.clipped,g,h);
    }
}
// Push의 가상 Redraw가 현재 NSInfo를 바꿀 수 있으므로 반환 뒤 같은 공용 슬롯/좌표를 재조회한다.
void RefreshMetaNsInfo(ClientNsInfoGlobals& globals,const NativeMetaDisplayHooks& h) {
    if (!globals.current) return;Need(h.push)(globals.current);const auto token=globals.current;auto& label=Need(h.gump)(token);label.flags|=1U;Need(h.attach)(token,label.x,label.y,0,0);
}
}
