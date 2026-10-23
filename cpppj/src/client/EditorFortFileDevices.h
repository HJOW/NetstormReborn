// 전체Editor부모를복제하지않고현재작업namespace의실제파일장치만빌린다.
#pragma once
#include "client/CampaignTemplateStorage.h"
#include "client/EditorFiles.h"
#include "o/BaseFile.h"
#include "o/ConfigInterface.h"

namespace netstorm::client {
// 현재Fort/고정FileSpec전역은기존상위소유자의조회/선택만사용한다.
struct EditorFortPathEffects {
    std::function<std::string_view()> currentName; //0055b490의현재NUL제외이름이다.pending/미션이름의사본으로대체하지않는다.
    std::function<void()> selectFileSpec; //005327e0이같은고정9BYTE FileSpec버퍼를가리키게한다.버퍼를fortSpec으로덮지않는다.
    std::function<std::string_view()> fileSpecKey; //선택한기존SoloMapFileSpec::Current등의같은현재키다.
    std::function<void(std::uint32_t)> assertion; //빈currentName의Fortspec.cpp39와stat실패Basefile.cpp진단이다.
};
// Client/Config/작업본보다짧게살고외곽호출이빌리는경로scratch만소유한다.
class EditorFortFileDevices {
public:
    EditorFortFileDevices(EditorFiles& work,const o::BaseFileSystem& source,o::ConfigInterface& config,EditorFortPathEffects effects); //세장기장치와실제현재이름효과를빌린다.생성만으로파일/manager를읽거나쓴적없다.
    EditorFortFileDevices(const EditorFortFileDevices&)=delete; //반환path뷰와bound callback의같은주소수명을복제하지않는다.
    EditorFortFileDevices& operator=(const EditorFortFileDevices&)=delete; //실제장치를다른owner참조로바꾸지않는다.
    std::string_view CurrentPath(); //00460680의현재이름진단/같은FileSpec선택/Config.PathSpec을실행한뒤작업namespace로해석한다.파일생성은하지않는다.
    std::string_view PrepareCurrent(); //호스트의명시작업본준비단계다.이미열리는작업파일은원본조차재읽지않고보존하며없는경우에만원본을읽어복사한다.
    std::int32_t Exists(std::string_view path) const; //작업파일의fopen rb성공5/실패0이다.서명파싱이나manager상태를조회하지않는다.
    bool ReadOnly(std::string_view path) const; //현재작업파일의CRT stat쓰기비트이며Config readOnlyForts를읽지않는다.
    std::vector<std::uint8_t> Read(std::string_view path) const; //기존BaseFileSystem::TryRead의절대경로몸체를재사용한다.읽기실패/빈파일은빈할당이다.
    TemplateFileLoadHooks BindLoad(TemplateFileLoadHooks hooks) const; //기존LoadFile의read/readOnly/실제CRT비교만채운다.나머지필수현재owner/TYPE/제목/진단은호출자가공급한다.
private:
    EditorFiles& work_; //원본/CD밖의유일한작업namespace와실제권한/쓰기장치다.
    const o::BaseFileSystem& source_; //원본/아카이브는읽기만하는기존파일시스템이다.준비뒤Read는검사된절대작업경로만넘긴다.
    o::ConfigInterface& config_; //동일Client의실제Config.PathSpec을그대로호출한다.
    EditorFortPathEffects effects_; //현재이름/키/선택/진단의상위실제저장소를조회한다.
    std::string configuredPath_; //마지막PathSpec결과scratch이며현재Fort이름owner가아니다.
    std::string namespacePath_; //기존BaseFileSystem과같이해석한작업경로다.상대게임경로만같은NormalizeGamePath를사용한다.
    std::string workingPath_; //다음CurrentPath/PrepareCurrent까지유효한검사된절대경로scratch다.
};
//0041aad0은현재헤더의첫DWORD비영만반환한다.토큰lookup/파일서명을추가검사하지않는다.
std::int32_t ExistingEditorTemplateValid(CampaignTemplateStorage& storage,std::uint32_t slot);
}
