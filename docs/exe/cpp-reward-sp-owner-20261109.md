# cpppj 1185 — Native HP·보상·SP의 같은 장기 owner

제품은 **1185 / 11.01**이다. M6-n의 n9c1 선행을 실제 적용했고, Native Session의 기존 HP/보상 caller도 같은 처리기를 빌린다. 전체 M6 완료가 아니다.

`RawSurfaceStorage`가 같은 Pool·현재 TYPE·공통 장부·최초 로딩 정수 비용표·GameAux 타입/환불 비율/ResourcePool과 `ClientSpGlobals::Store()`를 쓰는 `SquidRewardState`/`SquidReward`를 소유한다. Count는1184의 같은 `NativeFortSaveVirtual::Level`을 재사용한다. 독립 typed Session은 기존 지역 처리기를 유지한다. SP/난수/seed/World를 새로 만들거나 초기화하지 않는다.

fort/battle의 실제 C++ 필드는 bool이고, editor는 DWORD이다. fixedSp와 환불 모드는 해당 원래 주소를 사용한다. 사제 TYPE/약화도 GameAux 주소를 사용하므로 Session 지역 포인터가 장기 HP에 남지 않는다. SP 잔액 조회 뒤 현재 fort/editor를 읽는 순서와, Money 적재의 현재 모드 재조회를 연결했다.

장기 AI 지갑은 disabled→signed owner→현재 Player base→비영 brain 순서로 실제 조회한다. Player 표는 `ClientPlayerRecords::InitializeBody`에서 재할당되므로 생성 때의 span을 보관하지 않는다. 본체 선택 뒤에만 같은 현재 Session의 `RawAiBrainMemory`와 실제 전체 Runtime pin을 빌리고, demo 조회 뒤에도 저장한 같은 brain을 다시 읽는다. NULL brain은 Session 없이 반환한다. 비영 brain의 NoWorld 전체 AI owner는 후속이다. 실제 SP 표시는 기존 예산 serial 검색→없으면 반환→Push/Attach→깊이 복원을 재사용한다. demo는 현재 Mission의 실제 `gate.demo`이며 Mission이 없으면0이다.

처음 빌드 중 현재 Player span의 재할당 수명을 추가 점검했고, 그 빌드가 종료된 뒤 현재 base getter를 보완했다. 첫 빌드/소스 해시는 `build-initial.*`/`source-initial-frozen.json`으로 보존했다. 최종 결과는 보완 뒤 두 번째 빌드다. 빌드 중 제품 소스 수정은 없었다.

원문은 `0040eec0`, `0041df80`, `00413bb0`, `00413c20`, `00463b50`, `0043dad0`, `0044c2f0`, `0043cb30`, `0044b220`, `0049fce0`, `00483300`, `004bf340`, `00447810`의 전체 C/PE 범위를 보존했다. 현재 호스트와 마지막 완성 디컴파일 호스트는 HJOW-Athlon이다(Client2026-11-03, R2026-11-07). 디컴파일 소스는 변경하지 않았다.

검증은 NetstormCpp/NetstormRoot/집중 대상의 최종 Release/x64 빌드, 비MP 집중26검사(AiWallet10·기존 보상9·GameAux 보상2·FortObjectLoad5), 격리 Tutorial4의 워크샾 생산→윈드 제네레이터 건설 요청→600frame 대기와 Edit→TEST01→메인 복귀/정상 종료다. 보상 fixture는 원래 세 판본 각1000행이다. 보호2800파일·설치 자료·실행 파일·고정16소스 불변을 확인했다. 이 결과를 전체 솔루션/전체 시나리오/NoWorld 전체 동작 검증으로 합산하지 않는다.

새 멀티플레이 단위/패킷/게임/통신·원본 실행·사제 포획/희생·특정 적 공격·승리·공중3종 전용 검증은0이다. delay queue는 비활성 유지한다. 다음은 같은 HP/보상 처리기를 쓰는 장기 공통/자원 삭제, ConstructionRuntime·큰 AI/Carrier owner이며 Source/listener/공통표/Poll·잔여 F·큐·Snapshot/JoinTestBattle도 남는다. 최신 ZIP1092는 이번 제품의 ZIP이 아니며 M8에서 정합성을 마감한다.

[단위 보고서](../../cpppj/reward-sp-owner-20261109.json) · [현행 인계](../../LEFT_JOBS.md). 원시는 `extracted/cpp-resume-1185/`에 있다.
