// 복원된 Kencloud/팔레트 본체에 실제 물리/파일/설정 장치를 연결하고 별도 난수/구름 사본을 만들지 않는다.
#include "client/ClientKencloudGlobals.h"
#include "client/CurrentAllocationBytes.h"
#include "client/GifImage.h"
#include "o/BaseFile.h"
#include "o/ConfigInterface.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace netstorm::client { // 같은 고정 전역과 현재 할당을 동기 원문 부모에 제공한다.
namespace { // 경로/치수의 실제 읽기 위치에서만 물리를 검사한다.
std::string_view Text(std::span<const std::uint8_t> bytes) { const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("구름 현재 C문자열 물리 범위");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())}; } // 같은 NUL 입력을 복제하지 않고 빌린다.
void Path(std::vector<std::uint8_t>& target,const std::string& value) { target.assign(value.begin(),value.end());target.push_back(0); } // PathSpec 반환의 수명을 동기 부모 끝까지 유지한다.
std::pair<std::int32_t,std::int32_t> Dimensions(std::span<const std::uint8_t> bytes) {
    if (bytes.size()<13) throw std::out_of_range("구름 GIF 헤더 물리 범위");const std::size_t descriptor=13+((bytes[10]&0x80) ? (3U<<((bytes[10]&7)+1)) : 0U); //00407171의 실제 첫 descriptor 위치다.
    if (descriptor>bytes.size() || bytes.size()-descriptor<10 || bytes[descriptor]!=0x2c) throw std::runtime_error("구름 GIF 첫 descriptor 누락");
    const auto word=[&](auto offset) { return std::int32_t(bytes[descriptor+offset])|(std::int32_t(bytes[descriptor+offset+1])<<8); };return {word(5),word(7)}; // Native wrapper가 decoder 호출 전에 쓰는 실제 폭/높이다.
}
}
ClientKencloudGlobals::ClientKencloudGlobals(std::function<void(std::span<std::uint8_t>)> allocation):allocation_(allocation ? std::move(allocation) : CurrentAllocationBytes) {} // 현재 할당 입력만 연결한다.
o::KencloudGlobals ClientKencloudGlobals::Globals() { return {state_[515],state_[512],state_[514],state_[513],state_[2053],state_[2051],state_[2052],options_[3],options_[6],options_[1],options_[2],options_[5],rows_,std::span(state_).subspan(516,512),std::span(state_).subspan(1028,1000)}; } // 같은 packed 전역의 직접 참조다.
o::FortPaletteTransitionGlobals ClientKencloudGlobals::PaletteGlobals() { return {paletteName_,CloudName(),state_[515],options_[1],rendererToken_,redraw_}; } // Native 두 부모가 같은 cache/speed/name을 공유한다.
std::span<std::uint32_t> ClientKencloudGlobals::State() { return state_; } // 물리 쓰기/검사용 같은 전체 범위다.
std::span<const std::uint32_t> ClientKencloudGlobals::State() const { return state_; } // 전체 현재 물리를 복제하지 않는다.
std::uint32_t& ClientKencloudGlobals::Value(Option option) { return options_.at(static_cast<std::size_t>(option)); } // 같은 옵션 한 DWORD만 빌린다.
const std::uint32_t& ClientKencloudGlobals::Value(Option option) const { return options_.at(static_cast<std::size_t>(option)); } // 읽기도 같은 주소다.
std::span<const std::uint32_t,16> ClientKencloudGlobals::PhaseRows() const { return rows_; } // 현재 고정 행표를 직접 제공한다.
std::span<std::uint8_t> ClientKencloudGlobals::PaletteName() { return paletteName_; } // 명시 Native 부모만 이름을 쓴다.
std::span<std::uint8_t> ClientKencloudGlobals::CloudName() { return std::span<std::uint8_t>(reinterpret_cast<std::uint8_t*>(state_.data()),sizeof(state_)).subspan(8112); } // 뒤의 실제 flags와 같은 인접 물리다.
void ClientKencloudGlobals::ReadOptions(o::ConfigInterface& config) {
    const auto read=[&](auto key,std::uint32_t& field) { auto value=std::bit_cast<std::int32_t>(field);if (config.ReadInt(key,value)) field=std::bit_cast<std::uint32_t>(value); }; // 누락은 callback 뒤의 현재 필드까지 보존하며 실제 성공 store만 적용한다.
    read("cloudiness",options_[0]);read("cloudSpeed",options_[1]);read("cloudFury",options_[2]);read("cloudSpacer",options_[3]);read("cloudRecenter",options_[4]);read("cloudAlgo",state_[2050]);read("kenCloud",state_[2049]);
}
std::uint32_t ClientKencloudGlobals::Allocate(std::uint32_t bytes) {
    if (!bytes) throw std::out_of_range("구름 할당의 실제 물리 크기0");const auto extent=(std::uint64_t(bytes)+0xfffU)&~std::uint64_t(0xfffU);if (std::uint64_t(nextAddress_)+extent+0x1000U>std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("구름 DWORD 주소 공간 소진");
    std::vector<std::uint8_t> block(bytes);allocation_(block);const auto token=nextAddress_;blocks_.emplace(token,std::move(block));nextAddress_+=static_cast<std::uint32_t>(extent+0x1000U);return token; // 반환한 같은 블록만 Native cache store에 공개된다.
}
std::span<std::uint8_t> ClientKencloudGlobals::Memory(std::uint32_t address) {
    auto found=blocks_.upper_bound(address);if (found==blocks_.begin()) throw std::out_of_range("구름 현재 주소 물리 누락");--found;const auto offset=std::uint64_t(address)-found->first;if (offset>found->second.size()) throw std::out_of_range("구름 현재 주소 물리 범위");return std::span(found->second).subspan(static_cast<std::size_t>(offset)); // 마지막0BYTE와 내부 행 주소를 같은 할당에서 빌린다.
}
void ClientKencloudGlobals::Release(std::uint32_t token) { if (!token) return;if (blocks_.erase(token)!=1) throw std::out_of_range("구름 실제 반납 base 누락"); } // current cache 대입은 원문 부모에 남긴다.
std::size_t ClientKencloudGlobals::Allocations() const { return blocks_.size(); } // 실제 생존 표의 관측이다.
std::uint32_t ClientKencloudGlobals::LoadFile(const o::BaseFileSystem& files,o::ConfigInterface& config) {
    o::KencloudFileScratch scratch;allocation_({reinterpret_cast<std::uint8_t*>(&scratch),sizeof(scratch)});static_assert(sizeof(scratch)==264); // 원문 지역 입력을 임의0으로 만들지 않는다.
    o::KencloudFileHooks hooks;hooks.pathSpec=[&](auto key,auto first,auto second,auto third) { if (first || second || third) throw std::logic_error("구름 실제 FileSpec 인자 계약");Path(filePath_,config.PathSpec(key));return std::span<const std::uint8_t>(filePath_); };
    hooks.exists=[&](auto path) { return files.TryRead(Text(path)).has_value() ? 1U : 0U; }; // 실제 VFS 결과만 사용하며 decoder 오류는 존재 실패로 바꾸지 않는다.
    hooks.decode=[&](auto path,auto& width,auto& height) { const auto bytes=files.Read(Text(path));const auto dimensions=Dimensions(bytes);width=dimensions.first;height=dimensions.second; // 폭/높이 store는 실제 픽셀 해독보다 앞이다.
        const auto size=std::uint64_t(static_cast<std::uint32_t>(width))*static_cast<std::uint32_t>(height);if (!size || size>16U*1024U*1024U) throw std::out_of_range("구름 GIF 실제 할당 폭");const auto token=Allocate(static_cast<std::uint32_t>(size));const auto image=DecodeBackgroundGif(bytes);if (image.width!=std::size_t(width) || image.height!=std::size_t(height) || image.indices.size()!=size) throw std::logic_error("구름 Native descriptor/decoder 치수 불일치");std::copy(image.indices.begin(),image.indices.end(),Memory(token).begin());return token; };
    hooks.release=[this](auto token) { Release(token); };return o::LoadKencloudFile(scratch,hooks); //512판정/잘못된 이미지 반납은 기존 Native 부모가 수행한다.
}
o::KencloudHooks ClientKencloudGlobals::Devices(const o::BaseFileSystem& files,o::ConfigInterface& config) { o::KencloudHooks hooks;hooks.loadFile=[&] { return LoadFile(files,config); };hooks.allocate=[this](auto bytes) { return Allocate(bytes); };hooks.memory=[this](auto address) { return Memory(address); };return hooks; } // 같은 동기 호출보다 오래 빌리지 않는다.
void ClientKencloudGlobals::Build(const o::BaseFileSystem& files,o::ConfigInterface& config,o::GameRandom& random) { o::BuildKencloud(Globals(),random,Devices(files,config)); } // 실제 같은 GameRandom을 직접 소비한다.
void ClientKencloudGlobals::Step() { o::KencloudHooks hooks;hooks.memory=[this](auto address) { return Memory(address); };o::StepKencloud(Globals(),hooks); } // 행 이동은 별도 난수를 만들지 않는다.
void ClientKencloudGlobals::Transition(const std::span<const std::uint8_t>& name,const o::BaseFileSystem& files,o::ConfigInterface& config,o::GameRandom& random,o::FortPaletteTransitionHooks devices) {
    devices.compare=[](auto first,auto second) { Text(first);Text(second);return _stricmp(reinterpret_cast<const char*>(first.data()),reinterpret_cast<const char*>(second.data())); }; // 실제 CRT의 대소문자 무시 비교와 같은 현재 C문자열을 사용한다.
    devices.release=[this](auto token) { Release(token); };devices.buildCloud=[&] { Build(files,config,random); };devices.stepCloud=[this] { Step(); };
    devices.pathSpec=[&](auto key,auto input,auto second,auto third) { if (second || third) throw std::logic_error("팔레트 실제 FileSpec 인자 계약");Path(palettePath_,config.PathSpec(key,Text(input)));return std::span<const std::uint8_t>(palettePath_); };o::TransitionFortPalette(name,PaletteGlobals(),devices); // 이름/반납/cache0/생성/이동/Screen/표/dirty의 원문 순서를 보존한다.
}
void ClientKencloudGlobals::BindRenderer(Renderer* renderer) { if (renderer==renderer_) return;renderer_=renderer;if (!renderer) { rendererToken_=0;return; }if (nextRendererToken_>0xffffe000U) throw std::overflow_error("현재 Renderer 주소 토큰 소진");rendererToken_=nextRendererToken_;nextRendererToken_+=0x1000U; } // 호스트 포인터를 DWORD로 자르지 않는다.
const std::uint32_t& ClientKencloudGlobals::RendererToken() const { return rendererToken_; } // 현재 동일 장치의 opaque32 값이다.
std::uint32_t& ClientKencloudGlobals::WorldRedraw() { return redraw_; } // 같은 Native0059a8b0을 직접 빌린다.
}
