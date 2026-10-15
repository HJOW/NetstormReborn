# 실제 Player 기술 삭제·조회·목록 추가

현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 원본 게임/복사본/GUI를 실행하지 않고 보호 PE와 정밀 프로젝트를 읽기 전용으로 분석했다. `originals`/`originalCD`/AGENTS를 변경하지 않았다. 구현은 PATCH10.78 전용이며 CD10.72를 같은 주소/배치로 추정하지 않는다.

`RawPlayerTechnology`는 같은 실제 `RawPlayerWorld`, `RawCarrierPreDestroy::FindContained`, `RawContainedCreate`, 번호순 TYPE의 group, 가상 삭제를 연결한다. 모든 하위의 Pool 동일성을 생성 시 확인한다. Player9/내용물 목록/별도 기술 bool 표를 복사하지 않는다. `BindPlayerTechnologyTypes`는 기존 실제 `RiftTypeTable::Find`를 직접 연결한다.

| 원본 | 복원 API | 실제 순서 |
| --- | --- | --- |
| `00490330` | `Find(owner,type)` | 타입 범위 assert→같은 PlayerKnown 진단/Technology DWORD→실제 contained(mask4,type,index0) |
| `004903b0` | `Clear(owner)` | PlayerKnown 진단/현재 Technology 부모를 한 번 저장→mask4 첫 내용물 찾기→실제가상삭제 flags0 반복 |
| `00490420` | `Ensure(owner,type)` | Ensure 타입 assert→Find의 별도 타입 assert/PlayerKnown→없으면 현재 부모를 다시 조회→contained(parent,type,0,4,10,0) |
| `004914e0` | `ParseStep(text,list)` | 현재 세미콜론 항목→실제 TYPE 이름 조회→all 전체group 검색→40개 제한/중복제거→원본 index/종료 조건 |
| `004915c0` | `AddList(owner,text)` | 정상 종료 목록 전체를 먼저 구성한 뒤 각 타입을 원래 순서대로 Ensure |

타입 번호가1 미만/실제 numTypes 이상이면 원본처럼 진단 뒤 계속한다. Ensure의 type assertion은 Find 내부의 assertion과 위치가 다르므로 합치지 않는다. 없는 기술의 생성에서는 Find 조회 뒤 **현재 부모를 다시** 읽고 PlayerKnown 진단도 반복한다. 진단을 단순 false 반환으로 바꾸지 않는다. 실제9개 Player/SID 저장소 밖의 메모리 손상은 기존 host 범위 계약에서 명시적으로 거부한다.

Clear는 삭제 전 next를 캡처하지 않는다. 매 삭제 뒤 **처음 저장한 부모**의 현재 체인을 처음부터 검색한다. 가상 삭제 callback이 Player Technology DWORD를 바꾸어도 원래 부모의 제거를 계속한다. flags4가 없는 내용물/contained타입이 아닌 체인 원소는 기존 실제 finder가 그대로 제외한다. 가상 삭제는 같은 raw 체인의 실제 해제/통지 몸체여야 하며 빈 성공으로 공급하면 안 된다.

Ensure의 create 인자는 flags4, attachFlags10이며 menu 미리보기의 flags20/attach10과 다르다. 실제 `RawContainedCreate`가 Factory의 Form6/client flags2를 요청하고 저장 type WORD/argument WORD0/quality BYTE0/flags WORD4를 쓴 뒤 실제 ContentForm 부착에 넘긴다. 자동 Player 생성이나 다른 tech 부모 대체를 추가하지 않는다. `RawPlayerWorld::Create`는 별도 시작 부모/AI 초기 갱신의 실제 공급 전제다.

## 40개 목록과 원본 무진행

단어는 기존 세미콜론 전용 `ConfigListItem`으로 읽는다. 빈 첫 항목/연속 세미콜론은 그 위치에서 종료하므로 뒤의 항목을 읽지 않는다. 각 항목은 빈 값 여부 검사보다 TYPE 이름 조회가 먼저다. 단순한 trim/공백 구분/빈 항목 건너뛰기를 추가하지 않는다.

ASCII 대소문자를 무시한 `all`은 실제 타입0부터 numTypes-1까지 group!=10인 타입을 추가한다. capacity40이 가득 차면 이후 후보를 추가하거나 기존 항목을 교체하지 않는다. capacity 확인 뒤에만 중복 검사를 하며 첫 출현 순서를 유지한다. 일반 이름은 실제 lookup 결과가 양수일 때만 추가하고 index를 증가시킨다. `all` 문자열의 실제 lookup까지 양수인 특이 입력이라면 원본대로 index를 두 번 증가시킨다.

