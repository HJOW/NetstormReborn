# 실제 Session의 Player 링크 반납과 Kernel 선행 순서 — 단위392

기존 원문0048fcd0을 실제 Session의 현재 Player/Deck owner에 연결했다. 정상 Session 종료에서 Kernel 가상 소멸자는 이미 정리된 기술 링크·factory 개수·동적 Deck을 관측한다. 전체 State 부모나 Client/Login Core 공유의 완료는 아니다.

## 원문과 실제 소유자

기준은 `ca34d0be / 20261024-14 CP`다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 일치한다. 기존 정밀 C를 읽었으며 새 디컴파일/Native 관측은 만들지 않았다.

원문은79BYTE(`48fcd0-48fcd7;48fce0-48fd26`)이며 Player0..8의 +9c DWORD0→+8 DWORD0→+68 load→비0일 때 free→그때 현재 Player+68 DWORD0 순서다. 최초81입력×두 정밀도 **162관측(정상138/NULL물리6/외부중단18)**의 [Native 보고서](../../cpppj/player-links-cleanup-native.json), [fixture](../../cpppj/tests/fixtures/player-links-cleanup-x86.tsv), [기존 부모](../../cpppj/src/o/RawPlayerLinksCleanup.cpp)를 그대로 재사용했다. `--verify` 종료0이고 저장소의 보고서는 기존 extracted 원본과 BYTE가 같다. CD10.72 SourceMap은0046dfb0이며 새 CD 실행/전체 CD 복원을 뜻하지 않는다.

같은 ClientPlayerRecords의 현재 Records()를 각 원문 load/store에서 다시 읽고, 같은 Session의 RawProductionDeck::Release(token)에 선택된 free를 전달한다. +8은 [단위391](cpp-client-player-records-20261024.md)에서 실제 bookkeeping.ownerFactories의 Count와 연결했으므로 원문 쓰기가 현재 facade에도 바로 보인다. factory 포인터/용량과 그 할당, Player 전체 할당은 반납하지 않는다.

## 두 호출 문맥

`RawCampaignSession::ReleasePlayerLinks()`는 명시 State 자식 경계다. loaded/current owner/known 같은 새 선행 관문을 넣지 않았다. 선택된 실제 Deck 토큰이 없으면 원문의 그 호출 위치에서 예외가 전파되고, 앞선 행과 해당 행의 +9c/+8 쓰기는 유지하며 후속 +68 zero/행 방문은 실행하지 않는다. Kernel/Core/Player 초기화는 호출하지 않는다.

Session의 C++ 최종 소멸은 기존 AI 반납 뒤 ReleasePlayerLinks→기존 world.ClearProcesses→분배 callback 해제→멤버 반납 순서다. 이 단계에서만 기존 종료 정책에 따라 오류를 stderr로 진단하고 남은 소유권을 반납한다. 이 종료 예외 처리로 Native State 부모의 실패를 정상 성공으로 바꾸지 않는다. 명시 API를 호출하는 실제 State 어댑터는 실패 중단 계약을 사용할 수 있다.

기존391의 Player 대여는 services/Login보다 뒤에 선언돼 그 소비자보다 먼저 끝날 수 있었다. 서비스가 마지막 callback 보유자를 반납할 때 다음 Acquire가 아직 거부돼야 한다는 새 회귀는 교정 전 **1검사/1check 실패**였다. Player owner→Use→services를 Impl의 첫 멤버로 옮기고 입력 인자에서 먼저 빌려, 모든 Session 소비자가 반납된 뒤에만 재대여하게 고쳤다. 교정 뒤 실제 callback 소멸 관측·재대여 차단·최종 정상 재대여를 각각 확인했다. 기존391의 ‘모든 소비자보다 오래’라는 의도는 이 교정으로 보완한다.

