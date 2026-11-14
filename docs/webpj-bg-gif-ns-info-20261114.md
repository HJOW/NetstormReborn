# webpj 배경 GIF 창·NSInfo 전역/부착/생성 BackgroundGifGump·ClientNsInfoGlobals·HudNsInfo·HudNsInfoCreate (WB04-E444, 2026-11-14)

RawMenuControls 선행 **BackgroundGifGump 83줄·ClientNsInfoGlobals 47줄·HudNsInfo 75줄·HudNsInfoCreate 168줄(기준선 `1f00a25a`의 .cpp/.h 줄 수, 합계 373줄, todo→port)**을 새 모듈 [`www/ui/background-gif-gump.js`](../webpj/www/ui/background-gif-gump.js)·[`www/core/client-ns-info-globals.js`](../webpj/www/core/client-ns-info-globals.js)·[`www/ui/hud-ns-info.js`](../webpj/www/ui/hud-ns-info.js)·[`www/ui/hud-ns-info-create.js`](../webpj/www/ui/hud-ns-info-create.js)로 이식했다. 기준선 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약·손 계산 값·실제 기존 처리기(GumpCreate·GumpPixelTarget·GIF 해독기)와 실제 보호 자료(`originals/d/Defeat.gif`, 읽기만)로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

- BackgroundGifGump·HudNsInfo·HudNsInfoCreate: 기준선 뒤 변경 없음.
- **ClientNsInfoGlobals: 기준선 뒤 7커밋(`080c977d6` 등)**은 메타 루트·UDP 검색·서버 목록 갱신/저장과 체크섬(`UpsertServer`·`SetServerValue`·`RecalculateServerChecksum`·`ReadServerFields`)·참가자 정렬 40칸·passedDiags·루트 방송 포트·메타 메뉴 명령 칸(Leave/Kickoff/Sort/SetOption/Exclude/Restore)이다. 모두 온라인 멀티플레이 몫이라 웹에서 제외(3.6 ①)로 판정했고 대응표 base 를 `080c977d`로 올렸다. 이 판정으로 기준선 `LoadServerNames`/`ResolveServerName`의 순수 단일 플레이 동작만 옮겼다(`LoadServerNames` 뒤 체크섬 재계산도 웹에 없다).

## 구현

- **`BackgroundGifGump`(VT 0x501b60, 0xb0 바이트 파생)**: 노드는 종료 입력 세 DWORD(`event`/`first`/`second`)와 소유 원시 GIF 버퍼 토큰(`bitmap`)을 가진다. `construct`는 같은 파생 주소 확인 → 기본 생성(0,0,0,1) → 가상 표·종류(signed BYTE 부호 확장 DWORD)·`bitmap` 0 → `refresh` → 종료 입력 3필드 0 순서다. `refresh`는 이전 버퍼를 반납하고 0 으로 쓴 뒤 경로 규칙·존재 조회·적재(`outputs.width/height` 두 실제 출력 DWORD)를 하고 기본 크기는 정수의 float32, 표시 크기는 정수×현재 배율의 **배정도 곱을 float32 한 번**으로 저장한다. 파일이 없으면 오류가 아니라 bitmap 0·치수 0 이다. `setEvent`는 `event`→`second`→`first`→플래그 `|0x600` 순서, `reattach`는 같은 현재 x/y 로 부모 0·모드 0 부착이다. `handle`은 종료 입력이 있고 해제 비트(0x80000000)가 없으며 마스크(0x03ffffff) 뒤 코드가 0x10000/0x20000/27 이면 **호출 전 노드 값**으로 큐에 보내고 1 을 돌려주며(큐가 노드를 지워도 다시 읽지 않는다) 나머지는 기본 입력으로 위임한다. `draw`는 bitmap 이 없으면 위치도 읽지 않고, 있으면 위치 뒤 노드를 다시 읽어 **기본 크기**(배율 크기 아님)의 ftol 로 폭/높이/행 간격/소스 사각형 `(0,0,폭,높이)`을 복사 요청한다. `destruct`는 가상 표 재기록 → bitmap 반납 → 기본 소멸이며 bitmap 필드는 0 으로 쓰지 않는다. 노드는 쓰기마다 다시 조회한다.
- **`ClientNsInfoGlobals`**: PE 정적 초기값(종류 6/37/60, zone 형식 0x50c894), 80바이트 이름 버퍼, 시작 서버 행(바이트 문자열). `loadServerNames`는 개행 분할·모든 CR 제거·빈 행 건너뜀·행 256바이트(CR 포함) 초과 시 RangeError(기존 목록 유지)다. `resolveServerName`은 선택 -1 이면 `translate("LAN Server")`를 NUL 앞까지 복사(80바이트 이상이면 RangeError, NUL 뒤 기존 바이트는 지우지 않음), 유효 행이면 버퍼를 0 으로 채워 행(NUL 앞)의 첫 `;` 뒤~다음 `;` 앞을 최대 79바이트 복사, 그 밖의 선택은 기존 바이트를 보존하며 반환은 버퍼의 NUL 앞 부분을 빌린 `Uint8Array`다.
- **`HudNsInfo.attach`(0047e9d0)**: 기본 부착 → 현재 배경 Push → 폭(float32)·표시 폭(배경 배율 곱 float32)→ 높이 → 배경 부착(0,0,token,0) → 메타맵과 플레이어 인식이 모두 켜져야 전체 메타맵 배치를 호출한다. 두 전역은 값 객체/조회 함수 모두 소비 시점에 직접 읽고, 배경은 축마다 다시 조회한다(Push 나 첫 조회가 바꿀 수 있다).
- **`HudNsInfoCreate`**: `createHidden`→`construct`가 배경→명단(스크롤·`textFlags|=8`·scrollable/slider)→제목(zone 형식 번역·서버 이름·`formatZone`·`parseScroll`)→위/아래 구분선→BattleInfo(22쌍 `[0,1]`·모드 2·스크롤 제목/일반 요약/스크롤 본문)→플레이어 8개(스크롤 글·16×15 특수 버튼 `+ac|=6`·마스터 글·Start 돌, 버튼 TYPE 0 이면 진단 1912 후 계속)를 항상 생성한다. 종류 전역은 signed BYTE 를 부호 확장해 쓰고, visible 비트는 명단·제목·두 구분선·전투 정보만 끈다(배경/플레이어/나머지 글은 켠 채). 할당이 0 이면 하위 생성자를 건너뛰고 0 토큰 부착은 `실제 파생 저장소 없음` Error 다. `attachPlayer`/`attachBattle`은 공통 부착 뒤에만 메타맵 관문을 읽고 첫 값이 0 이면 둘째 전역을 읽지 않는다. `current`는 `{ value }` 상자, 플래그 참조는 `{ value }` 상자로 빌린다.
- 웹 적응 표기: C++ `invalid_argument`→`TypeError`, `length_error`→`RangeError`, `runtime_error`/`logic_error`→`Error`, 참조 출력 DWORD→`{ width, height }`/`{ value }`, 미선택 효과는 도달한 위치에서만 요구한다.

