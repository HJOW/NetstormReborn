"""동결된두준비본을합친final-pending만마무리하며제품/기존pending에는쓰지않는다."""
from pathlib import Path

OUT=Path(__file__).resolve().parent # 최종사본과인계만이폴더하위에기록한다.
FINAL=OUT/'final-pending' # 제품의직접수정대신root가검토할트리다.

# 이미완성된변경은중복삽입하지않고예상기준이없으면중단한다.
def change(text,before,after):
    """정확한조각1개를교체하고재실행시이미바뀐조각은유지한다."""
    if before==after:return text
    if after in text:return text
    if text.count(before)==1:return text.replace(before,after,1)
    raise ValueError('최종기준조각불일치:'+before[:120])

# final-pending의원래BOM/줄끝을유지한다.
def edit(name,operation):
    """후보파일만읽고Unicode변경함수의결과를같은사본에쓴다."""
    path=FINAL/name;raw=path.read_bytes();text=operation(raw.decode('utf-8-sig').replace('\r\n','\n'))
    if b'\r\n' in raw:text=text.replace('\n','\r\n')
    path.write_bytes((b'\xef\xbb\xbf' if raw.startswith(b'\xef\xbb\xbf') else b'')+text.encode('utf-8'))

# 한국어번역선택과확정입력문자보존을분리해English모드한글도HTML에서유지한다.
def client_display(text):
    """실제클론전역만유효확장문자를허용하고원문독립파서기본false는유지한다."""
    flag='    htmlWindowGlobals_->text.extendedText=true; // 클론의UTF8/BOM/IME표시토큰을모든UI언어에서보존한다. 번역선택과원문독립파서의기본false는별개다.'
    while flag+'\n'+flag in text:text=text.replace(flag+'\n'+flag,flag) # 중간저장재실행으로같은명시설정을중복삽입하지않는다.
    text=change(text,'    rendererFrame_=std::make_unique<ClientRendererFrame>(*this);',flag+'\n    rendererFrame_=std::make_unique<ClientRendererFrame>(*this);')
    return change(text,'htmlWindowGlobals_->text.extendedText=currentLanguage_==static_cast<std::int32_t>(kKoreanLanguageNumber); // 같은재사용HTML전역을현재언어로즉시갱신하며원문언어는기본BYTE관문을유지한다.','htmlWindowGlobals_->text.extendedText=true; // 같은재사용클론전역은언어변경뒤에도유효한표시4BYTE를보존한다. 영어원문파일선택/바이트와독립fixture의기본false는그대로다.')

# 기존본문이한글인경우도ASCII붙여넣기를물리BYTE상한으로검사한다.
def paste_limit(text):
    """순수BYTE기존분기는보존하고한글이있는삽입/본문만현대상한에연결한다."""
    return change(text,'if (HasExtendedDisplayText(incoming)) {','if (HasExtendedDisplayText(incoming) || HasExtendedDisplayText(Text())) { // 기존한글본문에긴ASCII를붙이는경우도최종BYTE상한을우회하지않는다.')

# 버퍼선택뿐아니라지연Shift기준도현재문자의실제경계로고정한다.
def shift_boundaries(text):
    """한글본문의stale앵커/End만교정하고원문순수BYTE계산은유지한다."""
    before='buffer.Select(static_cast<std::int32_t>(begin),static_cast<std::int32_t>(end)); }'
    after='buffer.Select(static_cast<std::int32_t>(begin),static_cast<std::int32_t>(end));node.shiftAnchor=static_cast<std::int32_t>(DisplayTextFloor(text,static_cast<std::size_t>(std::max(0,node.shiftAnchor))));node.shiftEnd=static_cast<std::int32_t>(DisplayTextFloor(text,static_cast<std::size_t>(std::max(0,node.shiftEnd)))); } // 외부선택뒤stale Shift기준도토큰내부를가리키지않는다.'
    text=change(text,before,after)
    text=change(text,'if (!(event&kShift)) { const auto at=Add(Buffer(token).State().length,1);Buffer(token).Select(at,at);','if (!(event&kShift)) { const auto at=extended ? Buffer(token).State().length:Add(Buffer(token).State().length,1);Buffer(token).Select(at,at);')
    return change(text,'node.shiftEnd=Add(Buffer(token).State().length,1);Buffer(token).Select(node.shiftAnchor,node.shiftEnd);','node.shiftEnd=extended ? Buffer(token).State().length:Add(Buffer(token).State().length,1);Buffer(token).Select(node.shiftAnchor,node.shiftEnd);')

