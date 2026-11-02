// 글꼴 저장소 재생성 때 Renderer의 주소 기반 압축 해제 캐시가 이전 글리프를 내보내지 않는지 검사한다.
#include "TestSupport.h"
#include "client/BitmapFont.h"
#include "client/PresentationCommandInput.h"
#include "client/Renderer.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace { // 파일·메모리만 사용하는 글꼴 그림 검사의 독립 입력 작성기다.
using namespace netstorm::client; // 실제 FontStore와 Renderer의 공개 경계를 같은 검사에서 연결한다.

// 독립 VFX/CHFNT 입력의 정수를 낮은 바이트부터 기록한다.
void CachePut(std::vector<std::uint8_t>& bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    for (std::size_t i=0;i<width;++i) bytes.at(offset+i)=static_cast<std::uint8_t>(value>>(8*i)); // 지정한 필드 폭만 쓴다.
}

// 두 픽셀 중 한 곳만 불투명하게 만들어 폭은 같고 그림은 다른 두 글리프를 만든다.
std::vector<std::uint8_t> CacheShape(bool right,std::uint8_t color=100) {
    std::vector<std::uint8_t> bytes(40);
    CachePut(bytes,0,0x30312e31);CachePut(bytes,4,1);CachePut(bytes,8,16);
    CachePut(bytes,16,0,2);CachePut(bytes,18,1,2); // 원문 상자 크기는 높이-1/폭-1이다.
    CachePut(bytes,24,0);CachePut(bytes,28,0);CachePut(bytes,32,1);CachePut(bytes,36,0);
    if (right) bytes.insert(bytes.end(),{1,1,2,color,0}); // 투명1, 불투명1, 행 끝이다.
    else bytes.insert(bytes.end(),{2,color,1,1,0}); // 불투명1, 투명1, 행 끝이다.
    return bytes;
}

// 실제 BitmapFont 판독기에 A 하나만 가진 정상 CHFNT를 제공한다. 두 그림 모두 전진 폭은3이다.
std::vector<std::uint8_t> CacheFont(bool right) {
    constexpr std::string_view signature{"BitmapFontData\x1a\0",16}; // 원문16BYTE 글꼴 서명이다.
    constexpr std::size_t code='A',table=36,payload=4132; // 실제 네 표와 payload의 독립 파일 오프셋이다.
    auto shape=CacheShape(right);std::vector<std::uint8_t> bytes(payload);
    std::copy(signature.begin(),signature.end(),bytes.begin());CachePut(bytes,16,1);
    CachePut(bytes,20,14);CachePut(bytes,24,10);CachePut(bytes,28,4);CachePut(bytes,32,256);
    CachePut(bytes,table+code*4,3);CachePut(bytes,table+2048+code*4,3);
    CachePut(bytes,table+3072+code*4,static_cast<std::uint32_t>(shape.size()));
    bytes.insert(bytes.end(),shape.begin(),shape.end());return bytes;
}

// 이번 검사에서 새로 확보한 디렉터리만 소유하고 마지막에 반납한다.
struct CacheDirectory {
    std::filesystem::path root; // 실제 원본 자료와 떨어진 임시 입력 루트이며 생성 성공한 경로만 저장한다.
    // 기존 디렉터리를 재사용하지 않아 다른 검사의 입력이나 사용자 파일을 덮어쓰지 않는다.
    CacheDirectory() {
        const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt=0;attempt<128;++attempt) { // 동시에 실행된 검사와 충돌하면 새 접미사만 시도한다.
            auto candidate=std::filesystem::temp_directory_path()/("netstorm-font-cache-764-"+std::to_string(stamp)+"-"+std::to_string(attempt));
            if (std::filesystem::create_directory(candidate)) { root=std::move(candidate);return; }
        }
        throw std::runtime_error("글꼴 캐시 검사용 임시 디렉터리 확보 실패");
    }
    CacheDirectory(const CacheDirectory&)=delete; // 디렉터리 반납 소유권을 복제하지 않는다.
    CacheDirectory& operator=(const CacheDirectory&)=delete; // 기존 소유권을 덮어쓰지 않는다.
    // 생성에 성공해 소유한 루트만 지우며 검사 실패 전파 중에는 정리 예외를 던지지 않는다.
    ~CacheDirectory() { std::error_code error;if (!root.empty()) std::filesystem::remove_all(root,error); }
};

// GDI fallback 없이 실제 Rebuild의18개 슬롯/스타일 파일을 모두 공급한다.
void CacheWriteFace(const std::filesystem::path& root,std::string_view face,bool right) {
    constexpr std::array<int,7> heights{14,13,0,20,48,14,12},weights{700,0,0,700,0,0,0}; // 원문 요청 슬롯의 높이/굵기다.
    constexpr std::array<std::string_view,5> styles{"normal","italic","bold","strikeout","underline"}; // FontStyle의 파일 이름 순서다.
    std::filesystem::create_directory(root/"d");const auto bytes=CacheFont(right);
    for (int slot=0;slot<7;++slot) { // 원문이 생성하는 유효 슬롯만 같은 파일 이름으로 작성한다.
        for (std::size_t style=0;style<styles.size();++style) { // 비어 있는 스타일은 GDI 입력으로 만들지 않는다.
            if (!FontStore::Available(slot,static_cast<FontStyle>(style))) continue;
            const auto selected=slot==1 ? std::string("Courier New") : std::string(face);
            const auto path=root/"d"/("!"+selected+"."+std::string(styles[style])+"."+std::to_string(heights[slot])+"."+std::to_string(weights[slot])+".chfnt");
            std::ofstream output(path,std::ios::binary|std::ios::trunc);
            output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
            if (!output) throw std::runtime_error("글꼴 캐시 검사용 CHFNT 작성 실패");
        }
    }
}