기존 RawProductionDeck 소멸자는 자신이 소유한 현재 +68만 정리하므로 원문 +9c/+8 및 Kernel 선행 반납을 대신할 수 없었다. 새 연결은 해당 소멸자나 단일 free의 의미를 변경하지 않는다. Core 선택/캐시 복사/known0도 암묵 추가하지 않았다.

## 검증

최초 PlayerLinks 신규4와 관련13의 집중17개는 첫 실행에서 통과했다. 추가 수명 회귀의 최초 실패를 보존한 뒤 교정 제품의 **신규5+관련13, 고유18개/실패0**을 확인했다. 검사 이름을 원시 출력에서 집계했고 제품/검사 SHA 및 원본1394 불변을 확인했다.

| 신규 검사 | 실제 확인 범위 |
| --- | --- |
| 명시 정리 | 실제 Client Player/9 Deck을 반납하고1620BYTE에서 세 DWORD 외의 BYTE를 보존한다. Kernel 등록/개수, Core cached alias/active, Player base/factory 포인터·용량을 유지한다. 반복 호출은 새 Deck을 할당하지 않는다. |
| 최종 소멸 | 실제 Kernel의 가상 소멸 callback이9행/실제 Deck 전부 반납·facade Count0·factory 용량30·살아 있는 같은 Player base를 확인한다. 이전 Session 소멸 뒤 다음 실제 초기화 부모가 Player/factory를 교체한다. |
| 빈 Deck | +68이 모두0이면 명시 정리와 최종 소멸 모두 실제 할당을 선택하지 않는다. |
| 선택 free 실패 | owner3의 미등록 토큰에서 앞3행 반납·해당 행 +9c/+8만0·포인터 보존·뒤5행 미방문·Kernel 미실행을 확인한다. 입력 포인터 복원 후 같은 실제 owner로 나머지를 정리한다. |
| 서비스 소비자 수명 | 마지막 서비스 callback 보유자의 소멸이 관측되고, 그 시점의 다음 Player Acquire는 거부된다. Session 전체 소멸 뒤의 Acquire는 성공한다. |

위 명시 입력은 headless 단위 검사이며 실제 플레이에 상태를 주입하지 않는다. 교정 제품은 **ecd705109d6df15a2316918f1b5329e321c58ebcfd0ba1d7959610952f85b376**, 검사 파일은 **f2263305340a1e2840efc0f9e57fad2aa881cf5c4e8b595d7c4f482b6f9f8074**다. 모든 실제 게임 종료와 설정 복구 뒤 최종 CTest1은 **2669검사/실패0·종료0·314.67초(전체314.71초, wrapper314.79초)**로 통과했다. 두 실행 파일 SHA는 검사 전후 같고 원본1394/1394가 동일하다. 결과는 [제품 보고서](../../cpppj/session-player-links-runtime-20261024.json)에 고정하며 최초 출력은 `extracted/cpp-session-player-links-392-*`에 보존한다.

최종 감사는 소스/재현 계획5개와 증거76개의 SHA를 고정했다. 최초 Native 자료와 기존 정리/Deck 구현은 변경하지 않았고 보호 경로의 추적 변경은0이다. UTF-8·문서 링크·선택8파일을 검사한 뒤 **20261024-15 CP**로 로컬 커밋한다.

## 실제 입력 회귀의 최초 실패와 범위

최초701b2806 제품에서 기존 첫 matching 다리 항목 고정 입력과35,33 간헐천 대체 입력은 D/SP0에 남아 고정 프레임의 Next Tutorial 명령에서 종료1이었다. 두 실행은 원본1394 불변이며 화면/입력/원시 종료를 보존했다. 직전391 Session 소스/헤더만 분리한 진단 파일7f05c238은 같은 기존 입력을 통과했으나 시계·난수까지 같은 통제실험이 아니므로 원인 확정이나 현재 제품의 성공으로 합산하지 않는다. 진단 target을 위한 임시 CMake hook은 빈 값으로 복구했고 공용 제품/검사 파일은 그대로였다.

