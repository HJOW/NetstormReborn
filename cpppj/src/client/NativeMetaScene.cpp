// 디컴파일이 생략한 x87 asin·스택 문맥/정수 카운터를 전체 PE와 대조한 메타 장면 본체다.
#include "client/NativeMetaScene.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawRiftShapeAccess.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace netstorm::client { // 계산과 실제 장치 호출을 원문 순서로 수행하고 부분효과를 되돌리지 않는다.
namespace { // 비정렬 필드·32비트 감김·원문256BYTE 서식을 호스트 배치에서 분리한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("메타 장면의 실제 하위 장치 미연결");return function; } //선택되지않은하위는미리요구하지않는다.
// 실제표/객체에서 지정폭만 읽고 물리밖은 원문 포인터를 역참조하지 않고 거부한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 장면의 실제 읽기 필드 부족");std::uint32_t value{};
    // 낮은BYTE부터지정폭만조립한다.
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value;
}
// 전체DWORD만갱신하며옆표/이름/포인터는보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("메타 장면의 실제 쓰기 필드 부족");
    // x86의낮은BYTE부터네바이트를쓴다.
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte));
}
// ADD/SUB의signed overflow를호스트에서실행하지않고하위DWORD를해석한다.
std::int32_t Add(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)+std::bit_cast<std::uint32_t>(second)); }
std::int32_t Sub(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)-std::bit_cast<std::uint32_t>(second)); } //링정원선택뒤남은수의SUB32다.
// 원격존1..50의같은144BYTE를빌린다.현재상태판정은caller가한다.
std::span<std::uint8_t> Zone(std::span<std::uint8_t> bytes,std::uint32_t index) {
    const auto at=std::size_t(index)*o::kMetaZoneCleanupStride;if (at>bytes.size() || o::kMetaZoneCleanupStride>bytes.size()-at) throw std::out_of_range("메타 장면의 실제 존 범위");return bytes.subspan(at,o::kMetaZoneCleanupStride);
}
// 설명C문자열의첫NUL을현재레코드에서만찾고임의절단/빈설명대체를하지않는다.
std::string_view Description(std::span<const std::uint8_t> bytes) {
    const auto text=bytes.subspan(0x5c);std::size_t length{};
    // 실제남은존물리에서첫NUL까지만검색한다.
    while (length<text.size() && text[length]) ++length;
    if (length==text.size()) throw std::out_of_range("메타 존 설명의 실제 NUL 없음");return {reinterpret_cast<const char*>(text.data()),length};
}
struct Rings { //현재호출의지역링정원/반경/중심이며전역좌표캐시와구별한다.
    std::array<std::int32_t,2> count{}; //Native esp+68/6c의두정수다.
    std::array<double,2> radius{}; //Native esp+b0/b8의두double이다.
    std::array<float,2> origin{}; //Native esp+40/44의명시저장된float이다.
};
// CRT_CIasin(4e7790)의FPU인자/결과와PE의float값PI를확장한double상수를사용한다.
Rings Layout(const NativeMetaSceneGlobals& g,std::int32_t extent,std::int32_t remaining) {
    constexpr double pi=3.1415927410125732; //0050c528의double값이며수학PI로치환하지않는다.
    constexpr float inverse22=0.04545454680919647F; //0050d210의float곱에쓰는저장비트다.
    Rings result;auto& d=g.poll.display;const auto& geometry=d.challenge.Geometry();
    if (g.geometryMode) {
        result.origin={geometry.originX,geometry.originY};const auto flags=d.screenFlags&0x10U;
        // 현재두반경을순서대로읽으며정원계산뒤남은수를갱신한다.
        for (std::size_t ring=0;ring<2;++ring) {
            const auto radius=ring ? geometry.secondRadius:geometry.firstRadius;
            const auto capacity=CameraFtolLow(pi/std::asin(static_cast<double>(extent)/radius));const auto selected=capacity<remaining ? capacity:remaining;
            result.count[ring]=selected;remaining=Sub(remaining,selected);result.radius[ring]=flags ? static_cast<double>(radius)*60.0F*inverse22:static_cast<double>(radius);
        }
    } else {
        result.origin={static_cast<float>(static_cast<double>(Add(g.target.limits[0],d.view.width))*0.03125F),static_cast<float>(static_cast<double>(Add(g.target.limits[1],d.view.height))*inverse22)};
        const auto capacity=CameraFtolLow(pi/std::asin(static_cast<double>(extent)*(1.0/22.0)));const auto flags=d.screenFlags&0x10U;
        // 원문고정22반경분기는같은정원을재사용하고남은수만다시비교한다.
        for (std::size_t ring=0;ring<2;++ring) { const auto selected=capacity<remaining ? capacity:remaining;result.count[ring]=selected;remaining=Sub(remaining,selected);result.radius[ring]=flags ? 60.0:22.0; }
    }
    return result;
}
// 원문new실패/serial진단을정상노드로치환하지않으며기존노드만Push/Clear한다.
std::uint32_t Label(std::uint32_t serial,std::string_view expression,std::uint32_t line,const NativeMetaSceneHooks& h) {
    if (!serial) return Need(h.createText)();const auto token=Need(h.findSerial)(serial);
    if (!token) Need(h.display.assertion)(expression,".\\Metadisplay.cpp",line);Need(h.display.push)(token);Need(h.clearText)(token);return token;
}
// 원문서식결과256BYTE의물리범위를넘기는쓰기만명시적으로거부한다.
template<class... Args> std::string Format(std::string_view format,Args... arguments) {
    const std::string pattern(format);std::array<char,256> text;const auto count=std::snprintf(text.data(),text.size(),pattern.c_str(),arguments...);
    if (count<0 || static_cast<std::size_t>(count)>=text.size()) throw std::out_of_range("메타 존 글자 서식의 원문256BYTE 초과");return {text.data(),static_cast<std::size_t>(count)};
}
// 실제TextGump의내용맞춤→화면제한해제→객체부착→현재크기기준위치→depth19 순서다.
void AttachLabel(std::uint32_t token,std::uint32_t sid,bool centered,const NativeMetaSceneGlobals& g,const NativeMetaSceneHooks& h) {
    Need(h.fitText)(token,0,0);auto& label=Need(h.display.gump)(token);label.flags|=0x20000U;Need(h.reference)(token,sid);
    const float x=static_cast<float>(static_cast<double>(label.baseWidth)*-0.5);const float y=centered ? static_cast<float>(static_cast<double>(label.baseHeight)*-0.5):10.0F;
    Need(h.display.attach)(token,x,y,g.poll.display.attachParent,0);label.depth=19;
}
}
// 같은Mutable TYPE·SHP에서frameCheck/상세/진단뒤실제signed픽셀폭/높이를읽는다.
std::int32_t MetaSceneGlyphExtent(RawSurfaceStorage& storage,std::uint32_t type) {
    if (type>=storage.types.size() || type>=storage.shapes.size()) throw std::out_of_range("메타 존 TYPE/SHP의 실제 범위");const auto& selected=storage.types[type];const auto& shape=storage.shapes[type];o::RiftShapeAccessHooks hooks;
    hooks.typeNumber=[](auto current) { return current.token; };hooks.frameError=[&storage](auto current,auto frame,auto abstract) { storage.RecordDisplayFrameError(current,frame,abstract); };hooks.assertion=[&storage](auto expression,auto,auto line) { storage.AssertDisplay(expression,"c:\\ns\\o\\rifttype.h",line); };
    hooks.shape=[&storage](auto current,auto frame) { const auto& value=storage.shapes.at(current);if (!value.loaded || frame<0 || static_cast<std::size_t>(frame)>=value.frames.size()) throw std::out_of_range("메타 진단복귀뒤현재SHP물리부족");const auto& header=value.frames[static_cast<std::size_t>(frame)];return std::array<float,2>{static_cast<float>(header.width),static_cast<float>(header.height)}; };
    const auto pixels=o::ReadRiftShapeFrame({type,selected.flags1,shape.frameCount,shape.loaded,selected.maxHitPoints},0,0,hooks);return std::max(static_cast<std::int32_t>(pixels[0])/16,static_cast<std::int32_t>(pixels[1])/11);
}
// 원문전체선행갱신뒤표를다시읽으며링/라벨과마지막예약만명시위치에서변경한다.
void BuildMetaScene(NativeMetaSceneGlobals g,const NativeMetaSceneHooks& h) {
    auto d=g.poll.display;if (!d.playerKnown) return;Need(h.display.denominator)(MetaProjectionDenominator(d.challenge.Projection().camera));ClearMetaDisplayZones(g.poll.zones,h.display);
    // 실제참가자1..39의전체마스크갱신이며다음함수호출도현재known을자체판정한다.
    for (std::uint32_t participant=1;participant<o::kChallengeParticipantCount;++participant) UpdateMetaParticipant(participant,0xffU,d,h.display);
    // 실제전투1..9의초기특수BYTE10/투영을적용한다.
    for (std::uint32_t battle=1;battle<o::kChallengeBattleCount;++battle) UpdateMetaBattle(battle,d,h.display);
    std::int32_t count{};
    // 원문unroll과같은1..50의양수존을세고자기존한개를뺀다.0이하로임의보정하지않는다.
    for (std::uint32_t index=1;index<o::kMetaZoneCleanupCount;++index) if (std::bit_cast<std::int32_t>(Read(Zone(g.poll.zones,index),0))>0) ++count;
    const auto white=g.white;const auto green=g.green;const auto black=g.black;
    // Native esp+70/90의문맥이며두통계/설명은font6·흰색,존번호는font0·초록을사용한다.
    const std::array<std::uint32_t,8> textContext{2,6,white,0,0,black,1,1},numberContext{2,0,green,0,0,black,1,1};
    const auto extent=Need(h.glyphExtent)(g.glyphType);const auto rings=Layout(g,extent,count-1);std::size_t ring{};std::int32_t slot{};
    // 원문존owner는별도1..50카운터이며투영높이나링카운터와공유하지않는다.
    for (std::uint32_t index=1;index<o::kMetaZoneCleanupCount;++index) {
        const auto record=Zone(g.poll.zones,index);const auto zone=Read(record,0);if (std::bit_cast<std::int32_t>(zone)<=0 || zone==g.poll.zone) continue;
        if (ring>=2) { Need(h.display.assertion)("rIndex < CL_NUM_ZONE_RINGS",".\\Metadisplay.cpp",0x681);throw std::out_of_range("메타 존 두 링의 실제 지역배열 범위"); }
        const auto old=Read(record,0x4c);auto sid=old;
        if (!old) { sid=Need(h.create)(g.glyphType,2);Put(record,0x4c,sid);Put(record,0x50,0);Put(record,0x54,0);Put(record,0x58,0);const auto frame=Need(h.defaultFrame)(sid);const auto raw=Need(h.display.raw)(sid);Put(raw,0x24,Need(h.remapFrame)(frame)); }
        const auto raw=Need(h.display.raw)(sid);if (old) Need(h.display.unpop)(sid,0);Need(h.display.setOwner)(sid,index);
        const auto capacity=rings.count[ring];const auto flags=d.screenFlags&0x10U;
        constexpr double fullCircle=6.2831854820251465; //0050d1d8의실제double값이다.
        const double angle=(fullCircle/static_cast<double>(capacity))*slot;
        const float x=static_cast<float>(std::cos(angle)*rings.radius[ring]+rings.origin[0]);const float y=static_cast<float>(std::sin(angle)*rings.radius[ring]+rings.origin[1]);Put(record,0x88,std::bit_cast<std::uint32_t>(x));Put(record,0x8c,std::bit_cast<std::uint32_t>(y));
        const auto projected=ProjectMetaObject(raw,d.challenge.Projection(),d.view,d.screenFlags,x,y,flags ? 4.0F:0.0F,true);Need(h.display.move)(sid,projected.position[0],projected.position[1],0);
        if (!projected.clipped) {
            const auto statistics=Label(Read(record,0x50),"tg[0] != NULL",0x6bf,h);
            if (std::bit_cast<std::int32_t>(Read(record,4))>0) { const auto pattern=Need(h.display.translate)("Players:%d  Avg Level:%d");const auto text=Format(pattern,std::bit_cast<std::int32_t>(Read(record,4)),std::bit_cast<std::int32_t>(Read(record,8)));Need(h.appendText)(statistics,text,textContext,-1); }
            AttachLabel(statistics,sid,false,g,h);
            const auto description=Label(Read(record,0x58),"tg[2] != NULL",0x6dd,h);const auto pattern=Need(h.display.translate)("%s");const std::string name(Description(record));const auto text=Format(pattern,name.c_str());Need(h.appendText)(description,text,textContext,-1);AttachLabel(description,sid,false,g,h);
            const auto number=Label(Read(record,0x54),"tg[1] != NULL",0x713,h);const auto identifier=Format("%d",std::bit_cast<std::int32_t>(Read(record,0)));Need(h.appendText)(number,identifier,numberContext,-1);AttachLabel(number,sid,true,g,h);
        }
        ++slot;if (slot>=capacity) { ++ring;slot=0; }
    }
    if (g.pending) { Need(h.resetCamera)();g.pending=0; }
}
}
