#!/usr/bin/env python3
"""현재제품을읽기만하여공통SNART/공유owner/빈Image선행API의좁은work패치를만든다."""
import difflib
import hashlib
import json
from pathlib import Path

# 이work안에서만기준복사본과제안파일을만든다.실제제품경로에는쓰지않는다.
WORK=Path(__file__).resolve().parents[1]
ROOT=WORK.parents[2]


def replace_once(text,before,after):
    """조사한현행hunk가정확히한번있는지확인하며원문변경을조용히덮어쓰지않는다."""
    if text.count(before)!=1:
        raise RuntimeError('기준hunk개수불일치:'+before[:100])
    return text.replace(before,after,1)


def main():
    """현재세제품파일의기준SHA와좁은diff를독립저장한다.제품적용은하지않는다."""
    names=['cpppj/src/o/RawDataManagerImage.cpp','cpppj/src/client/CampaignTemplateStorage.h','cpppj/src/client/CampaignTemplateStorage.cpp']
    sources={name:(ROOT/name).read_text(encoding='utf-8') for name in names}
    outputs=dict(sources)
    source=outputs[names[0]]
    source=replace_once(source,'#include "o/RawDataManagerImage.h"','#include "o/RawDataManagerImage.h"\n#include "o/RawSnart.h"')
    start=source.index('std::string_view Name(std::span<std::uint8_t> names,')
    end=source.index('\n}\n}',start)+2
    source=source[:start]+'''std::string_view Name(std::span<std::uint8_t> names,std::uint32_t index,const DataManagerImageHooks& hooks) {
    return ReadSnartName(names,index,[&hooks] { Assert(hooks,DataManagerImageAssertion::NameSeparator); }); // 동일몸체를공유하며기존reader의현재strlen시점은보존한다.
}'''+source[end:]
    outputs[names[0]]=source
    source=outputs[names[1]]
    source=replace_once(source,'#include "o/RawFortCurrentName.h"','#include "o/RawFortCurrentName.h"\n#include "o/RawDataManagerInitialize.h"\n#include "client/ClientTemplateSectionNames.h"')
    source=replace_once(source,'explicit CampaignTemplateStorage(std::shared_ptr<o::FortSerializationGlobals> serializationGlobals={});','explicit CampaignTemplateStorage(std::shared_ptr<o::FortSerializationGlobals> serializationGlobals={},std::shared_ptr<ClientTemplateSectionNames> sectionNames={});')
    marker='    std::uint32_t& SerializationVersion();'
    source=replace_once(source,marker,'''    std::uint32_t AllocateEmptyManager(std::uint32_t bytes); // 원문new276의실제미생성Image를소유표에추가하고불투명token/0을돌려준다.header는쓰지않는다.
    std::uint32_t ConstructEmptyManager(std::uint32_t manager,std::string_view name); // 같은할당에기존원문ctor와공유version0을쓰며header공개는바깥부모가맡는다.
    void InitializeEmptyManager(std::uint32_t manager,const std::function<void(o::DataManagerInitializeAssertion)>& assertion={}); // 고정manager의실제35개Create를공유mutable표로순회한다.선택진단은생략하지않는다.
    std::span<std::uint8_t> ManagerBytes(std::uint32_t manager); // 이미선택한같은manager276BYTE다.현재header를다시읽지않는다.
    o::TemplateLifetimeHooks ManagerLifetime(); // 같은owner의실제Destroy/Release를분리하여바깥NewManager에빌린다.
    std::shared_ptr<ClientTemplateSectionNames> SectionNamesOwner() const; // reader/신규초기화가공유하는Clientowner를읽는다.
'''+marker)
    marker='    std::array<std::uint8_t,180> headers_{};'
    source=replace_once(source,marker,'    std::shared_ptr<ClientTemplateSectionNames> sectionNames_; // 모든Image보다오래사는동일512BYTE lazy표owner다.독립Storage는새Client상태를한개소유한다.\n'+marker)
    outputs[names[1]]=source
    source=outputs[names[2]]
    source=replace_once(source,'#include "o/RawTemplateLoad.h"','#include "o/RawTemplateLoad.h"\n#include "o/RawDataManagerInitialize.h"\n#include <new>')
    source=replace_once(source,'    o::FortTemplate fort;','    ClientTemplateSectionNames& sectionNames; // Client와같은lazy이름표이며이Image보다긴Storageowner에서빌린다.\n    bool constructed{}; // 실제new된Image에원문ctor가끝났는지를구별한다.\n    o::FortTemplate fort;')
    before='''        std::vector<std::uint8_t> source,names;std::uint32_t allocating{},created{}; // 파일 할당과 새 임시 노드는 서로 다른 실제 소유자다.
        names.push_back('/');
        for (const auto section:o::FortTemplate::SectionNames()) { names.insert(names.end(),section.begin(),section.end());names.push_back('/'); } // 실제 SNART35개 이름과 마지막 빈 이름을 공급한다.
        names.push_back('/');names.push_back(0);
        o::DataManagerImageHooks hooks;hooks.names=[&names] { return std::span(names); };'''
    after='''        std::vector<std::uint8_t> source;std::uint32_t allocating{},created{}; // 파일할당은지역소유지만이름표는같은Client에서빌린다.
        o::DataManagerImageHooks hooks;hooks.names=[this] { return sectionNames.Names(); };'''
    source=replace_once(source,before,after)
    marker='    // State와 Territory 모두 같은 원시 섹션 바이트를 빌리며 파일 입력이면 원본 읽기 부모가 초기 목록을 만든다.'
    source=replace_once(source,marker,'''    // new276의같은할당이다.원문ctor/목록생성은아직하지않고metadata의선행노드투영도막는다.
    explicit Image(ClientTemplateSectionNames& names):sectionNames(names) { seeded.fill(true); }
    // 원문ctor는같은생성수명의실제manager와Client version을바꾼다.이름표할당은하지않는다.
    void ConstructName(std::string_view file,std::uint32_t& version) {
        if (constructed || !streams.empty() || file.size()>=256 || file.find('\\0')!=std::string_view::npos) throw std::invalid_argument("Template 빈Image 실제ctor계약");
        const std::span<std::uint8_t,4> global(reinterpret_cast<std::uint8_t*>(&version),4);o::ConstructTemplateDataManager(manager,file,global);name.assign(file);constructed=true;
    }
'''+marker)
    source=replace_once(source,'Image(o::FortTemplate value,std::string file,std::uint32_t& version,bool fileImage=false):fort(std::move(value)),name(std::move(file)) {','Image(o::FortTemplate value,std::string file,std::uint32_t& version,ClientTemplateSectionNames& names,bool fileImage=false):sectionNames(names),fort(std::move(value)),name(std::move(file)) {')
    source=replace_once(source,'        if (fileImage) return;','        constructed=true;if (fileImage) return;')
    source=replace_once(source,'CampaignTemplateStorage::CampaignTemplateStorage(std::shared_ptr<o::FortSerializationGlobals> serializationGlobals)\n    :serializationGlobals_(serializationGlobals ? std::move(serializationGlobals) : std::make_shared<o::FortSerializationGlobals>()) {','CampaignTemplateStorage::CampaignTemplateStorage(std::shared_ptr<o::FortSerializationGlobals> serializationGlobals,std::shared_ptr<ClientTemplateSectionNames> sectionNames)\n    :serializationGlobals_(serializationGlobals ? std::move(serializationGlobals) : std::make_shared<o::FortSerializationGlobals>()),sectionNames_(sectionNames ? std::move(sectionNames) : std::make_shared<ClientTemplateSectionNames>()) {')
    source=source.replace('SerializationVersion()));','SerializationVersion(),*sectionNames_));')
    source=source.replace('SerializationVersion(),true)','SerializationVersion(),*sectionNames_,true)')
    marker='// 같은DWORD의현재값과주소를호출자에게빌린다.'
    source=replace_once(source,marker,'''// 원문new276만실행하며header/ctor/이름표는나중에선택된부모가진행한다.
std::uint32_t CampaignTemplateStorage::AllocateEmptyManager(std::uint32_t bytes) {
    if (bytes!=276 || !nextHandle_) throw std::invalid_argument("Template 실제빈Image276할당계약");
    auto image=std::unique_ptr<Image>(new(std::nothrow) Image(*sectionNames_));if (!image) return 0;const auto token=nextHandle_++;images_.emplace(token,std::move(image));return token;
}
// 존재하는같은미생성할당만원문ctor로전환한다.
std::uint32_t CampaignTemplateStorage::ConstructEmptyManager(std::uint32_t token,std::string_view name) { images_.at(token)->ConstructName(name,SerializationVersion());return token; }
// 고정token의물리는header교체후에도같은Image이다.
std::span<std::uint8_t> CampaignTemplateStorage::ManagerBytes(std::uint32_t token) { return images_.at(token)->manager; }
// 실제표는진단뒤한번받고같은Image의기존rawCreate를매이름마다실행한다.
void CampaignTemplateStorage::InitializeEmptyManager(std::uint32_t token,const std::function<void(o::DataManagerInitializeAssertion)>& assertion) {
    auto& image=*images_.at(token);if (!image.constructed) throw std::logic_error("Template 실제ctor이전초기화");o::DataManagerInitializeHooks hooks;hooks.assertion=assertion;hooks.names=[&image] { return image.sectionNames.Names(); };
    hooks.create=[&image](std::span<const std::uint8_t> name) { const auto end=std::find(name.begin(),name.end(),std::uint8_t{});if (end==name.end()) throw std::out_of_range("Template 실제Create이름NUL범위");image.CreateSection({reinterpret_cast<const char*>(name.data()),static_cast<std::size_t>(end-name.begin())}); };o::InitializeDataManager(image.manager,hooks);
}
// 같은실제Image의소멸/할당반납을원문바깥부모에빌린다.
o::TemplateLifetimeHooks CampaignTemplateStorage::ManagerLifetime() { return Effects(); }
// 다른Storage도동일Client이름표를명시공유할수있다.
std::shared_ptr<ClientTemplateSectionNames> CampaignTemplateStorage::SectionNamesOwner() const { return sectionNames_; }
'''+marker)
    outputs[names[2]]=source
    patch=[]
    report={}
    # 제품기준복사본은한번만만들고제안파일도원문SHA가일치할때만준비한다.
    for name in names:
        relative=Path(name).relative_to('cpppj')
        baseline=WORK/'baseline'/relative
        destination=WORK/relative
        baseline.parent.mkdir(parents=True,exist_ok=True)
        destination.parent.mkdir(parents=True,exist_ok=True)
        if baseline.exists() or destination.exists():
            raise RuntimeError('기준/제안파일덮어쓰기금지:'+name)
        baseline.write_text(sources[name],encoding='utf-8',newline='')
        destination.write_text(outputs[name],encoding='utf-8',newline='')
        report[name]=dict(sourceSha256=hashlib.sha256((ROOT/name).read_bytes()).hexdigest(),proposalSha256=hashlib.sha256(destination.read_bytes()).hexdigest())
        patch.extend(difflib.unified_diff(sources[name].splitlines(True),outputs[name].splitlines(True),fromfile='a/'+name,tofile='b/'+name))
    (WORK/'existing-files.patch').write_text(''.join(patch),encoding='utf-8',newline='')
    (WORK/'storage-baselines.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
