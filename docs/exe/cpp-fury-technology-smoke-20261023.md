# Fury War의 네 원소 기술 선택 검증 도구 (단위376)

이 문서는 [새 도구](../../tools/cpp_fury_technology_smoke.py)의 독립 원본 기대와 실제 GUI 관측을 구별한다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. 원본·CD·AGENTS·디컴파일 산출물을 변경하거나 원본 게임을 실행하지 않았다.

## 독립 원본 기대

기대는 클론의 소스·덤프·실행 보고서를 사용하지 않는다. 보호 `originals/Netstorm.exe`의 TYPE 로딩 순서 포인터 `00540dd0`(116개), group 이름 포인터 `00542468`(10개), 디스크 우선/TAFF 보완 VFS의 각 `.type`, `bc8fury76.english`를 독립 Python 판독기로 읽는다. 원본 실행 파일 SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, Fury 스크립트 SHA-256은 `8cf9f00538b69800e02cf329114eb39a32cec58b3ca0a0e0265e9aa34f3c697e`다. 실제 각 검사 실행은 이 값과 모든 `.type` SHA를 다시 기록한다.

`CheatAllTechnology`는 원본 명령 `02000028` / 몸체 `00492ae0`다. 전체 TYPE 순서로 `group != 10 && theme != 5 && theme == argument`인 항목을 조회한다. 현재 기술 부모의 `FindContained(mask4,type,index0)`가 없을 때만 현재 부모를 다시 조회하고 `Create(parent,type,0,4,0x10,0)`을 호출한다. 즉 기존 기술은 지우지 않고, 같은 타입이 있으면 새 내용물을 만들지 않는다. 클래스 이름이나 일반 생산 가능 여부로 필터를 대체하지 않는다.

Fury의 초기 `myTech`는 `outpost`(126)와 `windBattery`(75)다. `[A.]`의 `Lets do this!` 뒤 `[@1]`의 Wind/Rain/Thunder/Sun은 각각 Tell W/R/T/S를 실행한다. 네 본문은 `<$CheatAllTechnology,0/1/2/3>` 한 줄뿐이다. 같은 명령이 `ai2priestdead`~`ai5priestdead`에도 있으나 이때는 실제 표시 본문이 있으므로, 기술 명령 함수 자체가 모든 안내창을 닫아서는 안 된다.

| 선택 | 원본 지급 대상(TYPE / 파일명 / group) | 기존 일치 | 최종 기술 수 |
| --- | --- | --- | --- |
| Wind / 0 | 75 windBattery / battery, 137 windWalker / walker, 139 windAviary / aviary, 140 windArcher / archer, 141 windBalloon / balloon, 142 windBlocker / blocker | windBattery | 7 |
| Rain / 1 | 76 rainBattery / battery, 129 rainCannon / cannon, 143 rainAviary / aviary, 145 rainFence / fence, 146 rainBalloon / balloon, 147 rainBlocker / blocker, 156 rainWalker / walker | 없음 | 9 |
| Thunder / 2 | 77 thunderBattery / battery, 79 thunderBlocker / blocker, 131 thunderCannon / cannon, 135 bulf / walker, 148 thunderArcher / archer, 149 thunderFence / fence | 없음 | 8 |
| Sun / 3 | 71 sunArcher / archer, 72 sunAviary / aviary, 78 sunBlocker / blocker, 111 sunBalloon / balloon, 128 sunCannon / cannon, 133 sunWalker / walker, 136 sunFence / fence | 없음 | 9 |

같은 원소지만 group10이라 지급하지 않는 항목은 다음과 같다. Sun에서 `outpost`는 초기 기술로 유지될 뿐 이 명령이 지급하는 항목이 아니다.

- Wind: windFactory(86), windVortex(123), windFlyer(138).
- Rain: rainFactory(87), rainVortex(124), rainFlyer(144), growingRainBlocker(150).
- Thunder: thunderFactory(88), thunderVortex(125).
- Sun: sunFlyer(73), emptyGeyser(84), sunFactory(85), treeTwo(100), treeThree(101), edgeFarm(121), geyser(122), outpost(126), RUIN(184), Monument(185).