## 검사와 발견

새 시험 파일 2개, **45건 통과**(`background-gif-gump.test.mjs` 23건·`ns-info.test.mjs` 22건). BackgroundGifGump 는 실제 `GumpCreate` 위의 기록 틀로 호출 순서·필수 장치·노드 재조회를 보고, 실제 `originals/d/Defeat.gif`를 실제 `decodeBackgroundGif`와 `GumpPixelTarget.rawBitmap`에 연결해 위치·클립 안에서 색 번호가 그대로 복사되는 것까지 확인한다. HudNsInfoCreate 는 기준선 소스의 호출 순서를 시험 쪽에서 따로 손으로 펼친 **전체 호출 기록 하나(플레이어 8개 포함)**와 비교한다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류):** 배율 0 인 노드에서 `-5×0=-0` 이 나와 기대값을 배율 1 로 맞췄다. 코드 마스크 위 비트(0x04000000·0x40010000)는 해제 비트가 아니라서 종료 입력으로 처리됨을 원문 비트 마스크로 다시 확인해 기대값을 고쳤다. 로그 형식 차이(십진/16진)로 틀린 Start 돌 라벨 기대값을 교정했다. 시험 파일 작성 중 쉘 인용이 NUL 바이트를 심은 것을 발견해 이스케이프로 바꿨다(NUL 검사 통과).

**변형 확인:** BackgroundGifGump 연산자·상수 변형 49개와 의미 변형 18개(배율 곱 float 순서·종류 부호 확장·`>>>0`·기본 노드 일치 검사·반납 조건·기본 크기/표시 크기·부모 값 등) 모두 잡았다(처음 생존 3개는 플래그 `0x600` 정확 값·재사용 저장소의 bitmap 0 쓰기·적재 토큰 부호 없음 시험으로 잡음). NSInfo 세 모듈은 220개 중 214개 포착이며 6개는 동등 변형이다: 바이트 문자열 변환의 범위 끝 `undefined` 읽기(`<=`), LAN 복사 루프의 `<=`(NUL 이 같은 위치를 쓴다), 세미콜론 `end` 가 0 일 수 없는 비교 2개, 생성자가 쓰는 `0x800`↔`0x801`(기본 생성이 visible 비트를 이미 켠다) 2개. 처음 생존한 NUL 경계·기본값·가짜 토큰 1(할당 실패가 0 이 아닌 값으로 바뀌어도 통과하는 위치)은 시험을 보강해 잡았다.

## 남은 경계

- 실제 연결(파일 시스템의 `BackgroundGifSpec`·존재 조회, 소유 이미지 표, 공통 트리, `GumpPixelTarget.rawBitmap` 복사 장치)은 `RawBackgroundGifControls` 조립 단위가, NSInfo 의 메타맵 전체 배치(0047ea71 이후)·`RawNsInfoContext`·메타 명령 칸은 후속 단위(웹은 멀티플레이 제외)가 한다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
