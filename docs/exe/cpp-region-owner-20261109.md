# cpppj 1206 — 섬·영역·신전의 같은 장기 본체와 삭제 부모

제품은 11.01이다. M6-n의 실제 NoWorld 연결을 이어갔으며 전체 목표는 미완료다. 직전 제품은 1204 `ffb485bc`, 직전 문서 정리는 1205 `d7c6f013`이다. 현재와 마지막 정밀 디컴파일 호스트는 모두 **HJOW-Athlon**이다(Client 양판본 2026-11-03, R 전용 2026-11-07). 원문 변경·새 디컴파일·원본 실행은 없다.

## 바뀐 실제 경로

신전의 전체 소유자 변경/삭제, 섬 소유자 전파, 지역 투표/도장이 Native Session의 짧은 조립본에 묶여 있었다. 같은 본체를 `ClientConstructionGlobals` 수명으로 옮겼다. 첫 메뉴에서는 실제 backing/장치의 정체성과 주소만 결합한다. Player 할당·지면 생성·난수 소비·캐시 초기화·창 생성은 하지 않는다.

| 범위 | 실제 연결 |
| --- | --- |
| 섬 | 같은 Storage의 3204BYTE/count/100기록·거주 구역 목록·현재 Fort 모드·실제 Kencloud redraw/Gameplay MapRevision을 `RawIslandRuntime`에 조립한다. 현재 Player getter는 첫 지역 DWORD가 선택될 때만 읽는다. |
| 영역 | 같은 Outpost/신전 목록과 현재 TYPE+98, 같은 hash0/프레임/난수로 `RawRegionOwnership::UpdateNative`와 전체 `RawRegionPaint`를 사용한다. 도장 캐시·요청·영향 목록도 Client 수명이다. 도장 요청마다 현재 terrain/edge TYPE를 읽는다. |
| 신전 소유자 | 기존 전체 `RawVortexOwner`를 같은 Player/섬/영역/공통 Owner에 연결했다. Storage의 실제 가상 소유자 분배도 같은 VT 판독을 사용하므로 신전을 공통 owner BYTE 변경으로 축약하지 않는다. |
| 신전 삭제 | 실제 `004dca50`의 두 목록 제거→신전/섬/영역→현재 Outpost 순회→실제 Damageable Pre→생산 갱신→현재 Player 네 필드 정리를 공유한다. |
| Outpost 삭제 | 실제 `00455840`의 두 목록 압축→같은 전체 영역 투표/도장→실제 Damageable Pre를 사용한다. 기존 정적 본문을 복제하지 않았다. |
| 현재 UI | 건설 Client 장치는 8개에서 **10개**다. 추가 생산 갱신은 현재 Menu의 같은 `RawButtonControls::RefreshProduction`/02000003, 테마 모드는 현재 Config의 원문 철자 `theammode`/기본 1을 사용한다. 선택한 집합/함수 사본은 호출 끝까지 보유한다. |

Native `RawCampaignSession`은 장기 섬/도장 상태/투표/신전 부모를 빌린다. `RawCampaignOwnership`의 Native wrapper도 같은 본체를 보유하며, 다른 사제의 실제 파생 소유자 분배만 짧은 wrapper에 남긴다. 기존 독립 typed/CD 경로는 기존 조립 계약을 유지한다.

## 원문과 수명

- `RawVortexOwner`/`RawVortexPreDestroy`에 현재 Player getter 진입을 추가했다. 기존 span 진입의 9기록 검증은 유지하고, Native는 원문이 선택한 읽기/쓰기 위치에서 현재 base를 다시 읽는다. owner0/extra 관문 전에 임시 Player를 만들지 않는다.
- 섬 역시 실제 현재 Player getter가 결합됐으면 독립 `playerRegions` 배열로 대체하지 않는다. 실제 Player 초기화로 배열을 반납/재할당한 뒤에도 같은 장기 본체가 새 base를 읽는다.
- `RawIslandRuntime`의 Native 진단은 같은 Storage 장치에 원문 식/행을 전달한다. Owner 28f·경계 2a4, Bounds 2ba/2c0, Temple 2dd/2f2, Owner getter 2fb, Delete 285를 대조했다. 이는 M7 중앙 진단 전체 완료를 뜻하지 않는다.
- `0046f530`은 `0041d7a0`을 먼저 호출한다. 실제 PE `00502310`의 비트 `3f7fff58` 편향→정수/float 저장→지역 조회를 기존 terrain 어댑터로 연결했다. Native wrapper의 이전 plain 절삭과 구별한다. 테마는 원문처럼 TYPE를 읽은 뒤 Config를 호출한다.
- 신전 삭제의 Outpost count도 실제 `jle`/`jl`의 signed 조건으로 읽는다. 현재 음수 count는 항목 물리나 큰 하위를 미리 조회하지 않는다.
- Globals는 실제 Storage/Gameplay/SP/Player/Random/Kencloud를 본체보다 오래 보유한다. Storage의 신전 Owner/Pre/영역 callback은 **weak owner를 선택 호출 동안만 pin**하므로 역참조 순환을 만들지 않는다. Native wrapper도 선택한 실제 owner/다른 파생 함수 사본을 호출 끝까지 보유한다.

