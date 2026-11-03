// 실제 읽기전용 SHP 매핑의 공유 수명·실패 반납·VFS 경계와 원본 자산을 검사한다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "FileProbeTestSupport.h"
#include "client/GameAssets.h"
#include "client/NativeShapeFile.h"
#include "o/BaseFile.h"
#include <optional>
#include <utility>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::fileprobe;
namespace { // 보호원본은읽기만하며쓰기/반납검사는독립scratch파일에한정한다.
std::vector<std::uint8_t> TinyShape(std::uint8_t color) { // 한1×1 VFX프레임과명시색의독립파일입력이다.
    std::vector<std::uint8_t> bytes(43);netstorm::test::rawscene::Put(bytes,0,0x30312e31);netstorm::test::rawscene::Put(bytes,4,1);netstorm::test::rawscene::Put(bytes,8,16);bytes[40]=2;bytes[41]=color;return bytes;
}
bool Writable(const std::filesystem::path& path) { // 파일 내용을쓰지않고scratch의쓰기핸들공유허용만검사한다.
    const auto file=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if (file==INVALID_HANDLE_VALUE) return false;CloseHandle(file);return true;
}
template<class Operation> bool Failed(Operation operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } // 실패를빈매핑성공으로바꾸지않는다.
void Color(const ShapeDatabase& shapes,std::uint8_t color) { const auto image=shapes.Decode(0,0);CHECK(image.width==1 && image.height==1 && image.indices==std::vector<std::uint8_t>{color} && image.opacity==std::vector<std::uint8_t>{255}); } // 명시입력픽셀을실제기존RLE파서로확인한다.
}

TEST_CASE(NativeShapeFile_SharedViewSurvivesCopiesMovesAndLastOwner) { // vector복사대신같은view를공유하고마지막반납뒤쓰기/삭제가가능해야한다.
    const auto root=Work()/"mapped-shape-791",path=root/"d/tiny.shp";const auto bytes=TinyShape(9);Write(path,bytes);BaseFileSystem files(root);std::optional<ShapeDatabase> retained;const std::uint8_t* pointer{};
    {
        auto original=LoadNativeShapeDatabase(files,root,"tiny.shp");pointer=original.Bytes().data();retained=original;auto moved=std::move(original);CHECK(original.Bytes().empty() && retained->Bytes().data()==pointer && moved.Bytes().data()==pointer);Color(*retained,9);CHECK(!Writable(path));
        ShapeDatabase assigned(TinyShape(11));assigned=std::move(moved);CHECK(moved.Bytes().empty() && assigned.Bytes().data()==pointer);Color(assigned,9);*retained=assigned;CHECK(retained->Bytes().data()==pointer);
    }
    Color(*retained,9);CHECK(retained->Bytes().data()==pointer && !Writable(path));retained.reset();CHECK(Writable(path) && std::filesystem::remove(path)); // 이단일삭제는검사가생성한scratch파일에만수행한다.
    CHECK(Failed([&] { ShapeDatabase invalid(bytes,{}); })); // 빌린물리만있고수명owner가없는입구는거부한다.
}

TEST_CASE(NativeShapeFile_FailedMapsAndParserReleaseTheirOwnResources) { // 열기/매핑/파서실패가다른정상view의수명에영향을주지않는다.
    const auto root=Work()/"mapped-failure-791";Write(root/"d/live.shp",TinyShape(13));Write(root/"d/empty.shp",{});const std::array<std::uint8_t,3> bad{1,2,3};Write(root/"d/bad.shp",bad);BaseFileSystem files(root);auto live=LoadNativeShapeDatabase(files,root,"live.shp");
    CHECK(Failed([&] { static_cast<void>(LoadNativeShapeDatabase(files,root,"missing.shp")); }));CHECK(Failed([&] { static_cast<void>(LoadNativeShapeDatabase(files,root,"empty.shp")); }) && Writable(root/"d/empty.shp"));CHECK(Failed([&] { static_cast<void>(LoadNativeShapeDatabase(files,root,"bad.shp")); }) && Writable(root/"d/bad.shp"));Color(live,13);CHECK(!Writable(root/"d/live.shp"));
    const auto archive=Taff("d/archive_only.shp",TinyShape(17));Write(root/"only.tarc",archive);CHECK(files.RegisterArchive(root/"only.tarc"));CHECK(Failed([&] { static_cast<void>(LoadNativeShapeDatabase(files,root,"archive_only.shp")); }) && !std::filesystem::exists(root/"d/archive_only.shp") && ReadFileBytes(root/"only.tarc")==archive); // TAFF일치를추출/whole-read 성공으로바꾸지않는다.
}