# 큐에들어온언어알림이기존Main부모의라벨갱신을실제로요청하게한다.
def main_menu_refresh(text):
    """MainMenu에서만컨트롤을재배치하고일반World/Chat은재생성하지않는다."""
    return change(text,'std::erase_if(scripts_,[](const auto& item) { return item.first!=kProductCreditsObject; });rebuild_=true;return;','std::erase_if(scripts_,[](const auto& item) { return item.first!=kProductCreditsObject; });if (state_.phase==ClientPhase::MainMenu) RebuildControls();rebuild_=true;return;')

# 열린안내의등록이름/객체주소를지우지않고같은목록Config의언어내용만교체한다.
def menu_language_reload(text):
    """언어파일만같은객체에재적재하며프로필cfg/미션실행Config는유지한다."""
    before='void TellFileMenus::InvalidateLanguageFiles() { owned_.clear(); } // 다른mission/사용자Config는소유하지않으므로건드리지않는다.'
    after='''void TellFileMenus::InvalidateLanguageFiles() { // 현재안내의officalN.Overview참조가살아있도록같은등록객체를유지한다.
    const auto language=config_.Configuration().Get("currentLanguage").value_or(""); // 실제Client가확정한현재설정이며빈독립입력은다음수집에서재생성한다.
    if (language.empty()) { owned_.clear();return; }
    constexpr std::array<std::string_view,7> languages{"english","french","german","spanish","japanese","portuguese","korean"}; // 언어확장자만교체하며일반cfg/프로필파일명은바꾸지않는다.
    if (std::find(languages.begin(),languages.end(),language)==languages.end()) return;
    for (auto& object:owned_) { // 소유한목록Config만교체하고mission/사용자다른객체에는접근하지않는다.
        const auto path=object->GetRaw("path");if (!path) continue;const auto dot=path->find_last_of('.');if (dot==path->npos) continue;
        const auto previous=o::AsciiLower(std::string_view(*path).substr(dot+1));if (std::find(languages.begin(),languages.end(),previous)==languages.end()) continue;
        const auto selected=path->substr(0,dot+1)+language;if (selected==*path) continue;
        const auto name=object->GetRaw("fileName").value_or(ObjectName(*object)),index=object->GetRaw("index").value_or("0"); // prefix주소와원래index/파일이름은보존한다.
        object->Clear();object->Append("path="+selected);object->Append("fileName="+name);object->Append("index="+index);config_.LoadFile(*object,selected); // 실제Config FILE/본문해독만사용하며AI/SP실행을다시시작하지않는다.
    }
}'''
    text=change(text,before,after)
    while '#include <array>\n#include <array>' in text:text=text.replace('#include <array>\n#include <array>','#include <array>') # 이준비단위가추가한인접중복만정리한다.
    return change(text,'#include <algorithm>','#include <algorithm>\n#include <array>')

# 공개주석도파괴가아닌동일객체재적재정책을설명한다.
def menu_language_header(text):
    """언어내용교체중에도등록이름/표시링크를유지하는계약을기록한다."""
    return change(text,'같은언어전환알림에서소유한목록Config만반납해다음수집이현재언어본문/제목을읽게한다.','같은언어전환알림에서소유한목록Config의언어내용만재적재하고등록주소/index/열린안내링크는유지한다.')

# 고정44의원래사본은불변이며최종config사본의수동표시문장두개만번역한다.
def diagnostic_translation(text):
    """휴면외부진단문구를번역하되기존URL/키/HTML/서식/escape는보존한다."""
    text=change(text,'Please copy this information and <a href=`"mailto:bugs@titanic.com`">Email Us</a> with it.`n<p>`n<a href=`"{troubleshooting URL}`">Go to troubleshooting pages.</a><hr>(to stop this browser launch, set troubleshootOnCrash = 0 in your options.cfg file.)`n','이 정보를 복사하여 <a href=`"mailto:bugs@titanic.com`">이메일로 보내 주십시오</a>.`n<p>`n<a href=`"{troubleshooting URL}`">문제 해결 안내로 이동합니다.</a><hr>(이 브라우저 실행을 중지하려면 options.cfg 파일의 troubleshootOnCrash 값을 0으로 설정하십시오.)`n')
    return change(text,"An assertion has failed:`n'%s' in '%s', line %d version %d.`nPlease copy this information and mail it to bugs@titanic.com`n","단언에 실패했습니다:`n'%s', 파일 '%s', 줄 %d, 버전 %d.`n이 정보를 복사하여 bugs@titanic.com으로 보내 주십시오.`n")

