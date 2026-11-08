// 현재10.78공통후처리를같은World의실제그래프/목록/장부/선택재검사에연결한다.
#include "o/SquidPostPop.h"
#include "o/RawSquidPostPopParent.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 독립CD/명시Validate계약은기존몸체에남긴다.
void SquidPostPop::PostPopPatch(Sid sid,std::uint32_t flags) { // 하위실패전의실제효과를롤백하거나사전전수검사하지않는다.
    SquidPostPopParentHooks hooks;hooks.suppressed=[this] { return state_.Suppressed(); };hooks.graphsEnabled=[this] { return state_.graphsEnabled; };
    hooks.type=[this](std::uint8_t number)->const RiftTypeRecord& { const auto current=CurrentTypes();if (number>=current.size()) throw std::out_of_range("공통후처리현재TYPE범위오류");return current[number]; };
    hooks.region=[this](Sid object) { if (!graph_) throw std::logic_error("실제후처리영역그래프미연결");graph_->InvalidateRegion(object); };
    hooks.add=[this](Sid object) { if (!graph_) throw std::logic_error("실제후처리Add그래프미연결");graph_->Add(object); };
    hooks.assertion=[this](auto expression,auto file,auto line) { const auto diagnose=manaDiagnostics_.assertion;if (!diagnose) throw std::logic_error("공통후처리실제진단미연결");diagnose(expression,file,line); };
    hooks.resetGraph=[this](Sid object) { if (graph_) graph_->SetNumber(object,state_.invalidGraph);else { const auto raw=pool_.Slot(object);const_cast<std::uint8_t*>(raw.data())[30]=state_.invalidGraph; } }; //독립미연결Graph의기존유효surface번호쓰기계약을유지한다.
    hooks.cost=[this](Sid object) { state_.TotalCost()=TotalCost(pool_.Slot(object)[10]); };
    hooks.provider=[this](Sid object) { RegisterManaProvider(pool_.Edition(),state_.Providers(),state_.productionDirty,object.value,manaDiagnostics_); };
    hooks.ownerFactory=[this](std::uint32_t owner,Sid object) { if (!owner) return;if (owner>=state_.ownerFactories.size()) throw std::out_of_range("공통후처리현재owner공장범위오류");state_.ownerFactories[owner].Append(object.value); };
    hooks.factory=[this](Sid object) { state_.Factories().Append(object.value);++state_.productionDirty; };
    hooks.aiNotice=[this](std::uint32_t owner,Sid object) { const auto notice=aiNotice_ ? aiNotice_:defaultDevices_.aiNotice;if (notice) { notice(owner,object,true);return; }if (owner>=state_.aiAttached.size()) throw std::out_of_range("공통후처리현재AI owner범위오류");if (owner && state_.aiAttached[owner]) throw std::logic_error("공통후처리실제AI통지미연결"); };
    hooks.localOwner=[this] { const auto owner=localOwner_ ? localOwner_:defaultDevices_.localOwner;return owner ? owner() : state_.localOwner; };
    hooks.localCount=[this](std::uint32_t type) { ++state_.LocalCounts()[type];++state_.LocalSecondaryCounts()[type]; };
    hooks.globalCount=[this](std::uint32_t type) { ++state_.GlobalCounts()[type]; };
    hooks.recheckSelection=[this] { const auto current=selectionRecheck_ ? selectionRecheck_:defaultDevices_.recheckSelection;if (current) current();else if (state_.pendingPlacement) throw std::logic_error("공통후처리실제선택재검사미연결"); }; //독립명시입력의mouseType0은장치없이자연반환한다.
    RunSquidPostPopParent(pool_,sid,flags,state_.depth,hooks);
}
void SquidPostPop::BindPatchSelectionRecheck(std::function<void()> recheck) { if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("선택재검사대여판본오류");selectionRecheck_=std::move(recheck); } //Session정리때빈함수로분리한다.
void SquidPostPop::BindPatchLocalOwner(std::function<std::uint32_t()> owner) { if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("현재로컬owner대여판본오류");localOwner_=std::move(owner); } //통지뒤현재실제owner를읽는다.
}
