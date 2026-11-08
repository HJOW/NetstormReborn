// Player 기록 변경을 실제 원본 순서로 수행하며 AI 생성/전략의 성공을 추정하지 않는다.
#include "o/RawPlayerParticipation.h"
#include <bit>
#include <string> // 원문 고정 Tell 서식의 현재 이름/색/번호만 조합한다.
#include <stdexcept>

namespace netstorm::o {
namespace {
// PATCH 원본 Player 폭/개수와 통신·동맹·brain 필드 위치다.
constexpr std::size_t kStride=0xb4,kCount=9,kFlags=0x50,kAllies=0x54,kBrain=0xb0,kSubscriber=0x2c;
struct BrainCall { // 실제 brain 파괴/반납 효과가 정상 또는 예외 복귀하기 전 현재 모듈을 반납하지 못하게 한다.
    std::size_t& calls; // 같은 실제 모듈의 C++ 호출 수다.
    explicit BrainCall(std::size_t& value):calls(value) { ++calls; } // 선택한 비영 brain의 실제 효과 직전에 시작한다.
    ~BrainCall() { --calls; } // 앞선 부분 효과를 되돌리지 않고 호출 수명만 끝낸다.
};
constexpr std::array<std::string_view,8> kAllianceTellColors{"~l","~r","~w","~u","~s","~y","~d","~a"}; //4cef50의 Tell 색이며 MessageGump8색과 다르다.
// 실제 소비 필드만 읽어 relay/안내의 앞선 부분 효과를 선행 길이 검사로 없애지 않는다.
std::uint8_t AlliancePacketByte(std::span<const std::uint8_t> packet,std::size_t offset) {
    if (offset>=packet.size()) throw std::out_of_range("동맹 협상의 현재 packet BYTE 물리 부족");return packet[offset];
}
// 원문1..8 번호만 특수 색이며 그 밖은~q다. 이 helper는 효과나 Player 조회를 하지 않는다.
std::string_view AllianceTellColor(std::uint32_t owner) { return owner>=1 && owner<=8 ? kAllianceTellColors[owner-1] : std::string_view("~q"); }
// 실제 도달한 효과만 필수로 요구한다. 부재를 성공이나 가짜 장치로 바꾸지 않는다.
template<class Function> const Function& Require(const Function& function) {
    if (!function) throw std::logic_error("Player 참여 초기화의 실제 외부효과 미연결");return function;
}
}
// 실제 일반 입력의9칸 제한 갱신은 target 검사보다 먼저 수행하며 fa/7e/db에는 제한을 적용하지 않는다.
bool HandleAllianceRequestInput(std::uint32_t event,std::int32_t target,AllianceRequestState& state,std::span<const std::uint8_t> players,const AllianceRequestInputHooks& hooks) {
    const auto command=event&0x03ffffffU; //원문 공통 modifier 제거 뒤 네 명령만 선택한다.
    if (command!=0x020000fbU && command!=0x020000faU && command!=0x0200007eU && command!=0x020000dbU) return false;
    if (event&0x80000000U) return true;
    const auto message=static_cast<std::uint8_t>(command==0x020000faU ? 0x79U : command==0x0200007eU ? 0x78U : command==0x020000dbU ? 0x6fU : 0x77U); //기존78h 강제동맹 receiver와 같은 송신만 사용한다.
    if (command==0x020000fbU) {
        if (!state.initialized) {
            for (std::size_t owner=0;owner<kCount;++owner) { state.clocks[owner][1]=Require(hooks.clock)();state.counters[owner]=0; } //각start에 서로 다른 실제 clock 호출을 저장한다. sampled는 쓰지 않는다.
            state.initialized=1;
        } else {
            for (std::size_t owner=0;owner<kCount;++owner) { //4a68b0은현재clock을sample에먼저쓰고signed감김/1000의정수초를만든다.
                auto& pair=state.clocks[owner];pair[0]=Require(hooks.clock)();const auto elapsed=std::bit_cast<std::int32_t>(pair[0]-pair[1])/1000;
                if (elapsed>=state.windowSeconds) { pair[1]=Require(hooks.clock)();state.counters[owner]=0; } //같은60초도초기화하며reset의두번째clock조회는합치지않는다.
            }
        }
        const auto limit=state.limit; //원문은현재limit을target범위검사보다먼저보관한다.
        if (target<1 || target>8) return true;
        const auto owner=static_cast<std::uint32_t>(target);const auto count=state.counters[owner]+1U;state.counters[owner]=count;
        if (std::bit_cast<std::int32_t>(count)>limit) {
            const auto offset=std::size_t(owner)*kStride+12;if (offset>players.size() || 32>players.size()-offset) throw std::out_of_range("동맹 요청 안내의 현재 Player 이름 물리 범위");
            const auto name=players.subspan(offset,32);std::size_t length{};while (length<name.size() && name[length]) ++length; //원문이실제서식에서소비하는현재첫NUL까지읽는다.
            if (length==name.size()) throw std::out_of_range("동맹 요청 안내의 현재 이름 NUL 부족");
            const auto text=std::string("AllianceSpam(")+std::to_string(state.limit)+","+std::string(AllianceTellColor(owner))+","+std::string(reinterpret_cast<const char*>(name.data()),length)+")";
            Require(hooks.tell)(text);return true;
        }
    }
    std::array<std::uint8_t,6> packet{1,message,6,0,static_cast<std::uint8_t>(Require(hooks.localOwner)()),command==0x020000dbU ? std::uint8_t{1}:static_cast<std::uint8_t>(target)}; //DeclareDraw는기존80bit를반전하지않고항상yes1이다.
    Require(hooks.send)(packet);return true;
}
// 실제 한 저장소의 물리 범위와 판본을 변경 전에 확인한다.
RawPlayerParticipation::RawPlayerParticipation(OriginalEdition edition,std::span<std::uint8_t> players,std::span<std::uint32_t> allies,
    std::span<const std::uint8_t> hidden,PlayerParticipationState& state,PlayerParticipationHooks hooks)
    :players_(players),allies_(allies),hidden_(hidden),state_(state),hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078 || players.size()!=kStride*kCount || allies.size()!=kCount*kCount || hidden.size()!=kCount)
        throw std::invalid_argument("Player 참여 초기화의 PATCH 공유 저장소 크기 오류");
}
// 부분 바인딩은 현재 전역과 값형 사본을 섞으므로 첫 변경 전에 거부한다.
void RawPlayerParticipation::BindCurrentInputs(PlayerParticipationReads reads) {
    if (!reads.localOwner || !reads.playerKnown || !reads.battle || !reads.metamap || !reads.localTransport)
        throw std::invalid_argument("Player 참여 현재 전역 조회 다섯 개 누락");
    reads_=std::move(reads);
}
// 값형 원본 fixture를 보존하며 실제 세션에서는 현재 owner를 직접 읽는다.
std::uint32_t RawPlayerParticipation::LocalOwner() const { return reads_.localOwner ? reads_.localOwner() : state_.localOwner; }
// 기술 부모와 같은 현재 known DWORD를 읽는다.
std::uint32_t RawPlayerParticipation::PlayerKnown() const { return reads_.playerKnown ? reads_.playerKnown() : state_.playerKnown; }
// 현재 전투 모드를 원본의 선택 지점에서 읽는다.
bool RawPlayerParticipation::Battle() const { return reads_.battle ? reads_.battle() : state_.battle; }
// 각 원본 진단 직전에 현재 metamap을 읽는다.
bool RawPlayerParticipation::Metamap() const { return reads_.metamap ? reads_.metamap() : state_.metamap; }
// 실제 로컬 전송 여부는 연결 수락 분기에서만 읽는다.
bool RawPlayerParticipation::LocalTransport() const { return reads_.localTransport ? reads_.localTransport() : state_.localTransport; }
// 0번 중립 기록은 동맹/초기화의 실제 입력이므로 허용한다.
std::span<std::uint8_t> RawPlayerParticipation::Player(std::uint32_t owner) const {
    if (owner>=kCount) throw std::out_of_range("Player 참여 번호 범위 오류");return players_.subspan(owner*kStride,kStride);
}
// 정렬되지 않은 원본 DWORD를 호스트 엔디언과 무관하게 읽는다.
std::uint32_t RawPlayerParticipation::Read(std::uint32_t owner,std::size_t offset) const {
    const auto raw=Player(owner);std::uint32_t result=0;
    // 낮은 바이트부터 원본 폭 네 바이트를 모은다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(raw[offset+index])<<(8*index);return result;
}
// 같은 raw 기록의 지정 DWORD만 변경한다.
void RawPlayerParticipation::Put(std::uint32_t owner,std::size_t offset,std::uint32_t value) const {
    auto raw=Player(owner);
    // 낮은 바이트부터 현재 저장소에 기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 원본 이름 필드+0xc부터subscriber+0x2c직전까지32바이트 안의 정상 문자열만 제공한다.
std::string_view RawPlayerParticipation::Name(std::uint32_t owner) const {
    const auto raw=Player(owner);const auto* text=reinterpret_cast<const char*>(raw.data()+12);std::size_t length=0;
    // NUL 이전 바이트만 실제 채널 효과로 전달한다.
    while (length<32 && text[length]) ++length;
    if (length==32) throw std::out_of_range("Player 참여 이름의 NUL 종료 없음");return {text,length};
}
// 참여비트2/연결비트1을 함께 켜며 해제100만 지우고 다른 flag는 보존한다.
bool RawPlayerParticipation::CheckLogin(std::int32_t pending,std::int32_t owner) const {
    if (owner>0 && owner<static_cast<std::int32_t>(kCount) && !(Read(static_cast<std::uint32_t>(owner),kFlags)&1)) {
        const auto player=static_cast<std::uint32_t>(owner);Put(player,kFlags,(Read(player,kFlags)&~0x100U)|3U);
        if (!(LocalTransport() && pending==owner)) Require(hooks_.assignConnection)(pending,owner);return true;
    }
    Require(hooks_.rejectLogin)(pending);Require(hooks_.disconnect)(pending);return false;
}
// 서로 다른 방향은 원본처럼 독립이다. Chat은 양방향 동맹이 모두 참인 때만40채널에 등록한다.
void RawPlayerParticipation::SetAlliance(std::uint32_t owner,std::uint32_t mask) const {
    Player(owner);mask|=1U<<(owner&31);Put(owner,kAllies,mask);
    // 원본은 한 행의9개 DWORD를 각각0/1로 갱신한다.
    for (std::uint32_t target=0;target<kCount;++target) allies_[owner*kCount+target]=(mask>>target)&1;
    if (!PlayerKnown()) return;
    Require(hooks_.clearChat)(0x40);Require(hooks_.clearChat)(0x80);
    // 채널 효과 뒤 현재 로컬 번호/표/Player를 다시 읽어 다음 owner를 처리한다.
    for (std::uint32_t player=0;player<kCount;++player) {
        if (LocalOwner()>=kCount) throw std::out_of_range("Player 참여 Chat의 로컬 번호 오류");
        if (allies_[LocalOwner()*kCount+player] && allies_[player*kCount+LocalOwner()]) Require(hooks_.addChat)(Read(player,kSubscriber),Name(player),0x40);
        if (Metamap()) Require(hooks_.assertion)(PlayerParticipationAssertion::MetamapMode);
        if (hidden_[player]) Require(hooks_.addChat)(Read(player,kSubscriber),Name(player),0x80);
    }
}
// 가상 owner getter는 현재 Player의 안정된 span인덱스와 같다. metamap의 진단은 원본 getter 호출마다 유지한다.
void RawPlayerParticipation::AddAlly(std::uint32_t owner,std::uint32_t target) const {
    Player(owner);Player(target);Put(owner,kAllies,Read(owner,kAllies)|(1U<<(target&31)));
    if (Metamap()) Require(hooks_.assertion)(PlayerParticipationAssertion::MetamapMode);
    const bool battle=Battle();allies_[owner*kCount+target]=1;
    if (battle) {
        if (Metamap()) Require(hooks_.assertion)(PlayerParticipationAssertion::MetamapMode);
        Require(hooks_.sendAlliance)(9,owner,Read(owner,kAllies));
    }
}
// 먼저 실제 mask를 바꾸며 이후 진단/물리 방향표 실패도 앞선 부분 효과를 취소하지 않는다.
void RawPlayerParticipation::RemoveAlly(std::uint32_t owner,std::uint32_t target) const {
    Put(owner,kAllies,Read(owner,kAllies)&~(1U<<(target&31)));
    if (Metamap()) Require(hooks_.assertion)(PlayerParticipationAssertion::MetamapMode);
    const bool battle=Battle();const auto cell=std::size_t(owner)*kCount+target; //원문은 번호 진단 뒤 battle을 저장하고 같은 방향 한칸만 지운다.
    if (cell>=allies_.size()) throw std::out_of_range("동맹 해제의 현재 방향표 물리 범위");allies_[cell]=0;
    if (battle) {
        if (Metamap()) Require(hooks_.assertion)(PlayerParticipationAssertion::MetamapMode);
        Require(hooks_.sendAlliance)(9,owner,Read(owner,kAllies));
    }
}
// 원문은 requester 주소를 먼저 보관하고 boss relay 뒤에 현재 target/요청 BYTE를 다시 읽는다.
void RawPlayerParticipation::ReceiveAllianceRequest(std::span<const std::uint8_t> packet,const AllianceNegotiationHooks& hooks) const {
    const auto boss=Require(hooks.boss)(); //PE48f7f6이 저장한 현재boss를 requester BYTE 조회보다 먼저 읽는다.
    const auto requester=std::uint32_t(AlliancePacketByte(packet,4)); //주소 선택만 보관하며 선택되지 않은 Player 기록을 미리 읽지 않는다.
    if (boss) Require(hooks.broadcast)(packet);
    if (AlliancePacketByte(packet,5)==LocalOwner() && !Require(hooks.ignoredSubscriber)(Read(requester,kSubscriber))) {
        const auto color=AllianceTellColor(AlliancePacketByte(packet,4));const auto name=Name(requester); //ignored 효과 뒤 같은 최초 Player의 현재 이름을 읽는다.
        const auto text=std::string("AllianceRequested(")+std::string(color)+","+std::string(name)+","+std::to_string(AlliancePacketByte(packet,4))+")";
        Require(hooks.tell)(text);
    }
}
// 두 owner/색/Player 선택은 효과 전 값이며 각 Tell의 이름 내용은 relay 뒤 현재값이다.
void RawPlayerParticipation::ReceiveBrokenAlliance(std::span<const std::uint8_t> packet,const AllianceNegotiationHooks& hooks) const {
    const auto first=std::uint32_t(AlliancePacketByte(packet,4)),second=std::uint32_t(AlliancePacketByte(packet,5));
    const auto firstColor=AllianceTellColor(first),secondColor=AllianceTellColor(second);
    if (Require(hooks.boss)()) { RemoveAlly(first,second);RemoveAlly(second,first);Require(hooks.broadcast)(packet); }
    const auto local=LocalOwner(); //PE48f961은 relay 뒤 현재local을 한 번 보관해 두 owner와 비교한다.
    if (local==first) { Require(hooks.tell)(std::string("AllianceBroken(")+std::string(Name(second))+","+std::string(secondColor)+")");return; }
    if (local==second) { Require(hooks.tell)(std::string("AllianceBroken(")+std::string(Name(first))+","+std::string(firstColor)+")");return; }
    Require(hooks.status)(8,first,second);
}
// 거부는 Tell 뒤 현재 flag80만 지운다. 합의는 안내 뒤 현재boss, relay 뒤 현재yes/게임시각을 읽는다.
void RawPlayerParticipation::ReceiveDraw(std::span<const std::uint8_t> packet,double& deadline,const AllianceNegotiationHooks& hooks) const {
    if (!AlliancePacketByte(packet,5)) {
        if (Read(LocalOwner(),kFlags)&0x80U) Require(hooks.tell)("DrawNotAccepted");
        for (std::uint32_t owner=1;owner<kCount;++owner) Put(owner,kFlags,Read(owner,kFlags)&~0x80U); //Tell 효과가 바꾼 각 현재 DWORD의80 bit만 해제한다.
        return;
    }
    const auto owner=std::uint32_t(AlliancePacketByte(packet,4));Put(owner,kFlags,Read(owner,kFlags)|0x80U);
    Require(hooks.status)(6,AlliancePacketByte(packet,4),0);
    if (Require(hooks.boss)()) {
        Require(hooks.broadcast)(packet);
        if (AlliancePacketByte(packet,5)) deadline=Require(hooks.gameSeconds)()+20.0; //원문 수신은 거부/종료 없이 별도595358만 현재시각으로 예약한다.
    }
}
// 원본의 수신 버퍼 직접 쓰기와 같은 Player/방향별 동맹/현재 Chat 재구성을 실행한다.
void RawPlayerParticipation::ReceiveAlliance(std::span<std::uint8_t> packet,const std::function<void(std::span<const std::uint8_t>)>& broadcast) const {
    if (packet.size()<9) throw std::out_of_range("동맹 수신의 실제9BYTE 미만 입력");
    const auto owner=std::uint32_t(packet[4]);Player(owner); // 원본 유효0..8 owner 계약을 변경 전에 확인한다.
    std::uint32_t mask=1U<<(owner&31);
    // 비정렬 packet+5의 현재 DWORD를 원본 순서로 조립한다.
    for (std::size_t index=0;index<4;++index) mask|=std::uint32_t(packet[5+index])<<(index*8);
    // 원본은 입력 packet에도 self bit를 기록하므로 broadcast가 같은 변경 바이트를 읽는다.
    for (std::size_t index=0;index<4;++index) packet[5+index]=static_cast<std::uint8_t>(mask>>(index*8));
    Put(owner,kAllies,mask);SetAlliance(owner,mask);Require(broadcast)(packet);
}
// 실제 AI 생성은 상위의00417e50/00416860이며 이 필드 setter가 대신 생성하지 않는다.
void RawPlayerParticipation::AttachBrain(std::uint32_t owner,std::uint32_t brain) const {
    if (BrainReturnedForState(owner)) throw std::logic_error("Native 반납된 AI owner의 새 brain 설치 금지"); // 실제 할당/필드쓰기로 수명 증빙을 무효화하지 않는다.
    if (Read(owner,kBrain)) Require(hooks_.assertion)(PlayerParticipationAssertion::BrainAlreadyAssigned);
    Put(owner,kBrain,brain);Put(owner,kFlags,Read(owner,kFlags)|0x20);
    if (std::bit_cast<std::int32_t>(Read(owner,kSubscriber))<1) {
        if (Metamap()) Require(hooks_.assertion)(PlayerParticipationAssertion::MetamapMode);
        Put(owner,kSubscriber,owner+10000);auto raw=Player(owner);
        // 원본은 DWORD7개와 마지막WORD를 써 +30..4d의30바이트만 지운다.
        for (std::size_t offset=0x30;offset<0x4e;++offset) raw[offset]=0;
    }
}
// 현재 실제 소유자의 토큰만 읽는다. flags20이나 연결상태로 포인터를 추정하지 않는다.
std::uint32_t RawPlayerParticipation::Brain(std::uint32_t owner) const { return Read(owner,kBrain); }
// 파괴/반납 callback 시점에는 원본처럼 이전 포인터를 Player에서 계속 읽을 수 있다.
void RawPlayerParticipation::RemoveBrain(std::uint32_t owner) const {
    if (BrainReturnedForState(owner)) throw std::logic_error("Native 반납된 AI owner의 반복 reset 금지");
    const auto brain=Brain(owner);if (!brain) return;BrainCall call(activeBrainCalls_);Require(hooks_.destructBrain)(brain);Require(hooks_.freeBrain)(brain);Put(owner,kBrain,0);
}
// 원문 cleanup의 owner1..8 위치에서 같은 실제 부모를 끝낸 뒤에만 이동 전용 증빙을 만든다.
RawPlayerParticipation::BrainResetPass RawPlayerParticipation::RemoveBrainForState(std::uint32_t owner) const {
    if (owner<1 || owner>=kCount) throw std::out_of_range("State AI 반납의 실제 owner1..8 범위");
    RemoveBrain(owner);return BrainResetPass(this,retirementLifetime_,owner);
}
// 정상 복귀와 현재 모듈의 같은 대여만 소비한다. 실패한 앞선 효과나 다른 owner 상태는 복구하지 않는다.
void RawPlayerParticipation::ReturnBrainForState(BrainResetPass&& pass) {
    const auto owner=pass.owner_;
    if (pass.source_!=this || pass.lifetime_.lock()!=retirementLifetime_ || owner<1 || owner>=kCount) throw std::invalid_argument("같은 실제 AI 모듈의 정상 reset 증빙 필요");
    if (activeBrainCalls_) throw std::logic_error("진행 중 AI 파괴/반납 효과의 증빙 소비 금지");
    if (returnedBrains_[owner] || Brain(owner)) throw std::logic_error("현재 brain0인 미반납 AI owner 증빙 필요");
    returnedBrains_[owner].emplace(std::move(pass)); // weak 증빙의 noexcept 이동만 하며 실제 소비 뒤 추가 할당하지 않는다.
}
// 실제 증빙이 없는 일반 경로는 종전대로 동작하며 raw 필드 외부변조도 새 brain을 숨기지 않는다.
bool RawPlayerParticipation::BrainReturnedForState(std::uint32_t owner) const {
    Player(owner);if (!returnedBrains_[owner]) return false;
    if (Brain(owner)) throw std::logic_error("반납된 AI owner의 현재 brain 정체성 변경");return true;
}
// 0번 중립 AI는 이 원본 전체삭제 루프의 대상이 아니다.
void RawPlayerParticipation::RemoveAllBrains() const {
    // 실제 owner1..8의 현재 포인터를 각 방문 시점에 새로 읽는다.
    for (std::uint32_t owner=1;owner<kCount;++owner) { // 이미 State가 반납한 NULL도 원문처럼 자연 반환한다. 개별 State reset의 중복 증빙 관문은 유지한다.
        if (!BrainReturnedForState(owner)) RemoveBrain(owner); // 증빙이 있어도 현재 brain 변경은 실제 조회가 거부하며 전체 루프를 생략하지 않는다.
    }
}
}
