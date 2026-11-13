# webpj ClientKencloudGlobals — 구름 전역·주소 공간·팔레트 이름 소유자 (WB04-E396, 2026-11-13, 5-A 스물네 번째 단위)

`ClientRendererFrame` 의 선행 부품인 client **ClientKencloudGlobals**(cpppj `client/ClientKencloudGlobals.cpp/.h`, 기준선 136줄, 상태 todo)를 `core/client-kencloud-globals.js` 로 옮겨 port 로 올렸다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`, 시험 파일도 변경 없음). 구름 계산 본체는 이미 옮긴 `raw-kencloud.js`(E118)·`raw-kencloud-render.js`(E119)·`raw-fort-palette-transition.js`(E227)를 그대로 부르고, 이 단위는 같은 Client 수명의 물리·주소 공간·파일/설정/화면 장치만 연결한다. 파일 부모가 쓰는 cpppj `GifFileDimensions`·`DecodeBackgroundGif` 는 웹에 없어 `assets/gif.js` 에 `gifFileDimensions`·`decodeBackgroundGif` 로 더했다.

## 1. 규칙

- **물리:** 연속 전역 2054 DWORD(`state()`) — 이전 행 0..511·end 512·lastRow 513·nextRow 514·cache 515·현재 행 516..1027·seed 1028..2027, 구름 이름은 같은 버퍼의 바이트 8112..8215(=DWORD 2028..2053, kenCloud 2049·cloudAlgo 2050·전체 2051·부분 2052·phase 2053 와 겹침). 옵션 8 DWORD(`value(option)` `{ value }` 상자, PE 최초값 72·2·10·32·99999·400·228·245), 고정 16행 순서표, 별도 팔레트 이름 128바이트. 생성자는 파일/난수/구름 생성을 하지 않는다.
- **세 전역 묶음:** `globals()`(raw-kencloud, getter/setter·`dirtyRows`/`seeds` 는 `subarray` 살아 있는 보기, lowColor·speed·fury·seedCount 는 읽기 전용), `renderGlobals()`(`{ state, get speed() }`), `paletteGlobals()`(팔레트/구름 이름·cache 상자·`count()`=speed·`renderer()`=현재 토큰·`dirty`=`worldRedraw()` 와 같은 상자). 필드 사본이 없어 본체가 쓴 값이 곧 소유자 상태다.
- **주소 공간:** `allocate(bytes)` — uint32 해석, 0 은 RangeError, 0x36000000 부터 `올림(크기, 0x1000) + 0x1000` 간격, `next + extent + 0x1000 > 0xffffffff` 면 RangeError(cpppj overflow_error), 새 블록은 현재 할당 입력 훅으로 채움. `memory(address)` — target 이하 가장 큰 생존 base 의 남은 물리(one-past 는 길이 0), 앞에 base 가 없으면 "누락", 블록 밖이면 "범위". `release(token)` — 0 무시, 정확한 생존 base 만.
- **파일 부모(웹 적응):** 264바이트 지역 scratch(height/width signed DWORD + 경로 256)를 현재 할당 입력으로 채우고 원문 `loadKencloudFile` 에 장치를 준다 — `pathSpec("CloudFileSpec")`(인자 비영이면 Error), 존재는 `files.has`(원문 Probe 의 위치 코드 대신 1/0), 해독은 `files.read`(없으면 원문 진단 251) → `gifFileDimensions`(치수 선행 쓰기, 서명/표식 미검사) → 크기 0/16MB 초과 RangeError → `allocate` → `decodeBackgroundGif` → 같은 블록 복사. 해독 실패 때 이미 한 할당은 반납하지 않는다(cpppj 와 같음).
- **설정:** `readOptions(config)` — cloudiness/cloudSpeed/cloudFury/cloudSpacer/cloudRecenter/cloudAlgo/kenCloud 순서로 현재 값을 signed 로 `config.readInt` 에 넘기고 `changed` 일 때만 DWORD 로 저장.
- **화면:** `draw(screen, area, …, alternate)` — Screen(E393)의 clip/setClip/lock/unlock/blitBuffer 를 그대로 빌린다(clip 은 DWORD 로, blit 좌표·사각형은 signed 로). alternate(원문 parallax DirectDraw 잠금 004a15d0)는 Error 이며 이미 바꾼 clip 은 되돌리지 않는다.
- **팔레트 전환:** `transition(name, …, devices)` — 호출자 장치(log/loadPalette/buildMaps)에 CRT `_stricmp`(A~Z 만 접음, 두 이름 모두 NUL 필요)·반납·생성·행 이동·`pathSpec("GamePalSpec", 이름)` 을 더한 **새 묶음**으로 원문 004b6100 을 실행한다(호출자 객체 불변).
- **Renderer:** `bindRenderer` — 같은 객체 그대로, null/undefined 는 0, 새 객체마다 0x3b000000 부터 0x1000 간격, 다음 토큰이 0xffffe000 을 넘으면 RangeError(객체는 이미 바뀐 상태). `rendererToken()`·`worldRedraw()`.
- **배경 GIF(`gif.js`):** `gifFileDimensions` — 전역 팔레트 바로 뒤 첫 descriptor 의 +5/+7 WORD, 헤더 13바이트·descriptor 9바이트 미만은 RangeError. `decodeBackgroundGif` — 첫 descriptor 가 0x2c 이고 10바이트 이상이어야 하며, 그 치수를 논리 화면으로 옮기고 left/top 을 0 으로 둔 사본을 `decodeGif` 로 푼다(원본 바이트 불변, 실패는 Error).

## 2. 검사 (`tests/client-kencloud-globals.test.mjs` 15건)

- **cpppj `ClientKencloudGlobalsTests` 8건 이식(첫 실행 전부 통과):** 최초값·인접 별칭·주소 수명, 실제 `originals/d/Gifcloud.gif`(원본 setup.cfg 의 `\D\gifcloud.gif` 그대로)와 seed 400 — 난수 0x89abcdef → **4201191824**, 간격 **110**(cpppj 가 원본 x86 관측 v2 행으로 확정한 값), 512 아닌 실제 `Defeat.gif`·없는 파일의 fallback(228 채움, Defeat 이미지 할당 뒤 반납), fury 0 의 부분 효과, 설정 옵션(signed DWORD), 팔레트 전환 `LPM`·같은 이름 자연 반환, 갱신/dirty, 느슨한 파일 우선·잘린 GIF 오류 전파.
- **웹으로 바꾼 부분:** cpppj 의 Client 즉시 옵션 callback(설정 쓰기 → 소유자 갱신)은 Client 본체 몫이라 `readOptions` 직접 호출로 대신했다. cpppj 의 PathSpec 이름 고리(10칸) 색인 단언은 웹 설정에 없는 host 구조라 제외했고, TAFF 아카이브는 덕타입 아카이브(`read`/`find`)로, 512×512 GIF 는 시험 안의 9비트 리터럴 GIF 생성기로 만든다.
- **웹 계약 7건:** 배경 GIF 치수/정규화와 13·9·10바이트 경계·팔레트 크기, 파일 부모 장치 순서(`CloudFileSpec`→has→read)·크기 0·4097×4096 거부·정확히 16MB 할당 뒤 해독 실패의 할당 보존·원문 진단 251·경로 255/256바이트, 주소 산술(크기별 간격·1바이트 블록 413,695개 뒤 소진)·조회/반납 오류·생성자 입력, 옵션/전역 별칭과 각 DWORD 위치·`readInt` 인자, 실제 Screen 출력(오프셋 (100,37) 전체 프레임 독립 계산 일치·부분 행 5 만·면적 0·parallax 거부 뒤 clip), `_stricmp` 경계(`@`/`` ` ``, `[`/`{`)·NUL 없는 이름, Renderer 토큰(806,911개 뒤 소진).

