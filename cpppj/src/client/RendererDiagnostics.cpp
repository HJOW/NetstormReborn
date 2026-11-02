// 원문 진단의 사각형/색/현재배열 재조회와 모듈러 DWORD 산술을 보존한다.
#include "client/RendererDiagnostics.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client { // 각 진단은 일반 장면 목록/dirty와 분리된 후단이다.
namespace { // 원문 정수/물리 접근의 호스트 명시 경계다.
template<class T> const T& Need(const T& hook) { if (!hook) throw std::logic_error("Renderer 진단 실제 경계 미연결");return hook; } //선택한효과에만실제장치를요구한다.
std::int32_t Add(std::int32_t value,std::int32_t delta) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+std::bit_cast<std::uint32_t>(delta)); } //signedoverflow없이원문하위DWORD다.
std::int32_t Sub(std::int32_t value,std::int32_t delta) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)-std::bit_cast<std::uint32_t>(delta)); } //현재snapshot반크기아닌전체칸크기를뺀다.
float Coordinate(std::span<const std::uint8_t> slot,std::size_t at) { std::uint32_t bits{};for (std::size_t i=0;i<4;++i) bits|=std::uint32_t(slot[at+i])<<(8*i);return std::bit_cast<float>(bits); } //pool+e/12의비정렬F32읽기다.
std::uint32_t Color(const RendererDiagnosticHooks& hooks,std::size_t index) { const auto colors=Need(hooks.colors)();if (index>=colors.size()) throw std::out_of_range("Renderer 진단 현재색표 물리범위");return colors[index]; } //색표를미리복사하거나명명색을고정하지않는다.
std::size_t Cell(std::int32_t x,std::int32_t y,std::size_t size) { const auto offset=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(y)*256U+static_cast<std::uint32_t>(x));if (offset<0 || static_cast<std::size_t>(offset)>=size) throw std::out_of_range("Renderer 진단 원문배열 물리범위");return static_cast<std::size_t>(offset); } //원문행alias는유지하되호스트밖메모리는거부한다.
void Inward(ScreenRect rect,std::uint32_t count,const std::function<void(ScreenRect,std::uint32_t)>& rectangle,const std::function<std::uint32_t()>& color) {
    for (std::uint32_t i=0;i<count;++i) { rect={Add(rect.left,1),Add(rect.top,1),Add(rect.right,-1),Add(rect.bottom,-1)};Need(rectangle)(rect,color()); } //매사각형직전현재색을다시읽고세겹을안쪽으로좁힌다.
}
}
// 실제 표를 빌리기만 하며 생성시 탐색/검색/픽셀을 변경하지 않는다.
RawRendererDiagnostics::RawRendererDiagnostics(const o::SidPool& pool,const o::SquidHash& hash,std::span<const o::RiftTypeRecord> types,std::span<const std::uint8_t> spots):pool_(pool),hash_(hash),types_(types),spots_(spots) {}
// 명시DebugSwitch가 기록한 현재 플래그를 원문 네 후단 몸체가 읽는다.
void RawRendererDiagnostics::Draw(RendererDiagnosticKind kind,const RawCameraViewState& renderer,const RawCameraViewState& snapshot,std::int32_t width,std::int32_t height,const std::uint32_t& flag,const std::uint32_t& fireMode,const std::int32_t& worldCells,std::uint32_t& background,const RendererDiagnosticHooks& hooks) const {
    if (!flag) return;const auto visible=DrawWaterVisibleArea(renderer,width,height);
    if (kind==RendererDiagnosticKind::Hash) {
        for (const auto scale:o::SquidHash::kScales) { //원문현재4단계1/2/4/16의행/열순서를유지한다.
            const auto clamp=[&](std::int32_t value) { return std::min(value/scale,Add(worldCells/scale,-1)); }; //34380은아래한계0을추가하지않는다.
            const auto left=clamp(visible.left),top=clamp(visible.top),right=clamp(visible.right),bottom=clamp(visible.bottom);
            for (auto y=top;y<=bottom;y=Add(y,1)) { //원문현재행의양쪽꼭짓점을매열투영한다.
                for (auto x=left;x<=right;x=Add(x,1)) { //음수칸과축별현재snapshot을그대로쓴다.
                    const auto product=[&](std::int32_t value) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)*static_cast<std::uint32_t>(scale)); }; //원문imul의하위값을signed32로읽은뒤F32저장한다.
                    const auto first=ProjectCameraView(snapshot,static_cast<float>(product(x)),static_cast<float>(product(y)));
                    const auto second=ProjectCameraView(snapshot,static_cast<float>(product(Add(x,1))),static_cast<float>(product(Add(y,1))));
                    const ScreenRect rect{Add(first[0],1),Add(first[1],1),Add(second[0],-1),Add(second[1],-1)};Need(hooks.screen.clip)(rect);Need(hooks.screen.rectangle)(rect,Color(hooks,0));
                }
            }
        }
        background=1;return; //Hash는즉시Present하지않고실제Renderer+60에1을쓴다.
    }
    if (kind==RendererDiagnosticKind::Fire) {
        o::RawSquidFinder finder(pool_,hash_,types_);
        for (auto sid=finder.Begin(DrawWaterFinderArea(visible),0);sid.value;sid=finder.Next()) { //같은네단계현재체인을순회하며추가free/dead필터는없다.
            const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("Renderer 진단 TYPE범위");if (!(types_[raw[10]].flags1&0x800U)) continue;
            const auto x=Coordinate(raw,14),y=Coordinate(raw,18);const auto point=ProjectCameraView(snapshot,x,y);
            if (!fireMode) {
                const auto graph=raw[30];const auto choose=[&] { if (!graph) return Color(hooks,0);if (graph==Need(hooks.invalidGraph)()) return Color(hooks,7);const auto colors=Need(hooks.colors)();if (colors.empty()) throw std::domain_error("Renderer 진단 색개수0");return colors[graph%colors.size()]; }; //현재graphBYTE와현재팔레트수의나머지다.
                const auto selected=choose();ScreenRect rect{Add(Sub(point[0],snapshot.cellPixels[0]),1),Add(Sub(point[1],snapshot.cellPixels[1]),1),Add(point[0],-1),Add(point[1],-1)};Need(hooks.screen.clip)(rect);Need(hooks.screen.rectangle)(rect,selected);Inward(rect,2,hooks.screen.rectangle,[selected] { return selected; }); //선택색은clip전에한번캡처하고세겹모두같은값이다.
            } else {
                const auto px=Add(point[0],-8),py=Add(point[1],-6);Need(hooks.screen.clip)({Add(Sub(px,snapshot.cellPixels[0]),1),Add(Sub(py,snapshot.cellPixels[1]),1),Add(point[0],-9),Add(point[1],-7)});
                constexpr std::array<std::int32_t,8> dx{0,1,1,1,0,-1,-1,-1},dy{-1,-1,0,1,1,1,0,-1}; //원문52f83c/85c의방향이다.
                for (std::size_t direction=0;direction<8;++direction) { //각방향마다현재59514c를다시읽고동일거리WORD만연결선으로그린다.
                    const auto distances=Need(hooks.distances)();if (distances.empty()) continue;const auto cx=CameraFtolLow(x),cy=CameraFtolLow(y);if (cx<0 || cy<0 || cx>256 || cy>256) Need(hooks.assertion)(0x1f9);
                    if (distances[Cell(cx,cy,distances.size())]==distances[Cell(Add(cx,dx[direction]),Add(cy,dy[direction]),distances.size())]) Need(hooks.line)({px,py},{Add(px,dx[direction]*7),Add(py,dy[direction]*5)},Color(hooks,7));
                }
            }
        }
        Need(hooks.screen.present)();return;
    }
    for (auto x=visible.left;x<=visible.right;x=Add(x,1)) { //Spot/Board는원문x외부/y내부순서와inclusive오른쪽/아래를유지한다.
        for (auto y=visible.top;y<=visible.bottom;y=Add(y,1)) { //현재spot과표면슬롯을그칸에서다시읽는다.
            std::uint8_t surface{};if (kind==RendererDiagnosticKind::Spot) { const o::Sid sid{(x<0 || y<0 || x>255 || y>255) ? std::uint16_t{} : hash_.Cell(0,x,y)};surface=pool_.Slot(sid)[12]; } //원문은표면BYTE를spot/색/진단보다먼저읽는다.
            const auto spot=spots_[Cell(x,y,spots_.size())];int index=-1;std::uint32_t selected{};
            if (kind==RendererDiagnosticKind::Board) { if (spot&8U) index=5;else if (spot&6U) index=1; }
            else { if (spot&0x40U) { index=7;selected=Color(hooks,7); }if (spot&0x80U) { if (index!=-1) Need(hooks.assertion)(0xfe);index=0;selected=Color(hooks,0); } }
            if (index<0) continue;const auto point=ProjectCameraView(snapshot,static_cast<float>(x),static_cast<float>(y));ScreenRect rect{Sub(point[0],snapshot.cellPixels[0]),Sub(point[1],snapshot.cellPixels[1]),point[0],point[1]};
            if (kind==RendererDiagnosticKind::Board) Inward(rect,3,hooks.screen.rectangle,[&] { return Color(hooks,static_cast<std::size_t>(index)); });
            else {
                for (unsigned count=0;count<6;++count) { //첫3겹은현재선택색,뒤3겹은표면BYTE0비트가있을때현재회색이다.
                    rect={Add(rect.left,1),Add(rect.top,1),Add(rect.right,-1),Add(rect.bottom,-1)};if (count<3 || (surface&1U)) Need(hooks.screen.rectangle)(rect,count<3 ? selected:Color(hooks,8));
                }
            }
        }
    }
    Need(hooks.screen.present)();
}
// 원문일반키분기는Console/모달뒤정지앞이며새Editor/mission관문을추가하지않는다.
bool DispatchRendererDiagnosticKey(InputEvent event,std::uint32_t special,std::uint32_t specialAuxiliary,std::uint32_t& fire,std::uint32_t& spot,std::uint32_t& board,std::uint32_t& redraw,const std::function<void(InputEvent)>& positionLog) {
    const auto code=event.code&0x03ffffffU;if (code!=7 && code!=0x10 && code!=0x13) return false;if (event.code&InputCode::kRelease) return true;
    if (code==7) { if (specialAuxiliary) { fire=fire==0;if (!fire) redraw=1; } }
    else if (special) { if (code==0x10) { if (!(event.code&InputCode::kShift)) { const auto next=static_cast<std::uint32_t>(spot==0);redraw=1;spot=next; }else Need(positionLog)(event); }else { board=board==0;if (!board) redraw=1; } }
    return true;
}
}
