# webpj 공통 객체 명령·Template 수명(RawObjectCommand·RawTemplateLifetime) 이식 — WB04-E123

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물세 번째 부분이다. 공통 객체 명령(004ae010)과 Form 부모 명령(004ac9b0)의 분기·가상 인자, Template9 20바이트 헤더의 초기화·반납·전역 정리(004bd2e0/004bf690/004bfb00)를 cpppj 기준선 `1f00a25a` 의 `o/RawObjectCommand`·`o/RawTemplateLifetime` 에서 옮겼다. 선행은 `SidPool` 뿐이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-object-command.js` — `RawObjectCommand(pool, state, hooks)`

- `command(parent, code)`: 명령 코드의 상위 6비트를 제거(`code & 0x03ffffff`)해 처리하고 처리 1 / 미처리 0 을 반환한다. 소유자/생존 필터는 추가하지 않는다.
  - 소유자 명령(0x030000af): 가상 +74 에 현재 로컬 소유자 전체 DWORD 를 전달한다.
  - 피해 명령(0x030000e2): 편집기이거나 미션 모드일 때만(단락 평가 순서 유지) 슬롯 +1a 의 HP DWORD 를 1 감소(32비트 감김)한 값으로 가상 +98 에 피해(소유자 0, 공격자 0)를 전달한다. 제한된 모드에서도 처리 1 을 반환한다.
  - 삭제 명령(0x03000015): Form 가상 +10 을 flags 0 으로, 진단 명령(0x0300006e): Form 가상 +70 진단을 부른다. 알 수 없는 명령만 0 이다.
- `state` 는 `localOwner()`·`editor()`·`mission()`(호출 시점에 읽음)과 명령 토큰(생략하면 기본값)이며, 선택된 효과 훅이 없으면 도달 시점에 거부한다.

`www/core/raw-template-lifetime.js` — `initializeTemplateHeader`, `releaseTemplateHeader`, `clearTemplateHeaders`

- 헤더 20바이트의 [0] 은 manager 포인터(불투명 핸들), [4] 는 빌림 DWORD, 나머지는 보존 필드이다.
- `initializeTemplateHeader(header)`(004bd2e0): 이전 포인터를 반납하지 않고 정확한 20바이트를 0 으로 쓴다(다른 폭은 거부).
- `releaseTemplateHeader(header, hooks)`(004bf690): 포인터가 비영이고 빌림 DWORD 가 0 인(소유한) 헤더만 `destroy` → `release` 한 뒤, 포인터가 0 이어도 20바이트를 비운다. 소멸이 헤더 포인터/빌림 값을 바꾸어도 처음 저장한 같은 manager 를 반납한다.
- `clearTemplateHeaders(headers, hooks)`(004bfb00): 9칸(180바이트)을 차례로 훑으며 비영 포인터인 칸만 반납한다. 포인터 0 인 칸의 stale 필드는 보존하고, 앞 칸 소멸 효과가 바꾼 뒤 칸의 현재 포인터/빌림 값을 순서대로 읽는다.

## 2. 검증

- `tests/object-command.test.mjs`(2건): `object-command-x86.tsv` 의 `originals` 160행(`originalCD` 160행은 10.72 의 WORD HP/무조건 Hurt 라 제외) — (명령 종류, 상위 비트, 편집기, 미션, 슬롯 HP, 로컬 소유자) 입력으로 반환과 가상 효과(owner/hit/destroy/describe) 인자 순서가 같고 슬롯 HP DWORD 는 변하지 않는다. 계약: 제한된 피해/미처리는 외부 효과를 요구하지 않고 현재 모드를 매번 읽으며(미션 조회 횟수까지 확인), 소유자 명령의 효과 누락/판본 오류를 거부한다.
- `tests/template-lifetime.test.mjs`(2건): `template-lifetime-v2-x86.tsv` 240행 전부(전역 정리 "A"·단일 반납 "S"·초기화, 소멸/free 직전 변이 포함) — 전체 196바이트 정리 결과와 소멸(D)/free(F) 직전 전체 바이트 호출 순서가 같다. 계약: 다른 폭 거부, 소유한 헤더의 효과 누락 거부(헤더 불변), 빌린 헤더는 소멸 없이 비움, 빈 포인터 칸의 stale 필드 보존.
- 구 관측 `template-lifetime-x86.tsv` 는 v2 가 대체한 자료이므로 사용하지 않는다.

최초 실패와 교정: 새 두 모듈은 모든 fixture 행을 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 가상 소유자(+74)·피해(+98)·삭제(+10)·진단(+70)과 DataManager 소멸/할당 반납은 훅 경계이다. 캠페인 템플릿 저장소(`CampaignTemplateStorage`, State 섹션 확장 포함)는 별도 client 단위이다.

## 4. 다음 시작점

`RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`(실제 자산 필요), `RawWindAviaryProcess`·`RawWindFlyer`, 비행체(`RawFlyerProcess`·`RawFlyerFrame`·`RawFlyerTarget`), 번개 수명/프레임/화면, `RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`, Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