group 표는 archer0/cannon1/blocker2/aviary3/flyer4/battery5/fence6/walker7/balloon8/misc9이며, 미지정은10이다. 따라서 `.type`의 `class="Aerial Attack"`만 있는 실제 Flyer는 포함하지 않는다. theme는 원본 `00472e60`의 첫 BYTE 조회 의미를 보존하며 대소문자를 임의로 통일하지 않는다. 내장 TYPE0..69는 초기 theme5이므로 이번0..3 선택에서 제외된다.

## 실제 입력과 판정

```text
python tools/cpp_fury_technology_smoke.py --output extracted/cpp-fury-technology-20261023-final
```

기본은 네 분기를 독립 프로세스로 순차 실행한다. `--theme Sun`처럼 한 분기만 지정할 수 있으며 동일 테마 중복 지정은 거부한다. `--prepare-only`는 기대·계획만 만들고 GUI를 열지 않는다.

각 분기에서 A.12프레임을 `require`/BMP로 관측하고13프레임에 시작 버튼을 한 번 누른다. 88프레임의 @1을 `require`/BMP로 관측하고89프레임에 선택한 원소 버튼을 한 번 누른다. `require`는 현재 페이지를 확인하고, 이어지는 실제 `click`은 현재 컨트롤이 존재해야 진행한다. 같은 관측의 실제 컨트롤과 활성 여부도 최종 판정에서 대조한다. 수만 행 전체를 가로지르는 탐욕 정규식을 추가하지 않는다. 네이티브 버튼은 같은 입력 큐에서 Tell까지 소비하므로 @1의 호스트 phase가 Dialog1일 수 있다. 이를 이전 어댑터의 Briefing3으로 강제하지 않는다. 최종 Mission4·비정지 및 초기/최종 기술 불변 판정은 그대로 유지한다. 90프레임의 명령 직후와108·133프레임의 재개 상태를 기록한다. 실제 루프의 `sleepPerLoop=30`과 자연 프레임으로 @1 시한 및 일곱 프레임 재개 관문을 기다리며 상태·시계·완료·SP·기술·미션 스크립트는 직접 쓰지 않는다.

기존 진입 감사 도구의 각 후보 라벨/`when`마다 전체 `Report()`를 직렬화하던 비용을 피한다. 새 도구는 `when`을 사용하지 않고, 정해진 두 `require`와 네 `report`만 사용한다. 보고서를 읽을 때도 수천 개 월드 객체는 행 수만 세고 실제 기술 내용물 반복 행·선택 컨트롤·필요 필드만 한 번 순회한다. 실제 입력 시점에 기대 브리핑이 없으면 실패하며, 다른 시점에 버튼을 반복해서 눌러 통과시키지 않는다.

성공에는 최종 정상 종료/상한 미도달, 해당 미션/원본 월드, Guide started1/sequence4, Mission/비정지, scriptPaused·lockFrames·clearFrames0, 로딩 부모 반납, 원소 선택 컨트롤 제거와 게임 시각의 자연 증가가 모두 필요하다. 기술은 독립 기대와 TYPE 순서까지 대조하며, `raw_technology`의 존재 조회만으로 중복 없음으로 판정하지 않는다. 실제 mask4 `raw_production_content` 전체의 타입별 개수가 각각1인지, 두 초기 내용물 SID가 유지되는지, 새 내용물 flags4/quality0인지, 재개 후 최종까지 기술/SID가 안정적인지도 검사한다.

각 실행의 명령·PID·실행 파일 SHA·종료 코드/상한/벽시각·로그 SHA·계획 원문/SHA·BMP/SHA·원시 `states.tsv`·추린 `observations.json`을 보존한다. 독립 기대는 `original-expectations.json`, 실제 관측은 각 분기의 별도 파일이다. 실행 전후 `originals` 전체 SHA 목록을 각각 저장하고, 성공·실패·예외 모두 허용 설정의 원래 바이트/존재 상태를 복구한 뒤 전체 해시를 대조한다.

