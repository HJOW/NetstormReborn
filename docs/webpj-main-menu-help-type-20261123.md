# 월드 없는 메인 메뉴 Help 의 TYPE 정보 (WB04-E1240)

E1228 읽기 전용 감사가 남긴 도움말 차이 — "원문은 월드가 없어도 `ReadAssetQuickHelpType`(`QuickHelpHtml.cpp:10`)으로 TYPE 정보를 읽는데 웹의 정보 패널·그림은 미션 중 경로(F1 생산 정보·링크 이동)에서만 만들어져, 메인 메뉴 Help(F1·General Help)에서 `priestType` 같은 주제로 가도 Hits/Range/Cost 표와 유닛 그림이 안 뜬다" — 를 닫았다.

## 원본 흐름

`UberGump.cpp:453`의 `help.type`은 `world_ ? world_->Session().HelpTypeInfo(type) : ReadAssetQuickHelpType(client_.Assets(), type)`이고 `help.hitPoints`는 월드 유무와 무관하게 `MakeCurrentTypeHitPoints`(장기 `SurfaceStorage` TYPE 표 + `GameAux`의 약화/사제/제단 번호 + `AiDebug` 진단)다. 주제를 열 때(`QuickHelpRequest::Open(0, 주제)`) 끝의 `type`→`help`를 차례로 떼고 `%s`·`sun%s`·`wind%s` 이름을 TYPE 표에서 찾아 정보 패널(`<info>`)과 그림을 만든다(E1229에서 미션 중에 구현한 규칙 그대로).

`ReadAssetQuickHelpType`은 표 레코드에서 `theme`·`flags1`·`flags2`·`cost`와 AST 요구 에너지(`mana`)를 읽고, 자산 TYPE(번호 70~185)이면 정의의 `range`·`hpPerSec`(float32)·`class`(23바이트 이하, 넘으면 `length_error`)를 더 읽는다. 월드가 있으면 `Session.helpTypeInfo`가 같은 항목을 현재 표(`gunTypes`·`hitTypes`)에서 읽는다.

## 웹 변경

- `ui/quick-help-info.js`에 `readAssetQuickHelpType(game, types, type)`을 더했다. 범위 밖 번호·23바이트 초과 class는 `RangeError`로 알린다(원본은 `out_of_range`/`length_error`).
- `netstorm.js`에 `_helpTypeSource()`를 만들었다. 미션 월드가 있으면 Session의 현재 표·`helpTypeInfo`·`hitPoints().typeHitPoints`, 없으면 장기 표면 저장소(`_surfaceStorage.types`)·`readAssetQuickHelpType`·`readRiftHitPoints`(현재 `aiDebug().checking()`·`gameAux().priestQuarterHp/priestType/daisType`, 진단은 `storage.assertDisplay`)를 돌려준다. `_helpTypeDisplay`(F1 생산 정보)와 `_resolveHelpType`(링크 이동·Back·첫 주제 열기)이 이 출처를 공유한다. 이전에는 월드가 없으면 `_resolveHelpType`이 곧바로 `null`이었다.
- 앱 시작 때 `_initializeClientWorldStorage`가 장기 TYPE 표를 만들므로(`netstorm.js` 2355행 부근) 메인 메뉴에서도 항상 읽을 수 있다. 출처를 못 만들면(장기 소유자 없음) 정보 없이 본문만 보이는 기존 동작을 유지한다.

## 시험

- `tests/quick-help-asset-type.test.mjs`(4건): 실제 아카이브의 188개 TYPE 전부에서 표 필드·요구 에너지·자산 TYPE의 range/hpPerSec/class를 정의와 대조(자산 116개 확인), 대표 TYPE 값(사제 범위 30·`High Priest`, 선캐논 범위 20·피해 16·비용 400·mana `s`·`Shooter`, 다리 `Bridge`)과 float32 저장, 범위 밖·NaN·소수 번호와 24바이트 class 거부(NUL 뒤는 읽지 않음), 월드 없는 정보→`buildQuickHelpInfoHtml`로 선캐논 도움말 HTML(Hits 600·Range 20·Damage 16·Cost 400)과 1/4 HP(사제만 4분의 1).
- `e2e/main-menu-help-type.spec.js`: 실제 Chromium에서 F1로 메인 메뉴 도움말을 열고(첫 주제는 정보 없음) `priestType`(그림 TYPE 158·Class High Priest·Hits 100·Range 30)·`sunCannonType`(`sun%s` 규칙으로 TYPE 128·Hits 600·Damage 16)로 이동하면 월드가 없어도 정보 생성기·그림이 채워지고, `F1Help`로 돌아가면 다시 비는지, 콘솔 오류가 없는지 확인한다.
- 영향 e2e(미션 중 링크 이동 `mission-object-commands`, `knowledge-menu`, `mission-unit-menu`, `korean-campaign`, `smoke`)를 함께 돌렸다.

## 남은 일

- 주제 80바이트 상한과 `RemoveSuffix` 범위 검사(원문 내부 검사)는 `_resolveHelpType`이 80자 이상을 정보 없음으로 처리하는 것으로 갈음했으며 별도 거부 오류는 만들지 않았다.
- 실제 원본 화면과의 시각 대조(정보 패널 위치·글꼴·그림 기준점)는 하지 않았다. 미션 중 같은 코드 경로의 화면은 E1048~E1055에서 확인했다.
