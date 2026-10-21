# Factory 업그레이드 예측 그림과 완료

2026-10-22 단위284, 시작 HEAD `37ad14a1 / 20261022-06 CP`. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-21**로 같다. 기존 정밀 산출물과 보호 10.78 PE를 정적으로 읽었으며 새 디컴파일·원본/CD/AGENTS 변경은 없다. 사용자의 게임·집중 C++ 검사 실행 중지는 유지한다.

## 변경

[`RawFactoryUpgrade`](../../cpppj/src/o/RawFactoryUpgrade.cpp)는 `004549c0 event3`의 두 단계를 같은 실제 raw 풀/Host에 연결한다.

1. count0에서 같은 TYPE의 flags2 client 객체를 만든다. `00453750`처럼 현재 레벨을 상한3까지 증가시키고, 상한 미만에서는 가상 getter를 다시 읽는다. 가상84의 상태/표시 갱신 뒤 extra8, void 진단/abstract 비트, **상태 WORD+12=0**을 적용한다. 이 WORD는 프레임 DWORD+36이 아니다. 현재 소유자를 지정하고 현재 좌표의 y에 PE `00509144`의 `0.0010000000474974513f`를 더해 Pop한다. 원본 로그→같은 실제 건설30/시각효과17·음향47→실제 양쪽 DependForm→부모 프레임 진행→Damageable 부모 반환 순서다.
2. count양수에서 로그→`004545e0` 완료→현재 소유자/Player+b0 검사→`00413b70/00411180` AI 통지→PE `00502350`의 -1 반환을 적용한다. 아웃포스트는 완료 효과 전체를 생략하지만 후속 AI 검사까지 생략하지 않는다.

완료는 가상84→현재 최대HP/직접 HP 쓰기→기존 `RawFactoryPostPop::AdjustFrame`→가상88→위치 `upgradeDone.wav`→현재 로컬 소유자일 때 `upgradeComplete.wav`→플레어6/event2→같은 Host의 event4 전부→event3 전부→예측 그림 삭제 순서다. 현재 실행 중인 예약도 지우며 기존 `SquidProcessHost::RunRegular`의 Kernel 등록 재검사로 삭제 뒤 접근을 막는다. 다른 사건은 보존한다. PATCH event5의 레벨3 플레어2/event1도 기존 `RawFlare`에 연결했다.

그림 finder는 `004b22d0/004b1b80/004b1bd0/004ae510`처럼 현재 부모 head에서 시작하고 **후보 반환 전에 next를 저장**한다. 실제 Depend TYPE만 선택하며 free/dead/owner 조건을 추가하지 않는다. 같은 TYPE, extra8, SID5..14999인 감시자만 삭제한다. 자기자신/비권한 dead 부모 진단 후에도 원본 순서대로 진행한다. `004537f0/00453c90`의 그림→event4→event3 단락 조회를 구현하고 Session의 `FactoryUpgrading`에 연결했다.

`RawAiFactoryProduction::Upgraded`는 같은 시설을 원래 미처리 큐에 중복 허용으로 추가한다. 큐가 가득 차도 현재 TYPE 비용의 **음수를 signed64 절삭한 하위 DWORD**를 같은 AI 지갑에 더하고, 현재 데모이면 현재 지갑에 signed1190 하한을 적용한다. PE `004111bd fld [eax+c4] / fchs / ftol / add [brain+2d8]`를 확인했다. 이는 앞선58h 수신의 실제 SP 비용 처리와 별도의 원본 통지다. Session은 현재 Player+b0와 challenge를 다시 확인한다. 시설 생산의 업그레이드 하위도 같은 실제 요청/수신/그림/완료 경로에 연결했다. 전체 AI 자동 전략이 완성됐다는 뜻은 아니다.

## 검증 기록

새11검사는 라이프사이클7개, AI 완료 큐/지갑2개, AssetRegular 분배1개, 실제 Session 예측 그림/건설/완료 수명1개다. 모두 **작성·컴파일만 하며 실행하지 않는다**. 실제 Session 검사는 별도 게임이나 일반 프레임 전체 대신 같은 Host의 선택된 예약 두 단계만 호출하도록 작성했다. 이 검사 역시 실행하지 않았다.

첫 Release 빌드84357은 통합 검사에서 비공개 `Host::RunRegular`를 호출한 C2248 두 건/경고0/종료1이다. `build1.log/build1-result.json`은 보존했다. 공개 `RegularProcess::RunFrame`으로 수정한 재빌드 세션78285는 실제 종료까지 조회했고 **종료0·경고0·오류0**이다. 최종 기록은 `extracted/cpp-factory-upgrade-lifecycle-20261022-build2.log`와 같은 이름의 `build2-result.json`이다. 두 로그를 내용 변경 없이 UTF-8로 변환했다.

[`cpp_factory_upgrade_lifecycle_audit.py`](../../tools/cpp_factory_upgrade_lifecycle_audit.py)의 [신규 JSON](../../cpppj/factory-upgrade-lifecycle-static-20261022.json)은 원본16함수의 정밀 범위/상수·직전과 같은 보호 PE SHA·최종 소스/바이너리 SHA·새11검사 링크 이름·첫 빌드 실패 기록을 확인한다. **C++ 검사·CTest·게임·native 실행0**이다. 이전 보고서는 덮어쓰지 않았다.

## 남은 작업

- 사용자 입력 `03000032`의 현재 자금 부족/이미 진행 중 안내와 실제 메뉴 입력 연결은 후속이다. 명시 `RequestFactoryUpgrade`는 원본 요청 하위이므로 입력 관문을 대신하지 않는다.
- 고갈 지점 정리/사제/건물 선택/다리 전략/수집·포획 등 다른 실제 AI 행동, 원래 순서의 자동 프레임/종료를 이어 복원한다.
- 목표는 Early Missions6개+캠페인1-1..1-6+데모3개, 총15개와 Edit/custom Fort/시나리오 동일성이다. Template/State7·전환/저장 수명, 자연 입력·표시·음향과 반복 플레이는 미검증이다.
- 전체 x87 확장 정밀도/예외와 CD10.72의 별도 구현/실행 대조도 미완료다. 이번 정적 비교는 C++ 검사·CTest·게임·native 실행을 뜻하지 않는다.

인수인계와 최종 증거를 확정한 뒤 `20261022-07 CP` 형식으로 로컬 커밋한다. 원격 push하지 않는다.