## 준비 확인

최종 준비 결과는 `extracted/cpp-fury-technology-20261023-prepare-native`에 있다. 이전 `...-prepare`는 phase3을 가정했던 최초 준비로 보존한다. **정적 기대 생성·네 입력 계획 생성·UTF-8/AST·공백 검사는 통과했고 전체 원본 해시는 같다. 두 준비 실행에서 GUI/빌드/원본 게임은 실행하지 않았다.** 기존 단위374의 실제 실패 보고서(9,149,762바이트)를 새 스트림 판독기로 읽어, 초기 mask4 두 내용물 SID164/165 및 최종 @1/정지 상태를 제대로 구분하는 것도 확인했다. 이는 수정 후 런타임 통과 증명이 아니다.

## 수정 제품의 실제 관측과 독립 재검토

실행 제품 SHA-256은 **`d53095e8f6f3cea34b9f086023d89dc06d046053f917a859185e8131e8d111bd`**다. 루트 작업자가 GUI를 실행했으며 검토 작업자는 새 게임을 실행하지 않고 원시 보고서·실행 메타데이터·전체 해시 목록·BMP를 읽었다.

최초 네 분기 실행은 `extracted/cpp-fury-technology-20261023-final`이다. Wind/Thunder/Sun은 각각18조건 통과·종료0/상한 미도달이다. **최초 Rain은 실패**로 보존한다. 종료1/0.897초, stderr 한 줄 `Unable to open d/options.cfg`, `states.tsv` 0바이트, BMP0개이며 실제 초기 브리핑/선택/기술 판정에 도달하지 못했다. 특정 Rain 기술 분기의 실패로 단정할 근거도, 이 실패를 성공으로 바꿀 근거도 없다. 옵션 파일 열기 실패의 근본 원인은 미확정이다.

Rain만 같은 제품/설정/입력 순서로 별도 실행한 `extracted/cpp-fury-technology-20261023-rain-retry`는 **18조건 통과·종료0/상한 미도달**이다. 처음 네 실행의3성공/1실패와 별도 Rain 성공을 합쳐, 네 종류 선택 모두에 정상 진입 관측이 있다는 의미로만 판단한다. 최초 실패 결과를 삭제하거나 자동 재시도 결과로 덮어쓰지 않았다.

| 실제 성공 관측 | 정상 종료까지 벽시각 | 최종 mask4 내용물 수 | 108→133프레임 게임 시각 | 원시 경로 |
| --- | --- | --- | --- | --- |
| Wind | 9.186초 | 7 | 2.163→3.857초 | `...-final/wind` |
| Rain 별도 재실행 | 8.636초 | 9 | 2.047→3.626초 | `...-rain-retry/rain` |
| Thunder | 8.849초 | 8 | 2.174→3.736초 | `...-final/thunder` |
| Sun | 9.112초 | 9 | 2.154→3.827초 | `...-final/sun` |

도구의 성공 플래그만 읽지 않고, 원시 `states.tsv`를 별도로 분해하여 다음 사항을 재검토했다.

- 모든 성공 실행의12프레임 A.와88프레임 @1에서 현재 기술은 정확히 windBattery/outpost이며, 부모 SID5와 초기 내용물 SID165(TYPE75)/164(TYPE126)가 같다. 시작13/선택89의 실제 클릭 두 개뿐이고 테마를 다시 선택하는 입력은 없다.
- 90프레임에는 선택 부모가 사라져 phase4/page mission이며 새 기술 내용물이 이미 있다. 이 시점은 아직 `scriptPaused=1, clearFrames=2`다. 이를 즉시 비정지 통과로 처리하지 않는다. 108·133프레임에는 Guide started1/sequence4, phase4/page mission, paused0, lockFrames/scriptPaused/clearFrames0이고 실제 게임 시각이 증가한다.
- 최종 실제 내용물 TYPE별 개수는 위 독립 원문 기대와 정확히 같고 모두 한 개다. 기존 SID164/165가 유지되며, 새로운 항목은 SID230부터 TYPE 오름차순으로 부착되어 flags4/quality0이다. 108→133프레임의 기술/SID도 같다. Wind는 기존 windBattery를 새로 만들지 않았다.
- 두 실행 묶음 각각에서 `originals-before.json`과 `originals-after.json`의 **1,394파일 전후 SHA가 모두 같다.** 두 묶음의 독립 기대 JSON도 같다. 각 `running.json`/결과의 실행 SHA가 같고 계획 SHA는 실제 파일 해시와 일치한다.
- 성공 실행 네 개는 A./@1/재개/최종 BMP가 각각4개, 모두1024×768이며 보고서에 기록한 SHA와 같다. A./@1의 실제 표시와 최종 모달 제거를 시각적으로 확인했다. 이 화면들은 **원본 픽셀 대조 자료가 아니며** 기술 체인 수/명령 의미는 별도 원문 기대와 원시 상태로 판정한다.