QWASZX 계획의 첫 준비는 기존 검사 도구의 조건부 key 대상 제한 때문에 게임 실행 전에 중단됐다. 여섯 문자만 허용하도록 입력 검증을 확장한 뒤 같은701 제품의 실제 키 입력은 D→E→F/SP400까지 진행했지만3924프레임 상한이 세 번째 귀환 전에 끝나 종료1이었다. 기존 실패를 수정하거나 성공으로 재판정하지 않았다. 이 결과는 기존 첫 항목 고정 계획과 관측 상한의 제약을 보여 주지만 최초 D 정체의 원인을 확정하지는 않는다.

교정 제품에서는 실제 여섯 슬롯/회전/다리/사제 입력을100주기로 확장한 [UI 계획](../../cpppj/tests/fixtures/tutorial-one-harvest-six-slots-ui-input.tsv)으로 자연 Early1 A~G/SP0→200→400→600/다음Tutorial2 Guide4·SP10000을 확인했다. 초기 완료1→1, 주입0, stocktimer1, 게임 종료0이고 cycle70.bmp의 완료창/SP600/좌측 UI도 직접 확인했다. 실제 실행 계획과 저장소 계획은 주석을 제외한 모든 행이 같다. Early2의 신전/워크샾·발사기4→3/환불75/SP3075·다음Tutorial3과 ESC 두 비율도 통과했고 Early2 goal.bmp의 완료창/SP3075를 직접 확인했다. 일반31은 CLI/실제 Play를 통한 진입31/31, Demo3은 실제 메뉴/15초 자연 timeout/AI를 통한 재생3/3이다. 일반31/Demo3의 사용자 자연 완주나 모든 미션의 정상 메뉴 선택을 증명하는 결과로 세지 않는다. 여섯 실행 그룹의 제품 SHA가 같고 각 그룹 종료0·원본1394동일이다.

검사 도구는 `--plan`을 명시했을 때만 다른 입력을 선택하고 기본 계획을 바꾸지 않는다. 기본 입력/실행 명령의 출력 경로를 정규화한 전후 비교는 같으며 기본3924프레임도 유지된다. 긴 계획은 마지막 관측+4인7424프레임이다. 조건부 다른 문자와 상태 쓰기는 준비 단계에서 거부했다. 시드·완료값·SP·단계·게임 시계·타이머 결과를 직접 쓰지 않는다. 다음 명령은 같은 검증을 재현하며 `--prepare-only`를 붙이면 게임을 실행하지 않는다.

```powershell
python tools/cpp_tutorial_harvest_smoke.py --output extracted/new-early1 --plan cpppj/tests/fixtures/tutorial-one-harvest-six-slots-ui-input.tsv
```

## 남은 경계

- 현재 Client Kernel과 Session world Kernel, frontend UI의 반납 순서는 아직 원문 단일 전역 Kernel/Gump 수명과 같지 않다. 이번 검사는 실제 Session world Kernel 앞의 PlayerLinks 순서와 자식 효과를 증명한다.
- 전체 State 부모의 저장/Player1..8 reset/Priest/SP/Chat/전송/MetaZone/PlayerLinks/Kernel/Gump/Uber/Core0/Pool/표/Graph/Player 재구성/Zone/모드/조건부Core1 접합은 남는다. [전체 자식 조사](cpp-state-world-cleanup-reuse-20261024.md)의 과거 분류는 당시 이력이며 이 연결이 전체 표 완료를 뜻하지 않는다.
- Client/Login current Core·cache·active는 아직 분리돼 있다. 선행 Native 정리 없이 alias를 공유하거나 Session 소멸자에서 임의 Core0/NULL 쓰기로 누락을 숨기지 않는다.
- Editor 최종 HUD/State/loaded·편집 도구·저장/언어별 시나리오·재로드, 모든 Early/캠페인/Demo 사용자 완주와 픽셀·음향·규칙 완전 동일성은 미완료이며 목표는 계속 활성 상태다.
