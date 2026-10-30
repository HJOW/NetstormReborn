// 보호 PE의 세 CD 탐색 몸체가 쓴 목적지 전체256BYTE/항목 순서/열린 핸들을 비교한다.
#include "TestSupport.h"
#include "client/CdDirectory.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace { // 파일 내용/순서만 명시하고 제품의 재귀/종류 필터/반납을 그대로 사용한다.
using namespace netstorm::client; // 실제 제품의 동일 CD 경계를 검사한다.
std::vector<std::string> Parts(const std::string& text,char delimiter) { std::vector<std::string> result;std::istringstream stream(text);std::string part;while (std::getline(stream,part,delimiter)) result.push_back(part);return result; } // fixture의 순서 있는 필드를 분리한다.
struct CdProbe { // 실제 Win32 장치 대신 명시 디렉터리/현재 열거 위치를 제공한다.
    std::vector<std::pair<std::string,std::uint32_t>> roots;std::map<std::string,std::vector<CdDirectoryEntry>> tree; // OS가 나열한 루트/항목 순서다.
    std::map<std::uint64_t,std::pair<std::string,std::size_t>> handles;std::uint64_t serial{1}; // 현재 열린 열거 핸들/다음 고유 번호다.
    std::uint32_t entries{},cancel{};bool error{};std::string trace; // 현재 항목/중단 시점/열거 실패와 실제 callback 순서다.
    void Trace(std::string item) { if (!trace.empty()) trace+=';';trace+=item; } // 순서 전체를 원문 장치 관측과 비교한다.
    CdDirectoryHooks Hooks() { // 제품이 선택한 실제 장치만 호출하도록 제공한다.
        CdDirectoryHooks h;h.drives=[this] { Trace("drives");std::vector<std::string> result;if (!error) for (const auto& [root,type]:roots) result.push_back(root);return result; }; // OS 열거 실패도 빈 루트와 구별해 입력한다.
        h.driveType=[this](auto root) { Trace("type:"+std::string(root));for (const auto& [name,type]:roots) if (name==root) return type;return 0U; }; // 실제 루트 순서대로 종류를 조회한다.
        h.first=[this](auto pattern) -> std::optional<CdDirectoryFirst> { Trace("first:"+std::string(pattern));const auto directory=std::string(pattern.substr(0,pattern.size()-2));const auto it=tree.find(directory);if (it==tree.end() || it->second.empty()) return {};const auto handle=serial++;handles.emplace(handle,std::make_pair(directory,0));++entries;return CdDirectoryFirst{handle,it->second.front()}; };
        h.next=[this](auto handle) -> std::optional<CdDirectoryEntry> { auto& [directory,index]=handles.at(handle);Trace("next:"+directory);if (++index>=tree.at(directory).size()) return {};++entries;return tree.at(directory)[index]; };
        h.close=[this](auto handle) { Trace("close:"+handles.at(handle).first);handles.erase(handle); };h.cancelled=[this] { return cancel!=0 && entries==cancel; };return h;
    }
};
}

// 원문의 최초15관측을 같은 목적지/루트·항목·중단·길이 입력으로 재현한다.
TEST_CASE(CdDirectory_Native15DestinationTraceAndHandleLifetime) {
    std::ifstream file(NETSTORM_CD_DIRECTORY_FIXTURE);CHECK(file.good());std::string line;int count{}; // 정식 독립 fixture다.
    while (std::getline(file,line)) { // 각 관측은 새 장치/목적지로 실행한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto fields=Parts(line,'\t');CHECK(fields.size()==10);if (fields.size()!=10) return;CdProbe probe;probe.cancel=static_cast<std::uint32_t>(std::stoul(fields[3]));probe.error=fields[9]=="1";
        if (fields[1]!="-") for (const auto& item:Parts(fields[1],',')) { const auto pair=Parts(item,'=');probe.roots.emplace_back(pair[0],static_cast<std::uint32_t>(std::stoul(pair[1]))); } // 물리 드라이브 순서를 보존한다.
        if (fields[2]!="-") for (const auto& item:Parts(fields[2],';')) { const auto parts=Parts(item,'|');probe.tree[parts[0]].push_back({parts[1],parts[2]=="1"}); } // OS 항목은 정렬하거나 합치지 않는다.
        std::array<char,256> destination;destination.fill(static_cast<char>(0xa5));std::copy_n("seed",5,destination.begin());const auto found=FindCdDirectory(std::span(destination).first(std::stoul(fields[4])),probe.Hooks());CHECK(found==(fields[5]=="1"));
        CHECK(fields[6].size()==512);for (std::size_t i=0;i<destination.size();++i) CHECK(static_cast<unsigned char>(destination[i])==std::stoul(fields[6].substr(i*2,2),nullptr,16)); // 마지막 guard까지 전체 물리를 비교한다.
        CHECK(probe.trace==fields[7]);CHECK(probe.handles.size()==std::stoul(fields[8]));++count;
    }CHECK(count==15);
}

// 긴 한국어 UTF-8 경로도 실제 호환 장치의 확장 버퍼에서 중간 BYTE가 잘리지 않는다.
TEST_CASE(CdDirectory_UnicodePathAndInvalidDestination) {
    CdProbe probe;probe.roots={{"D:\\",5}};const std::string directory="한글 영상😀";probe.tree["D:"]={{directory,true}};probe.tree["D:\\"+directory]={{"CD.NETSTORM.TITANIC.ACTIVISION",false}};std::vector<char> output(4U*32768U);CHECK(FindCdDirectory(output,probe.Hooks()));CHECK(std::string(output.data())=="D:\\"+directory);CHECK(probe.handles.empty());
    bool rejected{};try { static_cast<void>(FindCdDirectory({},{})); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