## 3. 변형 확인

scratchpad 도구(주석·문자열을 가린 연산자/정수 상수/단항 `!` 변형 자동 생성, 변형마다 시험 실행 뒤 원문 바이트 복원)로 확인했다.

- **`client-kencloud-globals.js` 183개:** 1차 생존 41건(28%) — 원본 관측 없이 쓴 시험의 공백이 대부분이었다: 고정 행 순서표 14칸(cpppj 시험은 [1]·[15]만 봄 → 16칸 전체 단언), `_stricmp` 첫 글자·새 이름 쪽 `Z`/`[` 경계, 0x80000000 이상 토큰의 반납, 곱이 2^31 이상인 GIF 크기(`>>> 0`), clip 복구의 left/top(시험 clip 이 0 이라 가려짐), blit y 좌표의 `| 0`(타일 y 가 모두 홀수라 가려짐 → 짝수 오프셋/행), 부분 행의 전체 폭. 보강 뒤 **165/183 잡음**, 남은 18건은 모두 동등/도달 불가이다: ① `RENDERER_LIMIT` +1(토큰이 0x1000 배수), ② `_stricmp` 의 `left − right` → `+`(본체가 0 여부만 봄), ③ 경로 버퍼 초기 길이(쓰기 전에 덮임), ④ 주소 소진 `>` → `>=`(합이 0x1000 배수라 0xffffffff 와 같을 수 없음), ⑤ `memory` 의 base 표지(−1/−2, `< 0`/`<= 0`/`< 1`: 토큰 ≥ 0x36000000), ⑥ 두 PathSpec 인자 방어 `||` → `&&` 3건(본체가 상수 0 을 넘김), ⑦ exists 반환 1 → 2(본체가 비영만 봄), ⑧ 해독 치수 불일치 방어 `||` → `&&` 2건(`decodeBackgroundGif` 가 descriptor 치수로 정규화), ⑨ clip 읽기의 `>>> 0` → `>> 0` 4건(본체가 그대로 `setClip` 의 `| 0` 에 넘김).
- **`gif.js` 추가분 47개:** 45개 잡음, 남은 2건은 `descriptor > 길이` → `>=` 로 동등(같을 때 뒤 조건 `0 < 9`/`0 < 10` 이 어차피 거부).

## 4. 남은 경계

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 메인 메뉴 구름 배경을 이 소유자의 실제 Kencloud 출력으로 바꾸는 연결(4절 M1 오라클의 행별 1픽셀 어긋남 37,054픽셀 해소)과 Renderer 생성/반납 위치의 `bindRenderer` 는 `ClientRendererFrame` 단위, 설정 변경 즉시 callback 은 Client 본체(4-D E 구획)에서 한다.
- 메뉴 배경 로더(`menu-assets.js`)는 아직 `decodeGif` 를 쓴다 — `GifImage` 는 partial 유지(메뉴 배경을 `decodeBackgroundGif` 로 바꾸는 것은 메뉴 배경 단위 몫).
- `ClientRendererFrame` 선행은 이제 `RawCampaignCursorDisplay`·`RawCampaignFactoryDisplay`·`RawMenuControls`·`GameWorld`·`ClientGameplayGlobals` 이다(`TileGroupCollection`·`GumpCollection`·`ClientKencloudGlobals` 는 끝남).