원시 `extracted/cpp-resume-1206/original-evidence.json`에 양판본 **24함수**의 전체 C·분리 PE 범위/해시를 기록했다. `static-audit.json`과 기존 실제 생성자 83VT 목록을 재계산했다. 신전 **5196f0/5197a8/519860/519918**와 Outpost **509210**의 부모 5쌍이 늘어 현재 분배는 **66/83, 전용 부모 잔여 17, 통지 잔여 6**이다. 분배 수를 큰 하위 효과까지의 전체 NoWorld 완료 수로 합산하지 않는다.

## 검증

최종 Release 제품 2·집중 5대상 빌드는 **종료 0 / 30.54초 / 경고·오류 0**, 소스 25개 고정이다. **비MP 183/183 통과**(Owner109·Carrier8·Damageable22·Common18·Surface26). 신규 13개와 기존 관련 15개를 추가 실행했으며 전체 솔루션/MP 검사 통과를 뜻하지 않는다. [보고서](../../cpppj/region-owner-20261109.json)에 제품 바이트·원시 근거를 기록했다.

실제 Tutorial4는 UI 생산/건설 요청 뒤 **TYPE75/owner1 생성·요청 0→1**, **600프레임 대기(게임시각 13.24→23.468)**와 정상 종료 0을 확인했다. 대기 뒤 실제 화면의 미션·생산 UI/지면 표시도 확인했다. 같은 EXE의 **800×600 Edit TEST01→Main 왕복 2회**는 11관측/정상 종료 0이다. 건물 완성·Test Battle·전체 시나리오 동일성을 확인한 것은 아니다. 두 실행 모두 격리한 게임/사용자 자료를 사용했고 정확한 EXE SHA는 `d00b61caa57e366498d943351b65ed72df63d92f8af5de362073cf6d0089d974`로 일치했다.

새 비MP 공통 검사 **13개**는 다음을 확인한다.

- World/Session 없이 전체 본체를 구성해도 Player base0/난수/캐시·영향 목록이 보존되고, Storage가 장기 owner를 강하게 순환 소유하지 않는다.
- 실제 hash0와 원문 snap, 5개 실제 생성자 VT의 삭제/공통 깊이/풀 반납, 실제 지면 owner와 거주 구역/Player 첫 지역 전파를 확인한다.
- 실제 공개 Player 초기화가 배열을 반납/재할당한 뒤 신전 Owner와 UI 후 신전 Pre가 현재 base를 읽는다. 선택한 Player가 없으면 앞선 Owner 쓰기를 보존하고 명시 실패한다.
- 실제 Outpost/신전 두 목록 제거 뒤 전체 투표·도장이 같은 지면을 바꾼다. 현재 TYPE/Config 재조회와 signed 음수 목록도 확인한다.
- 현재 UI 함수 교체 중 선택한 함수가 살아 있고, 장기 owner 반납 뒤에는 두 목록 제거/삭제 접두의 부분 효과를 보존한 채 실패한다.

최초 빌드의 새 검사 코드는 비공개 Player 메서드와 const Entries 쓰기로 실패했다. 공개 `Initialize`/`Records`·실제 Hash Cell을 사용하도록 교정했고 제품 공개 API를 검사 목적으로 늘리지 않았다. 실제 초기화는 이전 Player를 해제하므로 해제된 span을 읽는 초안 전제도 제거하고, 새 base/새 필드·원래 처리 순서를 직접 확인했다. 동맹 자식도 기존 `RawPlayerParticipation`의 실제 mask 본문을 사용한다.

최초 실행은 **183 중 180통과/3실패**였다. 장치별 진단으로 세 사례가 실제 공통 Refund의 Player 지갑 callback에서 멈췄음을 확인했다. 이후 원문이 필요한 실제 Player 초기화와 기존 `RawAiWallet`·`RawAiObjectNotice`의 NULL brain 자연 반환을 연결했다. 환불은 실제 분산 SP를 초기화/가산하고, SP 표시와 소리 장치만 호출을 관찰한다. 가짜 AI 본체/성공 callback을 넣지 않았으며 정상 삭제는 지갑/SP/AI 통지 호출과 AI 할당 0도 검사한다. 원시의 `first-*`, `diagnostic-*`, `before-alliance-child-*`, `before-real-refund-devices-tests.cpp`에 실패·교정을 보존했다.

허용 GUI는 Tutorial4 일반 생산/건설 요청·600프레임 대기와 같은 Client의 Edit TEST01→Main 왕복이다. 포획/희생·특정 적 공격·시나리오 승리·공중 3종 전용 관측·새 MP/패킷/통신/기록 검사는 실행하지 않는다. 원본/CD/fixture·AGENTS의 보호 2800파일 불변과 정확한 제품 바이트를 확인한다. 전체 34/CTest/ZIP 재검증으로 확대하지 않는다.

## 남은 작업

M6-n의 전용 **17부모/6통지**, 다른 Form/Regular, 큰 비영 AI, 다른 배치 호출자/Fort 상위 Finish와 전수 Source/listener/두 등록표/Poll/25태그/무태그63F·Snapshot/debugBurst가 남는다. 실제 지연큐(M6-l)·JoinTestBattle(M6-m), M7의 5건/M8, K1~K5·S1~S4·B1~B4의 **상위 16계획/버그 34건**도 그대로다. 큰 건설 효과 16개는 여전히 실제 Runtime의 약한 대여를 선택한다. NULL brain 관문 검증을 큰 AI 복원 완료로 세지 않는다.
