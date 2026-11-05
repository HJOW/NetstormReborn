// 원본 함수의 바이트 쓰기를 보존하며 미복원 생성자/공간 효과는 변경 전에 거부한다.
#include "o/SquidFactory.h"
#include "o/SquidUnpop.h"
#include "o/SquidForms.h"
#include "o/RawRiftHitPoints.h"
#include "o/RawSquidTake.h" // 공통 Take도 현재 가상 해제/생성자 반환 객체를 부모에 전달한다.
#include "o/RawSquidCreation.h" // 원문 생성 부모가 같은 실제 풀/TYPE/생성자/가상 초기화를 호출한다.
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본의 공통 raw 필드 위치와 상태 비트다.
constexpr std::size_t kType=10,kState=11,kHp=26;
constexpr std::uint8_t kFree=1,kVoid=4,kContained=8;
// 원본 타입 번호와 dais의 HP 필요성 플래그다. 두 판본의 PE 초기값/로딩 순서로 대조했다.
constexpr std::uint32_t kManaType=158,kFakeSurface=162,kDaisFlag=0x400000;
constexpr std::uint32_t kDaisType=165; // 독립 입력의 초기 예외 번호이며 실제 월드는 같은 현재 DWORD 주소를 명시 연결한다.
// 호스트 포인터로 사용하지 않는 실제 32비트 base vtable 기록값이다.
constexpr std::uint32_t kPatchVtable=0x501dd0,kCdVtable=0x5058d8;
// 생산 칸 contained Form의 내장 타입 번호다. 생성자 기록값은 공통 SquidForms가 제공한다.
constexpr std::uint32_t kContainedType=6;
// GumpForm의 내장 타입 번호다. 메뉴의 등록 ID는 이후 +0x12에 별도로 쓴다.
constexpr std::uint32_t kGumpFormType=3;
#include "TypeConstructors.inc"
// 순서 있는 생성자 쓰기의 위치·폭·OR 여부와 기록값이다. 최대 다섯 쓰기를 보관한다.
struct ConstructorWrite { /* offset: 배열·파일·좌표 계산에 더할 기준 오프셋. */ /* width: 가로 크기. */ std::uint8_t offset,width; /* orBits: 비트OR 비트 여부. */ bool orBits; /* value: 읽기·쓰기·통지에 전달하는 값. */ std::uint32_t value; };
// 원본 타입 생성자의 주소·가상함수표·필드 쓰기 목록이다.
struct ConstructorRecipe {
    // address: 원본 실행 파일 안의 함수 시작 주소.
    // vtable: 원본 객체의 가상 함수 표 주소.
    std::uint32_t address,vtable;
    // count: 현재 항목 또는 실행 횟수.
    std::uint8_t count;
    // writes: 쓰기.
    std::array<ConstructorWrite,5> writes;
};
#include "DerivedConstructors.inc"
// 호스트 함수 호출 없이 판본과 원본 주소가 일치하는 검증된 쓰기 계약을 찾는다.
const ConstructorRecipe* Recipe(OriginalEdition edition,std::uint32_t address) {
    const auto recipes=edition==OriginalEdition::Patch1078 ? std::span<const ConstructorRecipe>(kPatchRecipes) :
        std::span<const ConstructorRecipe>(kCdRecipes);
    // 주소가 표에 없으면 null/assert/미복원 생성자로 간주한다.
    for (const auto& recipe:recipes) if (recipe.address==address) return &recipe;
    return nullptr;
}
// 원본 포인터/HP를 little endian raw 필드로 쓴다. 정렬되지 않은 +26 쓰기도 안전하다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    // CD HP는 2바이트이고 패치 HP는 4바이트다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 선택된 검증 recipe의 순서/폭/OR를 같은 물리 슬롯에 적용한다. TYPE 조회나 할당은 하지 않는다.
void ApplyRecipe(std::span<std::uint8_t> bytes,const ConstructorRecipe& recipe) {
    // 쓰기 순서와 폭을 유지하여 섬 번호·플래그·anim HP의 판본 차이를 보존한다.
    for (std::size_t i=0;i<recipe.count;++i) {
        const auto& write=recipe.writes[i]; auto value=write.value;
        if (write.orBits) {
            // OR에 필요한 기존 바이트만 읽는다. 이웃 필드는 보존한다.
            for (std::size_t j=0;j<write.width;++j) value|=static_cast<std::uint32_t>(bytes[write.offset+j])<<(j*8);
        }
        Put(bytes,write.offset,value,write.width);
    }
}
// 이전 vtable이 실제 base 객체인지 raw 기록값만 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> bytes) {
    std::uint32_t value=0;
    // 네 바이트를 호스트 엔디언과 무관하게 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
}
// 타입 번호 밖 조회를 거부하고 원본 주소 메타데이터를 반환한다.
std::uint32_t TypeConstructorAddress(OriginalEdition edition,std::size_t number) {
    const auto values=edition==OriginalEdition::Patch1078 ? std::span<const std::uint32_t>(kPatchConstructors) :
        std::span<const std::uint32_t>(kCdConstructors);
    if (number>=values.size()) throw std::out_of_range("생성자 타입 번호 오류");
    return values[number];
}
// pool 판본에 맞는 타입 배열을 소유한다. 실제 타입 로더 또는 합성 검증 입력을 사용할 수 있다.
SquidFactory::SquidFactory(SidPool& pool,std::span<const RiftTypeRecord> types,bool weakenedMana,SquidUnpop* unpop)
    :pool_(pool),types_(types.begin(),types.end()),weakenedMana_(weakenedMana),unpop_(unpop) {
    const auto count=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchConstructors.size() : kCdConstructors.size();
    if (types_.size()!=count) throw std::invalid_argument("SID 판본과 타입 수가 다릅니다");
    if (unpop_ && &unpop_->Pool()!=&pool_) throw std::invalid_argument("Unpop과 Factory의 SID 풀이 다릅니다");
}
// 현재 TYPE를 빌린 실제 소비자와 기존 사본을 쓰는 독립 입력의 수명을 구별한다.
void SquidFactory::BindDiagnostics(SquidFactoryDiagnostics diagnostics) {
    if (!diagnostics.types.empty() && diagnostics.types.size()!=types_.size()) throw std::invalid_argument("Factory 현재 TYPE의 판본/길이 오류");
    diagnostics_=std::move(diagnostics);
}
// PostTake 진단 복귀 뒤에는 Form 타입도 같은 실제 표에서 읽으며 배열 밖 접근만 거부한다.
std::span<const RiftTypeRecord> SquidFactory::CurrentTypes() const { return diagnostics_.types.empty() ? std::span<const RiftTypeRecord>(types_) : diagnostics_.types; } // 진단 뒤에도 같은 현재 표를 조회한다.
std::int32_t SquidFactory::TypeHitPoints(const RiftTypeRecord& type,std::span<const RiftTypeRecord> selectedBase) const { // 선택한 TYPE의 현재 HP를 두 판본의 실제 getter에 연결한다.
    RiftHitPointHooks hooks;
    hooks.number=[this,selectedBase,&type] { return CurrentRiftTypeNumber(type,selectedBase,CurrentTypes()); }; // 진단 뒤 현재 base에 대한 같은 TYPE this의 번호를 다시 계산한다.
    hooks.assertion=[this](auto expression,auto file,auto line) { const auto selected=diagnostics_.assertion;if (!selected) throw std::logic_error("생성 TYPE HP의 실제 진단 장치 미연결");selected(expression,file,line); };
    return ReadRiftHitPoints(pool_.Edition(),type,{diagnostics_.checking,RawCheckingFlag{sharedWeakenedMana_ ? *sharedWeakenedMana_ : weakenedMana_},sharedPriestType_ ? *sharedPriestType_ : kManaType,sharedDaisType_ ? *sharedDaisType_ : kDaisType},hooks);
}
const RiftTypeRecord& SquidFactory::TypeRecord(std::uint32_t number) const {
    const auto types=CurrentTypes();
    if (number>=types.size()) throw std::out_of_range("Factory 현재 TYPE 번호 오류");return types[number];
}
std::int32_t SquidFactory::ReadSelectedHitPoints(const RiftTypeRecord& type,std::span<const RiftTypeRecord> selectedBase) const { return TypeHitPoints(type,selectedBase); } // 번호로 다시 선택하지 않고 같은 물리 TYPE를 공통 getter에 전달한다.
// 미복원 생성자 호출이나 일반 자산 경로에 Form 가상 메서드를 적용하는 것을 막는다.
const RiftTypeRecord& SquidFactory::Type(std::uint32_t number,bool requireSupported) const {
    if (number<kFirstAssetTypeNumber) throw std::out_of_range("base Squid 타입 번호 오류");
    const auto& type=TypeRecord(number);
    if (requireSupported && type.constructorAddress && !Recipe(pool_.Edition(),type.constructorAddress))
        throw std::logic_error("null/assert 또는 미복원 Squid 생성자는 지원하지 않습니다");
    return type;
}
// 진단 장치가 돌아오면 원문 다음 위치에서 현재 타입/상태를 다시 읽는다.
void SquidFactory::Assert(std::string_view expression,std::uint32_t line) const {
    const auto assertion=diagnostics_.assertion; // 진단이 장치를 재연결해도 실행 중인 callable의 수명을 보존한다.
    if (!assertion) throw std::logic_error("Factory의 선택된 원문 진단 장치 미연결");
    assertion(expression,pool_.Edition()==OriginalEdition::Patch1078 ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",line);
}
// Take로 받은 예측 머리도 원본 범위 안이다. 일반 Allocate의 예약 검사와 분리한다.
std::span<std::uint8_t> SquidFactory::LiveBytes(Sid sid) {
    const auto offset=pool_.Offset(sid);
    auto bytes=std::span(pool_.bytes_).subspan(offset,pool_.layout_.stride);
    if (sid.value<5 || (bytes[kState]&(kFree|kContained))) throw std::logic_error("base 초기화의 SID 상태 오류");
    Type(bytes[kType],false);
    return bytes;
}
// 판본별 실제 base vtable의 기록값이다.
std::uint32_t SquidFactory::BaseVtable() const { return pool_.Edition()==OriginalEdition::Patch1078 ? kPatchVtable : kCdVtable; }
// 각 생성자의 최종 vtable만 읽는다. 중간 vtable은 ApplyConstructor에서 원본 순서로 쓴다.
std::uint32_t SquidFactory::ConstructorVtable(const RiftTypeRecord& type) const {
    if (!type.constructorAddress) return BaseVtable();
    for (const auto formType:{1U,2U,3U,6U}) { // 일반Create도 네Form의 원문 생성자/VT를 사용한다.
        if (type.constructorAddress==TypeConstructorAddress(pool_.Edition(),formType)) return SquidForms(pool_).Vtable(formType);
    }
    const auto recipe=Recipe(pool_.Edition(),type.constructorAddress);if (!recipe) throw std::logic_error("현재 TYPE의 미등록 생성자 가상 표 조회");return recipe->vtable;
}
// 원본 생성자 표를 재사용하므로 가상 분배용83개 목록을 별도로 복제하지 않는다.
bool SquidFactory::SupportsConstructedVtable(OriginalEdition edition,std::uint32_t table) {
    if (table==(edition==OriginalEdition::Patch1078 ? kPatchVtable : kCdVtable)) return true;
    const auto recipes=edition==OriginalEdition::Patch1078 ? std::span<const ConstructorRecipe>(kPatchRecipes) : std::span<const ConstructorRecipe>(kCdRecipes);
    for (const auto& recipe:recipes) if (recipe.vtable==table) return true;return false; // 현재 판본의 실제 최종 VT만 허용한다.
}
// 원본의 짧은 생성자는 같은 raw 슬롯에 mov/OR를 적용한다. CD Bomb 보조 생성자도 포함한다.
void SquidFactory::ApplyConstructor(std::span<std::uint8_t> bytes,const RiftTypeRecord& type) const {
    if (!type.constructorAddress) { Put(bytes,0,BaseVtable(),4); return; }
    ApplyRecipe(bytes,*Recipe(pool_.Edition(),type.constructorAddress));
}
// 주소별 cdecl은 기존 슬롯의 TYPE/free/dead/예약 상태를 다시 해석하거나 새 객체를 할당하지 않는다.
bool SquidFactory::ConstructByAddress(std::uint32_t address,Sid sid) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch && address==0x4429e0U) return true; // 원문 NULL 함수는 SID/풀조차 조회하지 않는다.
    const auto* recipe=Recipe(pool_.Edition(),address);
    const bool dais=patch && address==0x44a420U;
    if (!recipe && !dais) return false; // 직접 NULL 함수 포인터0을 base 생성자로 대체하지 않는다.
    if (dais) {
        const auto assertion=diagnostics_.assertion; // 콜백이 자기 연결을 반납해도 선택한 진단은 호출 끝까지 살아 있다.
        if (!assertion) throw std::logic_error("Dais 보조 생성자의 실제 진단 장치 미연결");
        assertion("false",".\\Dais.cpp",0x484);
    }
    const auto offset=pool_.Offset(sid); // 진단 정상 복귀 뒤에만 물리 주소를 조회한다.
    auto bytes=std::span(pool_.bytes_).subspan(offset,pool_.layout_.stride);
    if (recipe) ApplyRecipe(bytes,*recipe);
    else Put(bytes,0,0x507950U,4); // 진단이 돌아온 원문169는 이DWORD만 쓴다.
    return true;
}
// 생성자 자체는 type 번호·상태·가상 초기화를 쓰지 않는다. 독립 검증과 재초기화에 사용한다.
Sid SquidFactory::Construct(std::uint32_t type,Sid sid) {
    const auto& record=Type(type,true); const auto offset=pool_.Offset(sid);
    auto bytes=std::span(pool_.bytes_).subspan(offset,pool_.layout_.stride);
    if (sid.value<5 || (bytes[kState]&(kFree|kContained))) throw std::logic_error("생성자 슬롯 상태 오류");
    ApplyConstructor(bytes,record); return sid;
}
// 현재 원문 cdecl 주소를 타입/상태 선행 검사 없이 같은 물리 슬롯에 적용한다.
Sid SquidFactory::ConstructRaw(std::uint32_t address,Sid sid,bool requireObject) {
    if (!address) { Put(std::span(pool_.bytes_).subspan(pool_.Offset(sid),pool_.layout_.stride),0,BaseVtable(),4);return sid; }
    // 네 Form 생성자는 TYPE/부모/목록을 쓰지 않고 VT DWORD만 남긴다.
    for (const auto formType:{1U,2U,3U,6U}) {
        if (address==TypeConstructorAddress(pool_.Edition(),formType)) { SquidForms(pool_).ConstructVtableAt(formType,sid);return sid; }
    }
    if (!ConstructByAddress(address,sid)) throw std::logic_error("Create/현재 TYPE의 선택된 cdecl 생성자 미연결");
    if (requireObject && pool_.Edition()==OriginalEdition::Patch1078 && address==0x4429e0U)
        throw std::logic_error("void NULL 생성자는 Create의 반환 객체를 제공하지 않습니다");
    return sid;
}
void SquidFactory::ConstructCurrent(Sid sid) { // 원문은 free/dead/contained/예약 SID를 선행 제외하거나 TYPE/HP를 쓰지 않는다.
    const auto address=TypeRecord(pool_.Slot(sid)[kType]).constructorAddress;
    ConstructRaw(address,sid,false);
}
void SquidFactory::VirtualPostCreate(Sid sid,std::uint32_t table) { // 실제 VT를 사용하고 원래 TYPE와의 쌍 일치를 요구하지 않는다.
    const SquidForms forms(pool_);
    for (const auto formType:{1U,2U,3U,6U}) { // 네 Form의+28은 PATCH4437f0/CD4b0470의 빈 함수다.
        if (table==forms.Vtable(formType)) return;
    }
    if (!SupportsConstructedVtable(pool_.Edition(),table)) throw std::logic_error("Create의 선택된 실제 VT+28 미연결");
    PostCreate(sid);
}
// 10.78의 타입 변경은 진단 복귀 뒤에도 계속하며 free/dead/예약 SID를 추가로 제외하지 않는다.
void SquidFactory::Retype(Sid sid,std::uint32_t type,const std::function<void(std::uint32_t)>& assertion) {
    if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("타입 변경4b1620은10.78만 복원했습니다");
    const auto diagnose=[&](std::uint32_t line) { if (!assertion) throw std::logic_error("타입 변경의 실제 원문 진단 미연결");assertion(line); }; // 원문 세 진단을 선택된 위치에서만 요구한다.
    if (!(pool_.Slot(sid)[kState]&kVoid)) diagnose(0x22d);
    if (TypeRecord(pool_.Slot(sid)[kType]).footX!=TypeRecord(type).footX) diagnose(0x22e);
    if (TypeRecord(pool_.Slot(sid)[kType]).footY!=TypeRecord(type).footY) diagnose(0x22f);
    const auto offset=pool_.Offset(sid);auto bytes=std::span(pool_.bytes_).subspan(offset,pool_.layout_.stride);
    bytes[kType]=static_cast<std::uint8_t>(type);ConstructCurrent(sid); // 원본4af720은 BYTE 쓰기 뒤 현재 TYPE의 실제 생성자를 다시 조회한다.
}
// 실제 부모가 진단→할당→현재 생성자→반환 객체 TYPE→실제VT+28을 선택한다.
Sid SquidFactory::Create(std::uint32_t type,std::uint32_t flags) {
    RawSquidCreationHooks hooks;
    hooks.fakeSurfaceType=[this] { return sharedFakeSurfaceType_ ? *sharedFakeSurfaceType_ : kFakeSurface; };
    hooks.assertion=[this](auto expression,auto file,auto line) { const auto selected=diagnostics_.assertion;if (!selected) throw std::logic_error("Factory Create의 실제 진단 장치 미연결");selected(expression,file,line); };
    hooks.allocate=[this](auto flags) { return pool_.Allocate(flags).value; };
    hooks.constructor=[this](auto number) { return TypeRecord(number).constructorAddress; };
    hooks.construct=[this](auto address,auto number) { // 원문 DWORD SID를 호스트 WORD로 자르기 전에 물리 범위를 확인한다.
        if (number>=pool_.Capacity()) throw std::out_of_range("Create 생성자의 실제 물리 SID 범위");
        return std::uint32_t(ConstructRaw(address,Sid{static_cast<std::uint16_t>(number)},true).value);
    };
    hooks.vtable=[this](auto object) { return Vtable(pool_.Slot(Sid{static_cast<std::uint16_t>(object)})); };
    hooks.storeType=[this](auto object,auto value) { std::span(pool_.bytes_).subspan(pool_.Offset(Sid{static_cast<std::uint16_t>(object)}),pool_.layout_.stride)[kType]=value; };
    hooks.postCreate=[this](auto object,auto table) { VirtualPostCreate(Sid{static_cast<std::uint16_t>(object)},table); };
    return Sid{static_cast<std::uint16_t>(CreateRawSquid(pool_.Edition(),type,flags,hooks))};
}
// 할당/생성 연결에서 월드 혼합을 거부하기 위한 참조다.
const SidPool& SquidFactory::Pool() const { return pool_; }
// 현재 생성 HP가 사용하는 같은 원문 전역만 빌리고 다른 owner로 바꾸지 않는다.
void SquidFactory::BindWeakenedMana(const bool& current) {
    if (sharedWeakenedMana_ && sharedWeakenedMana_!=&current) throw std::invalid_argument("생성 HP의 약화 전역 중복 다른 주소 연결");sharedWeakenedMana_=&current;
}
void SquidFactory::BindHitPointTypes(const std::uint32_t* priest,const std::uint32_t* dais) { // 같은 owner의 수명보다 공장이 오래 살지 않도록 실제 World가 장기 주소를 공급한다.
    if (!priest || !dais || (sharedPriestType_ && sharedPriestType_!=priest) || (sharedDaisType_ && sharedDaisType_!=dais)) throw std::invalid_argument("생성 HP의 특별 TYPE 주소 연결 오류");
    sharedPriestType_=priest;sharedDaisType_=dais;
}
void SquidFactory::BindFakeSurfaceType(const std::uint32_t& current) { // Session을 바꿔도 동일 owner의 현재 DWORD를 읽는다.
    if (sharedFakeSurfaceType_ && sharedFakeSurfaceType_!=&current) throw std::invalid_argument("Create fakeSurface TYPE의 중복 다른 주소 연결");sharedFakeSurfaceType_=&current;
}
Sid SquidFactory::CreateContained(std::uint32_t flags) {
    return SquidForms(pool_).Create(kContainedType,flags);
}
// 004af530의 GumpForm 경로다. 할당으로 초기화된 슬롯에 가상 표→타입만 쓰며 빈 postCreate를 생략한다.
Sid SquidFactory::CreateGumpForm(std::uint32_t flags) {
    return SquidForms(pool_).Create(kGumpFormType,flags);
}
// 두 판본 모두 실제 VT+48→현재 생성자→반환 객체→저장한 VT+2c의 원문 부모를 사용한다.
Sid SquidFactory::Take(std::uint32_t type,Sid sid) {
    const auto object=[this](std::uint32_t number) { if (number>=pool_.Capacity()) throw std::out_of_range("Take의 실제 물리 SID 범위");return Sid{static_cast<std::uint16_t>(number)}; }; // DWORD 토큰을 WORD로 자르기 전에 물리 범위를 확인한다.
    const auto raw=[this,object](std::uint32_t number) { return std::span(pool_.bytes_).subspan(pool_.Offset(object(number)),pool_.layout_.stride); }; // free/dead/contained를 선행 거부하지 않는다.
    RawSquidTakeHooks hooks;hooks.capacity=[this] { return pool_.Capacity(); };hooks.slot=[object](auto number) { return std::uint32_t(object(number).value); };
    hooks.state=[raw](auto number) { return raw(number)[kState]; };hooks.type=[raw](auto number) { return raw(number)[kType]; };hooks.vtable=[raw](auto number) { return Vtable(raw(number)); };
    hooks.detail=[this](auto serverType,auto clientType) { // 상세 writer/진단보다 먼저 현재 두 이름을 복사한다.
        const auto name=[](const RiftTypeRecord& record) { return record.name.substr(0,record.name.find('\0')); }; // TYPE+4의 첫NUL까지의 이름이다.
        const auto clientName=name(TypeRecord(clientType));const auto serverName=name(TypeRecord(serverType));const auto detail=std::string("Invalid take. Server type ")+serverName+", client type "+clientName;
        if (detail.size()>=256) throw std::length_error("Take 원문 상세256BYTE 범위");const auto writer=diagnostics_.detail;if (!writer) throw std::logic_error("Take의 실제 오류 상세 장치 미연결");writer(detail);
    };
    hooks.assertion=[this](auto expression,auto,auto line) { Assert(expression,line); };
    hooks.unpop=[this,object,raw](auto number,auto table) { // 공통 자산은 현재 VT로 분배한다. Form 하위의 기존 내부 검사는 별도 계약이다.
        const SquidForms forms(pool_);
        for (const auto formType:{1U,2U,3U,6U}) { if (table==forms.Vtable(formType)) { forms.Unpop(object(number));return; } } // 알려진 Form의 실제 공통 해제다. 필요한 부모 알림 부재는 그 종단에서 실패한다.
        if (!SupportsConstructedVtable(pool_.Edition(),table)) throw std::logic_error("Take의 선택된 실제 VT+48 미연결");
        if (!unpop_) throw std::logic_error("Take의 선택된 실제 공간 해제 연결이 없습니다");unpop_->Unpop(object(number),TypeRecord(raw(number)[kType]),0);
    };
    hooks.storeState=[raw](auto number,auto value) { raw(number)[kState]=value; };hooks.storeType=[raw](auto number,auto value) { raw(number)[kType]=value; };
    hooks.constructor=[this](auto number) { return TypeRecord(number).constructorAddress; };hooks.construct=[this,object](auto address,auto number) { return std::uint32_t(ConstructRaw(address,object(number),true).value); };
    hooks.postTake=[this,object](auto number,auto table) { // 네 Form의+2c는 빈 함수이며 자산은 같은 현재 공통 후처리를 사용한다.
        const SquidForms forms(pool_);for (const auto formType:{1U,2U,3U,6U}) if (table==forms.Vtable(formType)) return; // 원문 빈 함수에 자산 zOrder/진단을 추가하지 않는다.
        if (!SupportsConstructedVtable(pool_.Edition(),table)) throw std::logic_error("Take의 선택된 실제 VT+2c 미연결");PostTake(object(number));
    };
    return object(TakeRawSquid(pool_.Edition(),type,sid.value,hooks));
}
// 모든 지원 자산의 공통 firstPop은 type/풀을 쓰지 않고 extra의 두 비트만 읽는다.
std::uint32_t SquidFactory::FirstPopFlags(Sid sid) {
    const auto bytes=LiveBytes(sid); const auto& type=Type(bytes[kType],true);
    if (Vtable(bytes)!=ConstructorVtable(type)) throw std::logic_error("firstPop vtable 오류");
    const auto extra=bytes[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35];
    return (extra&9)==0 ? 1U : ((extra&1)!=0 ? 2U : 0U)|((extra&8)!=0 ? 4U : 0U);
}
// 두 판본의 세 진단이 돌아온 위치부터 같은 물리 슬롯/현재 TYPE로 계속한다.
void SquidFactory::PostCreate(Sid sid) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto offset=pool_.Offset(sid);auto bytes=std::span(pool_.bytes_).subspan(offset,pool_.layout_.stride); // free/dead/contained를 별도 선행 거부하지 않는 같은 물리 슬롯이다.
    if (bytes[kType]<kFirstAssetTypeNumber) Assert("!isForm()",patch ? 0x7d4 : 0x7a6);
    if (bytes[kState]&kContained) Assert("!isContained()",patch ? 0x7d5 : 0x7a7);
    const auto selectedBase=CurrentTypes();const auto& initial=TypeRecord(bytes[kType]); // 두 입구 진단 뒤 선택한 TYPE와 그 물리 기준을 저장한다.
    const bool cdServer=!patch && pool_.IsServer(); // CD는 zOrder 쓰기 직전에 읽은 서버 플래그를 유지한다.
    bytes[patch ? 35 : 33]=static_cast<std::uint8_t>(initial.zOrder);
    if (patch ? pool_.IsServer() : cdServer) {
        bytes[patch ? 34 : 32]=0;
        const bool needsHp=(TypeRecord(bytes[kType]).flags2&kDaisFlag) ? (bytes[patch ? 40 : 35]&1U)!=0 : (TypeRecord(bytes[kType]).flags1&TypeFlag1::kHasHitPoints)!=0;
        if (needsHp) {
            const auto hp=TypeHitPoints(initial,selectedBase);
            Put(bytes,kHp,static_cast<std::uint32_t>(hp),patch ? 4 : 2);
        }
    }
    if (bytes[kState]&kContained) Assert("!isContained()",patch ? 0x7df : 0x7b1); // 비서버도 마지막 현재 상태 진단을 생략하지 않는다.
}
// 두 판본 모두 Form 진단 복귀 뒤 현재 zOrder를 쓰고 contained를 마지막에 진단한다.
void SquidFactory::PostTake(Sid sid) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (pool_.Slot(sid)[kType]<kFirstAssetTypeNumber) Assert("!isForm()",patch ? 0x7f6 : 0x7c8);
    const auto zOrder=TypeRecord(pool_.Slot(sid)[kType]).zOrder; // 진단이 고친 현재 TYPE를 다시 읽어 BYTE로 저장한다.
    std::span(pool_.bytes_).subspan(pool_.Offset(sid),pool_.layout_.stride)[patch ? 35 : 33]=static_cast<std::uint8_t>(zOrder);
    if (pool_.Slot(sid)[kState]&kContained) Assert("!isContained()",patch ? 0x7fa : 0x7cc);
}
}