// 논리 표면의 변경 영역을 실제 합성하고 창 장치 없이 Present까지 완료한다.
void CacheDraw(Renderer& renderer,std::vector<std::uint8_t>& pixels) {
    CHECK(!renderer.Draw(pixels,8).empty());renderer.Present([](ScreenRect) {});
}

// 같은 전진 폭의 A를 색77로 화면(1,1)에 그리도록 실제 RenderText를 교체한다.
void CacheText(Renderer& renderer,const BitmapFont& font) {
    RenderText text;text.font=&font;text.bytes="A";text.x=1;text.y=1;text.color=77;
    renderer.SetScene({}, {std::move(text)});
}
}

// 일반 dirty/장면 교체는 캐시를 지우지 않으며 명시 반납 뒤에만 같은 주소의 새 VFX를 읽는다.
TEST_CASE(Renderer_DecodedImagesRetireExplicitlyAtShapeAddressReuse) {
    std::optional<netstorm::client::ShapeDatabase> storage(std::in_place,CacheShape(false,5));
    auto* const original=&*storage;netstorm::client::Renderer renderer(8,4);std::vector<std::uint8_t> pixels(32);
    netstorm::client::RenderSprite sprite;sprite.database=original;sprite.x=1;sprite.y=1;sprite.clip={0,0,8,4};
    renderer.SetScene({sprite});CacheDraw(renderer,pixels);CHECK(pixels[9]==5 && pixels[10]==0);
    renderer.SetScene({});storage.reset();storage.emplace(CacheShape(true,7));
    CHECK(&*storage==original);sprite.database=&*storage;renderer.SetScene({sprite});renderer.InvalidateAll();CacheDraw(renderer,pixels);
    CHECK(pixels[9]==5 && pixels[10]==0); // 주소 재사용을 알리기 전의 캐시를 확인하여 매 프레임 clear가 추가되는 것도 잡는다.
    renderer.ClearDecodedImages();renderer.InvalidateAll();CacheDraw(renderer,pixels);
    CHECK(pixels[9]==0 && pixels[10]==7);
}

// 실제 FontStore 재생성 부모가 반납을 호출하지 않으면 옛 주소의 왼쪽 A가 그대로 남는 회귀다.
TEST_CASE(PresentationFontRebuild_ActualFontStoreDrawsNewGlyphAtReusedAddress) {
    CacheDirectory directory;CacheWriteFace(directory.root,"Arial",false);CacheWriteFace(directory.root,"NextFace",true);
    netstorm::o::BaseFileSystem files(directory.root);
    FontStore fonts(files,"Arial",[](const auto& visit) { visit("Arial",4,0);visit("NextFace",4,0); }); // 두 번 열거하는 실제 목록 부모에 같은 독립 입력을 준다.
    Renderer renderer(8,4);std::vector<std::uint8_t> pixels(32);
    const auto& before=fonts.Get();CHECK(before.Measure("A")==3);CacheText(renderer,before);CacheDraw(renderer,pixels);
    CHECK(pixels[9]==77 && pixels[10]==0);
    renderer.SetScene({}); // 이전 글꼴을 빌리는 장면을 소유자 반납보다 먼저 제거한다.
    auto& oldGlyph=const_cast<FontGlyph&>(before.Glyph('A')); // 실제 객체는 mutable이며 검사에서만 글리프 소유권 이동을 제어한다.
    auto retained=std::move(oldGlyph.shape);auto* const reused=retained.get();CHECK(reused!=nullptr);
    if (!retained) return; // CHECK가 비중단형이어도 실패를 nullptr 접근으로 바꾸지 않는다.
    RebuildPresentationFonts(fonts,renderer,fonts.FaceAt(1)); // UberGump의 실제 hook과 같은 부모 몸체를 호출한다.
    CHECK(fonts.CurrentFace()=="NextFace");const auto& after=fonts.Get();CHECK(after.Measure("A")==3);
    auto& newGlyph=const_cast<FontGlyph&>(after.Glyph('A'));CHECK(newGlyph.shape!=nullptr);if (!newGlyph.shape) return;
    *retained=std::move(*newGlyph.shape);newGlyph.shape=std::move(retained); // 실제 새 CHFNT의 자료를 이전 할당 주소로 옮겨 주소 재사용을 결정적으로 강제한다.
    CHECK(newGlyph.shape.get()==reused);CacheText(renderer,after);CacheDraw(renderer,pixels);
    CHECK(pixels[9]==0 && pixels[10]==77); // 폭이 같아도 실제 새 글리프의 오른쪽 픽셀만 보여야 한다.
}