# 새검사소스에남은경고/optional실패경계를구문검토가능하게정리한다.
def integration_tests(text):
    """실제파서검사에영어회귀를추가하고미검증예외역참조를제거한다."""
    text=change(text,'std::string KoreanHelpBody(std::string_view source,std::string_view topic) {','std::string KoreanHelpBody(std::string_view source,std::string_view topic,bool extended=true) {')
    text=change(text,'globals.extendedText=true;std::uint32_t info{};','globals.extendedText=extended;std::uint32_t info{};')
    text=change(text,'files.RegisterArchives();std::string language="english";','files.RegisterArchives();')
    text=change(text,'CHECK(mission.Section("A.")->find("사용자 안내")!=std::string::npos);','const auto body=mission.Section("A.");CHECK(body && body->find("사용자 안내")!=std::string::npos);')
    text=change(text,'const auto english=labels;menus.InvalidateLanguageFiles();labels.clear();','const auto english=labels;auto* registered=config.Registry().FindByName("offical1");CHECK(registered);config.Set("currentLanguage","korean");menus.InvalidateLanguageFiles();CHECK(config.Registry().FindByName("offical1")==registered);if (registered) CHECK(registered->Section("Overview"));labels.clear();')
    if 'KoreanIntegration_EnglishHelpBytesAndOutputStayIdentical' not in text:text+='''
TEST_CASE(KoreanIntegration_EnglishHelpBytesAndOutputStayIdentical) { // 실제클론표시허용이기존영문파일선택/출력을바꾸지않는다.
    using namespace netstorm::client;const auto repo=KoreanRepo();netstorm::o::BaseFileSystem files(repo/"assets/game-data");files.RegisterArchives();const auto raw=files.Read("d/help.english");const std::string english(raw.begin(),raw.end()); // 원본자료는읽기만한다.
    const auto localized=ReadLocalizedGameFile(files,{repo},"d/help.english");CHECK(localized && *localized==raw);CHECK(DecodeQuickHelpDisplaySource(english)==english);const std::regex anchor("<a name=\\"([^\\"]*)\\">");std::set<std::string> topics;
    for (std::sregex_iterator entry(english.begin(),english.end(),anchor),end;entry!=end;++entry) topics.insert(netstorm::o::AsciiLower((*entry)[1].str())); // 실제원문주제의중복/별칭은전체파서입력에남긴다.
    CHECK(topics.size()==128);
    for (const auto& topic:topics) CHECK(KoreanHelpBody(english,topic,false)==KoreanHelpBody(english,topic,true)); // defaultfalse오라클과클론true를동일한영문입력으로대조한다.
}
'''
    return text

# 독립편집장치를사용하여두정적검토누락을실제제품경로로재현한다.
def input_tests(text):
    """ASCII Paste/지연Shift선택의원자문자회귀를추가한다."""
    if 'KoreanInput_AsciiPasteIntoExtendedBodyHonorsFinalByteLimit' in text:return text
    return text+'''
TEST_CASE(KoreanInput_AsciiPasteIntoExtendedBodyHonorsFinalByteLimit) { // 기존본문이한글일때긴ASCII도물리상한을넘기지않는다.
    KoreanEntryScene scene;scene.node.maxLength=10;scene.Set("한");scene.Buffer().Select(4,4);scene.clipboard=std::string(255,'A');scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);
    CHECK(scene.Text()=="한AAAAA" && scene.Buffer().State().length==9);scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);CHECK(scene.Text()=="한AAAAA"); // 남은상한0이면본문/선택을자르지않는다.
    scene.Set("한글");scene.Buffer().Select(0,4);scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);CHECK(scene.Text()=="AAAAA글" && scene.Buffer().State().length==9); // 지워질선택을차감한최종길이다.
    scene.node.maxLength=5;scene.Set("한");scene.Buffer().Select(0,0);scene.entry->Handle(1,InputCode::kShift|0x2d0000,0,0);CHECK(scene.Text()=="한");
}

TEST_CASE(KoreanInput_StaleShiftAnchorsAlwaysEndOnWholeCharacters) { // 외부선택뒤Shift가부분한글을다시선택하지않는다.
    for (const std::uint32_t key:{0x250000U,0x270000U,0x230000U,0x240000U}) { // Left/Right/End/Home은같은원자문자경계를사용한다.
        KoreanEntryScene scene;scene.Set("한A");scene.Buffer().Select(0,4);scene.node.shiftAnchor=2;scene.node.shiftEnd=2;scene.entry->Handle(1,InputCode::kShift|key,0,0);
        const auto boundary=[](std::int32_t at) { return at==0 || at==4 || at==5; }; // 한글1자와ASCII1자의독립적인의미경계다.
        CHECK(boundary(scene.Buffer().State().selectionBegin) && boundary(scene.Buffer().State().selectionEnd));CHECK(boundary(scene.node.shiftAnchor) && boundary(scene.node.shiftEnd));
    }
}
'''