**미등록 비빈 이름 또는 type0 이름은 원본 `004914e0`의 index가 증가하지 않아 같은 항목을 무한 반복한다.** `ParseStep`은 이 한 반복을 그대로 실행하고 `Stalled`를 돌려줘 현재 index/누적 목록을 관찰하게 한다. `AddList`는 이 상태를 성공/skip으로 처리하지 않고 구체적 오류를 보고한다. 이 오류가 난 뒤 부분 목록을 Ensure하지 않는다. 정상 미션 기술 문자열의 종료 경로가 이번 완료 범위이며, 원본의 무한 반복을 실제 GUI 스레드에서 그대로 멈추지 않게 실행하는 것은 host 경계에서 제외한다.

원본 임시 목록은 new(160)/free다. host `PlayerTechnologyList`는 동일40개 DWORD 배열과 count/index를 값으로 보유한다. 임시 메모리의 주소/할당 실패는 이번 정상 경로 대조 범위가 아니며 실제 heap 수명 계약을 복원했다고 주장하지 않는다. 임시 할당/반납은 fixture의 기술 효과 비교 trace에서도 제외하며 검사가 제품의 요청처럼 N/X를 합성하지 않는다.

## 원본 대조

[선택형 정밀 export 목록](../../tools/ghidra/player-technology-functions.json)의5함수, [관찰 도구](../../tools/decomp_player_technology.py), [SHA 보고서](../../cpppj/player-technology.json), [원본 fixture](../../cpppj/tests/fixtures/player-technology-x86.tsv)를 추가했다. **158고유입력**은 Find56/Ensure56/Clear16/정상Parse9/Add18/무진행3이다. 같은 입력의 재생/강화는 새 원본 관찰로 합산하지 않는다.

원본5함수와 기존 `004ac9f0`/`004b2270`/`004b1b20`/기본true 필터/`004afa70`는 실제 기계어로 실행한다. 실제 제어 분기를 대체하지 않는다. 명시 입력 TYPE48개의 이름/group과 이름 조회 결과, 임시힙/Factory 반환, 가상부착/가상삭제의 raw 응답만 외부 경계다. 가상부착/삭제는 같은 raw 체인을 실제로 변경한다. 정상 출력에서는 부모/4개 기존 종속의 전체50바이트, 모든 새 Form50바이트, 실제 부모 DWORD, 목록/반환값/효과 순서를 비교한다. 전체 Pool의 free list/Factory 내부 할당까지 이158입력으로 원본 대조했다고 주장하지 않는다.

무진행3입력은 실제 같은 index의 두 반복을 관찰하고 다음 동일항목 요청 직전에 격리 실행을 중단했다. 정상 반환/승패 완료 관찰로 세지 않는다. `PlayerTechnologyTests`는 같은 두 `ParseStep`의 현재 상태를 재생한다. 별도 실제보호자산 연결 검사는 `RiftTypeTable::Find`로 sunWalker/windBattery/sunCannon의 순서를 확인하고, 미등록 항목이 있으면 실제 Ensure/생성을 시작하지 않는 경계를 확인한다.

도구의158입력 고유성/SHA 감사는 통과했다. 최초 root Release 빌드는 경고0/오류0이었으나 집중2개 모두 검사 setup의48개 TYPE 입력이 공통Post의 실제PATCH188개 계약에 맞지 않아 시작 전에 예외로 실패했다(`extracted/player-technology-root-focused.log`, `extracted/run47-49-build.log`). 이 실패는 완료로 세지 않는다. 검사 setup을 Post/Factory에는188개, 기술 fixture의 선택 표에는 앞48개 span을 공급하도록 교정했다. production의 공통 타입 계약을 완화하지 않았다.

교정 뒤 **통합 게임/검사 Release 빌드 경고0·오류0, 집중2개 모두 통과**를 root가 확인했다(`extracted/player-technology-root-final-build.log`, `extracted/player-technology-root-final-focused.log`). 최종158입력 고유성/SHA/UTF-8/미추적 새파일 포함 행끝공백 감사도 통과했다. 이 결과는 기존전체1183개와 구분하는 새집중 검사이며 원본 정상반환155개/무진행3개를 명시적으로 구분한다. Session/GameWorld/CMake/LEFT/커밋은 이 하위 agent가 변경하지 않았다. Stage3의 SP/기술 시작 순서 모듈과 연결하기 전에는 실제 가상 파괴/ContentForm 알림/현재 PlayerKnown의 수명 계약을 확인해야 한다.