최종 GUI 명령은 일반 `--run originals --mission bc8fury76 --window --no-audio --frames 134`, `SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=30`, 해당 `--ui-script`/`--ui-report`다. 검사 전용 상태 주입 인자는 없다. 성공한 네 stderr는 각각340바이트로 동일한 미션/로그인 진단과 `No Central Island.`를 포함한다. 미연결 CheatAllTechnology 경고나 다른 예외 문구는 없다. `No Central Island.` 진단을 숨기거나 전체 원본 동일성의 성공 근거로 사용하지 않는다.

## 남은 경계

이 도구는 각 분기 선택을 한 번만 하므로 **재선택/명령 재적용의 멱등성, 적 사제 희생의 실제 안내 본문 유지, Fury 자연 완주·승패·전체 픽셀/사운드 동일성을 검증한 것으로 확대하지 않는다.** Wind의 초기 중복 내용물이 보존되는지와 모든 테마의 최종 중복 개수 검사는 포함한다. 기술 명령 재적용과 실제 버튼 부모 지연 종료 계약은 별도의 원본 fixture/단위 검사 및 수명 관측과 함께 판단한다. 최초 Rain의 옵션 열기 실패도 미확정 원인으로 계속 보존한다.

## 단위376 후속 전체 진입

`extracted/cpp-fury-technology-20261023-solo-all/results.json`의 **Early6·Priest Training9·캠페인16 = 31/31** 진입 성공을 독립 재검토했다. 선택 목록31개와 실행31개는 중복/누락 없이 일치하며, 각 원시 `states.tsv`의 최종 필드를 JSON과 별도로 대조했다. 모두 같은 제품 SHA `d53095e8...111bd`, 종료0/상한 미도달, 해당 미션의 Guide started1/sequence4·실제 월드·phase4/비정지·로딩 부모 반납이다. Fury도 최종 page mission/게이트0/실제 객체6,418개와 Sun 기대9기술을 확인했다. 결과 JSON의 전체 원본 전후 해시 일치 표식은 true다.

별도 `extracted/cpp-fury-technology-20261023-demos/results.json`도 완료/3성공/전체 원본 전후 해시 일치 true이며, DemoStormRages·GuardMyBack·DissolvedAlliance 세 원시 최종 TSV를 독립 대조했다. 같은 제품/종료0/상한 미도달, Guide started1/sequence4/demo1, phase4/비정지, 로컬 owner1의 실제 AI 포인터와 월드/로딩 반납을 확인했다(실제 객체 각각2,902/3,667/3,419개). 따라서 이번 제품은 **전체34진입 경로의 정상 진입 관측**을 갖췄다. 루트 작업자는 모든 진입 실행 뒤에도 최초 `...-final/originals-before.json`의1,394파일과 현재 원본 전체 SHA가 같음을 다시 확인했다. 앞의 최초 Rain 옵션 열기 실패는 별도 실패 기록으로 그대로 남긴다. 이 진입 성공을 자연 완주·승패·사용자 전체 플레이·원본 전체 동일성 완료로 확대하지 않는다. 진행 중인 전체 CTest도 여기의 GUI 진입 결과와 별개다.