# Unicode형식이없을때의기존ASCII장치순서/할당플래그는독립장치로관측한다.
def clipboard_fallback_test(text):
    """새Unicode장치를설치해도영문복사/붙여넣기의기존경로가유지되는지검사한다."""
    if 'KoreanInput_UnicodeMissingUsesOriginalAsciiClipboardPath' in text:return text
    return text+'''
TEST_CASE(KoreanInput_UnicodeMissingUsesOriginalAsciiClipboardPath) { // Unicode부재/ASCII복사에서는CF_TEXT와원문할당순서를유지한다.
    TextClipboardDevices devices;std::string calls;std::array<char,16> memory{};const std::string source="ASCII";std::copy(source.begin(),source.end(),memory.begin());std::uint32_t flags{},allocated{},unicodeWrites{}; // 시스템대신명시된기존장치를관측한다.
    devices.readUnicode=[&]()->std::optional<std::u16string> { calls+='U';return {}; };devices.writeUnicode=[&](auto) { ++unicodeWrites; };
    devices.formatAvailable=[&](auto format) { CHECK(format==1);calls+='F';return 1U; };devices.open=[&](auto window) { CHECK(window==0);calls+='O';return 1U; };devices.get=[&](auto format) { CHECK(format==1);calls+='G';return std::uintptr_t{1}; };
    devices.lock=[&](auto handle) { CHECK(handle==1);calls+='L';return memory.data(); };devices.unlock=[&](auto) { calls+='N';return 1U; };devices.close=[&] { calls+='C';return 1U; };
    devices.empty=[&] { calls+='E';return 1U; };devices.allocate=[&](auto value,auto size) { flags=value;allocated=size;calls+='A';return std::uintptr_t{1}; };devices.set=[&](auto format,auto handle) { CHECK(format==1 && handle==1);calls+='S';return handle; };
    TextClipboard clipboard(std::move(devices));std::array<char,16> output{};clipboard.Read(output);CHECK(std::string(output.data())==source && calls=="UFOGLNC");
    calls.clear();const std::string copied="COPY";clipboard.Write(copied);CHECK(calls=="OEALNSC" && unicodeWrites==0 && flags==0x2000 && allocated==5);CHECK(std::string(memory.data())==copied);
}
'''

# CMake도제품이아닌최종사본에서만완전한등록안으로만든다.
def cmake_sources(text):
    """확정입력공통구현과Windows IMM연결을후보빌드에등록한다."""
    if 'client/DisplayTextInput.cpp' in text:return text
    return text+'\n# 한국어 확정입력/문자경계 공통부품과 실제 Windows IME 결과조회다.\ntarget_sources(netstorm PRIVATE client/DisplayTextInput.cpp client/DisplayTextInput.h)\ntarget_link_libraries(netstorm PUBLIC imm32)\n'

# 기존Nativefixture는변경하지않고새headless소스만두검사대상에등록한다.
def cmake_tests(text):
    """전체검사와기존한국어집중검사에같은새검사파일을추가한다."""
    if 'DisplayTextInputTests.cpp' in text:return text
    return text+'\n# 한국어 입력/본문의 창 없는 검사이며 기존 원문 fixture 파일은 그대로다.\ntarget_sources(netstorm_tests PRIVATE DisplayTextInputTests.cpp KoreanResourcesIntegrationTests.cpp)\ntarget_sources(netstorm_korean_text_tests PRIVATE DisplayTextInputTests.cpp KoreanResourcesIntegrationTests.cpp)\n'

