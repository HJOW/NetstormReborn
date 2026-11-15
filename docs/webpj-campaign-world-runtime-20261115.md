# 캠페인 월드 설치 연결 이식 (WB04-E519)

4-D 구획 C의 `client/RawCampaignWorldRuntime`을 `webpj/www/core/raw-campaign-world-runtime.js`로
옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 `git diff --numstat 225553df HEAD`로 확인했다
(헤더 목록의 `(+2)`는 기준선 `1f00a25a` 대비 값이다).

## 옮긴 범위

- 생성자: 월드 Core·섬 목록·Player·정상 지형·HP 장부·신전 소유권·finish·내용물·로더 상태·캠페인 전역과
  상위 저장소(`data`)·Core 밖 효과(`effects`)를 받아 같은 풀·같은 HP 장부·같은 Player 9기록·9 Template/
  Territory·180바이트 header·NUL 포함 handle·필수 효과 아홉 개를 최초 등록/쓰기 전에 검사한다.
- 저장 객체 로더(`FortObjectLoad`)의 훅은 소유권(SetOwner) → 월드(Pop/Destroy/로딩 깊이) → 청크 지도의
  부호 있는 BYTE 섬 번호 → 섬 목록 영역 허용(`matches`) → finish 후처리 → 생성/HP/내용물 순으로 쌓는다.
- Fort 부모(`RawFortWorldLoad`)와 캠페인 부모(`RawCampaignWorldLoad`)는 같은 정상 지형 상위
  (`normal.generateOriginal`)·Graph 재구성을 쓴다. generic Fort 후처리(Money/Badges/Technology)는
  캠페인이 쓰지 않으므로 명시 오류로 둔다. 일반 Fort 가 주어지면 기존 완료 Money/Meta/Object child를 쓴다
  (Money 선택은 LoadSelected 안에서).
- `load()`, `loadExistingFort(beforeWorld, alternate)`, `finishMissionPriests()`, `pool`, `objects`,
  `readTerritory(slot, output120)`. 중복 실행과 실패 뒤 재실행은 거부한다.
- 이 단위에서 `fort-object-load.js`에 cpppj `MakeFortObjectLoadHooks`에 해당하는
  `makeFortObjectLoadHooks(pool, factory, hp, contained, hooks)` 팩토리를 추가했다
  (웹에 빠져 있었다). 다른 풀이면 어떤 훅도 연결하지 않고 거부한다.

## 웹 이식에서 확인한 차이

- C++은 `RawFortWorldLoad`·`RawCampaignWorldLoad`에 프레임 표를 그대로 넘기지만 웹 섬 Builder는
  타입별 프레임 코드 배열을 읽는다. 런타임이 `core.frames.map(table => table.codes)`로 같은 배열을
  빌린다(사본 아님). `FortObjectLoad`는 표(`{ codes }`)를 그대로 받는다.
- C++이 `player.State().templateOwner`의 참조를 넘기는 자리는 현재 값을 읽고 쓰는 `{ value }` 상자로
  같은 DWORD를 빌린다.
- 10.78 전용이라 `RawFortWorldLoad` 생성자의 CD용 깊이 인자는 옮기지 않았다.

## 시험

`webpj/tests/raw-campaign-world-runtime.test.mjs` 8건(보호 TYPE 188칸·실제 프레임 코드/셀 크기·보호
Tutorial 1 Territory 120바이트, 실제 월드·섬 목록·Player 월드·소유권·finish):

1. `Load`: 번역 You → 이름 config → 창 제목 → Player 생성(`L:1:11:1:0`) → 저장 객체(Session 이 쓰는
   `campaignObjects` 자리) → 카메라 순서, Builder 경계 1..15·owner 1·noRegion 127의 정상 지형 요청,
   깊이 0, 섬 0 존재, 최초 Template header +8 DWORD 1.
2. 중복 `load()`와 실패 뒤 재실행 거부.
3. 구성 거부(다른 풀의 섬/Player/지형/HP/내용물/finish, 다른 소유권 9기록, Template/Territory 수,
   180 header, 빈 handle, 필수 효과 아홉 개) 뒤 Player·섬·header 기록과 이벤트가 변하지 않음.
4. 객체 로더 훅: 신전 가상 소유자 변경이 Player 기록에 쓰임, 청크 섬 BYTE 0xfe → −2, 일반 모드 섬
   `matches`, 같은 풀 생성/HP.
5. `makeFortObjectLoadHooks`의 풀 불일치 거부와 네 훅 연결.
6. 일반 Fort alternate 1: 외곽 준비 후 populate 미연결 오류, 실패 뒤 재실행 거부, 일반 Fort 가 아닌
   연결의 `loadExistingFort` 거부.
7. 일반 Fort alternate 0: `before → objects → selectMoney → money:loadSelected → moneyStream →
   Badges → Technology` 순서와 깊이 0.
8. `readTerritory`: 120바이트 읽기·위치 +120·빈 header 칸의 진단 요청·9칸 밖 거부.

최초 실패와 교정:

- `RawContainedCreate`를 인자 없이 만들었더니 생성 경계 누락으로 거부됐다 → 같은 풀 Factory 훅
  (`makeContainedCreateHooks`)으로 구성했다.
- 합성 프레임 코드가 비어 섬 Builder가 `undefined.side`로 중단됐다 → 보호 자료의 실제 `.type` 프레임
  코드와 SHP 셀 크기를 썼다. 이후에도 같은 오류가 남아 런타임이 프레임 표 대신 코드 배열을 넘기도록
  고쳤다(위 차이 참조).
- `matches`의 일반 모드 비교를 보려 했으나 장면이 chaff 모드라 원래 owner를 비교하지 않았다 → 시험에서
  모드를 일반으로 바꿔 확인했다.
- 일반 Fort 시험의 Meta 가짜 child 에 풀이 없어 child 훅 연결이 거부됐다 → 같은 풀을 부여했다.

## 남은 경계

- 정상 지형 상위와 저장 객체 본문은 호출 형태만 대조했다. 실제 Tutorial1 저장 객체 적재와
  `session-dump` 대조는 Session 조립(D 구획)의 몫이다.
- 이 단위는 부품 대조이며 앱 수명에 연결하지 않았다(PORT_MAP `port`, session 아님).
- 다음 시작점: `RawCampaignLifetimeBindings`·`RawCampaignMissionStartBindings`·
  `RawCampaignTutorialOneBindings`.
