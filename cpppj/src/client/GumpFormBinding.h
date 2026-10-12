// 실제 Gump 등록 표/가상 객체 삭제/메뉴 Form 공급/지연 수명을 공용 SID 풀의 GumpForm에 연결한다.
#pragma once
#include "client/MenuGumpWidgets.h"
#include "o/GumpFormHost.h"

namespace netstorm::client {
// 원본 Gump vtable +4의 객체 삭제 통지다. 대상은 등록 ID가 아닌 조회된 현재 Gump 토큰이다.
using GumpObjectRelease=std::function<void(std::uint32_t token,o::Sid object)>;
// 00464190/CD 00492be0: 알림 시점의 등록 표를 조회하고 현재 가상 +4 처리기에 부모 SID를 전달한다.
// state/가상 처리기와 그 저장소는 반환 콜백보다 오래 살아야 한다. null 등록/범위 초과는 원본 assert 계약 밖이다.
o::GumpFormNotice MakeGumpFormNotice(GumpCreateState& state,GumpObjectRelease releaseObject);
// 00466740/CD 00492ff0의 기본 가상 +4다. 전달한 부모 SID를 사용하지 않고 실제 자식 우선 Kill을 수행한다.
// 기본/메뉴처럼 이 가상 함수를 상속하는 노드에서 사용한다. 별도 파생 재정의는 자신의 분배기가 처리해야 한다.
void ReleaseGumpObject(const GumpLifetime& lifetime,std::uint32_t token,o::Sid object);
// 메뉴의 Form(type,2)/owner ID/가상 Attach 경계를 같은 SID 풀에 연결하고 실제 내장 타입 3을 공급한다.
// 호출 전에 다른 생성/Attach 저장소와 판본을 맞춰야 하며 widgets가 훅을 복사하기 전에 연결한다.
void BindMenuGumpForms(GumpCreateState& create,MenuGumpWidgetState& state,MenuGumpWidgetHooks& hooks,o::GumpFormHost& forms);
// Gump Kill의 +0x28 Form 인덱스를 실제 공통 삭제/Unpop/SID 반납에 연결한다. 다른 Form의 기존 삭제 경계는 보존한다.
// lifetime 생성 전 호출하며 forms는 lifetime과 반환 훅보다 오래 살아야 한다.
void BindGumpFormLifetime(GumpLifetimeHooks& hooks,o::GumpFormHost& forms);
}