# 실제cache생성장치를재사용하는추가검사만현재983사본의끝에붙인다.
def help_cache_tests(text):
    """영어성공cache가무효화뒤현재한국어FILE을다시읽고복귀하는지검사한다."""
    while '#include "o/OriginalText.h"\n#include "o/OriginalText.h"' in text:text=text.replace('#include "o/OriginalText.h"\n#include "o/OriginalText.h"','#include "o/OriginalText.h"') # 추가검사헤더의인접중복만제거한다.
    text=change(text,'#include "client/QuickHelpFileRead.h"','#include "client/QuickHelpFileRead.h"\n#include "o/OriginalText.h"')
    text=change(text,'    std::function<void(std::string_view)> fileError;', '    std::function<std::string()> fileSpec; // 언어cache추가검사만현재파일키를공급하며기존fixture의기본영문키는그대로다.\n    std::function<void(std::string_view)> fileError;')
    text=change(text,'return std::string("d/help.english"); };','return fileSpec ? fileSpec():std::string("d/help.english"); };')
    if 'KoreanIntegration_ActualHelpCacheReloadsAcrossLanguageInvalidation' in text:return text
    return text+'''
TEST_CASE(KoreanIntegration_ActualHelpCacheReloadsAcrossLanguageInvalidation) { // 같은실제창/cache와FILE해독을영어/한국어/영어순서로대조한다.
    using namespace netstorm::test::helpread;const auto root=Work();const std::string english="<h2>English Help</h2>";Text(root/"d/help.english",english);Text(root/"d/help.korean","\\xef\\xbb\\xbf<h2>한국어 도움말</h2>");netstorm::client::Client instance(Options(root));WindowFixtureScene scene;scene.state.helpData.reset();std::string selected="d/help.english";std::size_t reads{}; // fixture기대값을바꾸지않는새독립입력이다.
    scene.fileSpec=[&] { return selected; };scene.fileRead=[&](auto name) { ++reads;return ReadClientQuickHelpFile(instance,name); };scene.help->Construct(kWindow,0);CHECK(scene.state.helpData==english+"\\n" && reads==1);
    selected="d/help.korean";scene.allocations.clear();scene.help->Construct(kWindow,0);CHECK(scene.state.helpData==english+"\\n" && reads==1); // 기존성공cache는명시무효화전까지유지된다.
    scene.help->InvalidateCache();CHECK(!scene.state.helpData);scene.allocations.clear();scene.help->Construct(kWindow,0);CHECK(scene.state.helpData && reads==2);
    if (scene.state.helpData) CHECK(netstorm::o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(scene.state.helpData->data()),scene.state.helpData->size()})=="<h2>한국어 도움말</h2>\\n");
    selected="d/help.english";scene.help->InvalidateCache();scene.allocations.clear();scene.help->Construct(kWindow,0);CHECK(scene.state.helpData==english+"\\n" && reads==3);CHECK(netstorm::o::ReadFileBytes(root/"d/help.english")==std::vector<std::uint8_t>(english.begin(),english.end()));
}
'''

def main():
    """제품/빌드/게임을건드리지않고최종트리의필수마무리만적용한다."""
    tasks={'cpppj/src/client/ClientMain.cpp':client_display,'cpppj/src/client/TextEntryBuffer.cpp':paste_limit,'cpppj/src/client/TextEntryGump.cpp':shift_boundaries,'cpppj/src/client/UberGump.cpp':main_menu_refresh,'cpppj/src/client/TellFileMenus.cpp':menu_language_reload,'cpppj/src/client/TellFileMenus.h':menu_language_header,'assets/lang/korean/d/config.korean':diagnostic_translation,'cpppj/tests/KoreanResourcesIntegrationTests.cpp':integration_tests,'cpppj/tests/DisplayTextInputTests.cpp':lambda text:clipboard_fallback_test(input_tests(text)),'cpppj/tests/QuickHelpWindowTests.cpp':help_cache_tests,'cpppj/src/CMakeLists.txt':cmake_sources,'cpppj/tests/CMakeLists.txt':cmake_tests}
    for name,operation in tasks.items():edit(name,operation) # 기존두pending과그기준파일은읽거나쓰지않는다.
    print('final-pending 마무리 '+str(len(tasks))+'파일 저장; 빌드/실행 0')

if __name__=='__main__':
    main()
