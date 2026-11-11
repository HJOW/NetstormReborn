# webpj 캠페인 월드 적재 부모(RawCampaignWorldLoad) — WB04-E287 (2026-11-11, 4-B)

3.1절 4-B 의 **RawCampaignWorldLoad**(144줄)를 `core/raw-campaign-world-load.js` 로 옮겼다. 원본 `4b5300`(실제 미션 새 login 의 저장 월드 부모)이며 기준선(`1f00a25a`) 뒤 `RawCampaignWorldLoad.cpp/.h` 에는 변경이 없다(3.6절 ①). generic Fort 메뉴 부모(`RawFortWorldLoad`, E286)와 별도 몸체이고 Money/Badges/Technology 를 호출하지 않는다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 선행: E286(Territory getter)·E55(RawPlayerWorld)·E222(RawIslandBuilder)·E161(RawIslandRuntime).

## 1. 규칙

`load()` 순서: ① 현재 `mySubscriber` 를 `loginSubscriber` 로 맞춘다 → ② 현재 Template owner 의 Template header(+8 DWORD)를 1 로 쓴다(9기록 밖은 거부) → ③ 번역키 "You" 의 NUL 포함 결과를 현재 mySubHandle 저장소에 복사하고 `fort.mySubHandle` 설정(NUL 포함 현재 handle)과 창 제목 갱신을 요청 → ④ Player 생성(현재 battleOwner) → ⑤ builder(owner 1·경계 1..15)를 만들고 편집(editor) 전역이 켜져 있으면 현재 Player 기록의 첫 지역 DWORD(+0x78)를 0 으로 쓴다 → ⑥ **현재** Template owner 의 Territory 120BYTE → PlaceAll(0, 20) → Generate(현재 noRegion) → **현재** Template owner 로 Objects → Graph 재구성(false) → 미션 후처리(missionPost 가 0 일 때만 사제 준비 부모) → 카메라 재설정 → 현재 battleOwner 의 Player 기록 `+0xc` 에 현재 handle 이름(NUL 포함)을 복사한다. 이름 복사는 NUL 이 없거나 대상(64BYTE handle / Player 기록 꼬리)을 넘으면 거부한다. 각 단계 직전에 두 owner 를 다시 읽는다.

`CampaignWorldGlobals`: `mySubscriber`(00540c74)·`loginSubscriber`(0054265c)는 독립 값이거나 `CampaignWorldGlobals.borrow({ my, login })` 로 외부 `{ value }` 상자를 빌린다(빌리면 초기화 쓰기를 하지 않고 읽기/쓰기가 상자로 전파됨), `editor`(005c85a4)·`missionPost`(00594fa4, 기본 true)는 값이다. 10.78 전용이라 CD 의 깊이 훅은 없다.

## 2. 검사 (`tests/campaign-world-load.test.mjs` 3건)

- `campaign-worldload-x86.tsv` **originals 65행**: 실제 Player152 생성(보호 Factory·Owner·Post)·Builder·Territory(profile 3 은 보호 WarBegins Fort 의 Territory 120BYTE)·3204BYTE 섬 목록·16×16 청크·Template 9 header·mySubHandle·Player 9기록을 같은 입력에서 재생해 실행 뒤 전체 현재 BYTE·두 owner(template/battle)·editor·pos/end/depth·하위 요청 순서(번역 `Y` · 설정 `S` · 제목 `V` · Player pop `L` · 생성 `G` · Objects `O` · 그래프 `R` · 카메라 `K` · 진단 `A:E`)와 진단 수가 같다. 원본 Native 로그의 내부 진입 계수(`C`·`A`·`P:0:20`·`F:1`)는 공개 하위 요청이 아니라 제외한다(cpppj `CampaignExternalEvents`). 변이 10종이 단계 직전의 owner/editor/handle 재조회를 구별한다. **첫 실행의 유일한 실패는 이 내부 계수 제외를 빠뜨린 것이었고 제외 후 65행 모두 통과했다.** CD 65행은 웹 목표 밖.
- 계약 2건: 전역 묶음(독립/빌린 상자·전파·`borrow` 거부)과 구성 거부(다른 풀의 islands/territory·필수 하위·header 180BYTE·handle 비어 있음·표 크기·전역 종류), missionPost 0 의 사제 준비 부모 요구와 mission 1 자연 반환, 9기록 밖 template owner 거부(header 불변), 이름 복사의 NUL 누락/물리 범위(메시지로 구별) / Territory 조회 칸은 setConfig 가 바꾼 현재 template owner, Generate 는 현재 noRegion, PlaceAll 은 20번째 영역(오프셋 114)까지, 사제 준비는 Graph 뒤·카메라 앞, 카메라가 battleOwner 를 9로 바꾸면 마지막 복사 전에 거부.
- 변형 확인 38개 중 처음에 11개가 살아남았다. 계약 두 건으로 10개를 잡았고 남은 1개(`handle NUL 검사`)는 **도달 불가한 방어**다(번역 복사가 항상 NUL 을 쓰므로 직후의 handle 에 NUL 이 없을 수 없음).

## 3. 남은 경계

- cpppj `CampaignWorldLoad_*` 외 `RawPlayerWorld` 의 첫 Pop/최초 Regular 연결은 4-D Session 조립에서 한다(이 시험은 Player 첫 Pop 을 관찰 훅으로 남김). 미션별 후처리 부모(0x923e0 사제 준비 등)는 `preparePriests` 훅이며 8단계(튜토리얼/미션)에서 연결한다.
- 3.1절 4-B 는 18/31 이다. 다음은 **Islandbuilder**(partial 117줄)이다.