TEST_CASE(NativeShapeFile_UnicodeRootsAndArchiveSelectionKeepDirectMapBoundary) { // 기본disk→기본TAFF→보조disk선택과직접map의의미를보존한다.
    const auto root=Work()/"mapped-roots-791",primary=root/std::filesystem::path(u8"기본"),secondary=root/std::filesystem::path(u8"보조");Write(primary/"d/choice.shp",TinyShape(19));Write(secondary/"d/choice.shp",TinyShape(23));BaseFileSystem files(primary,secondary);
    { const auto shape=LoadNativeShapeDatabase(files,primary,"choice.shp");Color(shape,19); }
    CHECK(std::filesystem::remove(primary/"d/choice.shp"));{ const auto shape=LoadNativeShapeDatabase(files,primary,"choice.shp");Color(shape,23); }
    Write(primary/"first.tarc",Taff("d/choice.shp",TinyShape(29)));CHECK(files.RegisterArchive(primary/"first.tarc"));CHECK(Failed([&] { static_cast<void>(LoadNativeShapeDatabase(files,primary,"choice.shp")); })); // 기본TAFF는원래이름을반환하므로누락된기본disk의직접map을보조성공으로대체하지않는다.
}

TEST_CASE(NativeShapeFile_ActualOriginalGameAssetsMatchLegacyStorage) { // 보호원본의실제5618120BYTE와프레임/픽셀을읽기만대조한다.
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";BaseFileSystem files(root);files.RegisterArchives();GameAssets legacy(files,OriginalEdition::Patch1078);bool called{};const std::uint8_t* pointer{};
    GameAssets mapped(files,OriginalEdition::Patch1078,GameAssets::kDefaultPalettePath,{},[&] { called=true;auto shapes=LoadNativeShapeDatabase(files,root);pointer=shapes.Bytes().data();return shapes; });const auto& a=legacy.Shapes();const auto& b=mapped.Shapes();CHECK(called && b.Bytes().data()==pointer && a.Bytes().size()==5618120 && a.Bytes().size()==b.Bytes().size() && std::equal(a.Bytes().begin(),a.Bytes().end(),b.Bytes().begin()) && a.Blocks().size()==b.Blocks().size() && mapped.Types().size()==legacy.Types().size());
    bool decoded{};for (std::size_t block=0;block<a.Blocks().size();++block) { // 모든프레임수와일반프레임하나의실제그림을비교하며게임행동은재현하지않는다.
        CHECK(a.Blocks()[block].frames.size()==b.Blocks()[block].frames.size());if (decoded) continue;
        for (std::size_t frame=0;frame<a.Blocks()[block].frames.size();++frame) { if (a.Blocks()[block].frames[frame].IsSpecial()) continue;const auto left=a.Decode(block,frame),right=b.Decode(block,frame);CHECK(left.width==right.width && left.height==right.height && left.indices==right.indices && left.opacity==right.opacity);decoded=true;break; } // 실제기존파서를두backing에동일적용한다.
    }
    CHECK(decoded); // 살아있는원본매핑에대한쓰기/삭제/파일변경검사는하지않는다.
}
