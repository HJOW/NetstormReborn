# webpj client 자료 읽기 계열 — WB04-E272 (2026-11-10, 4-A)

3.1절 4-A 의 **client 자료 읽기 계열**(client M 영역 18단위)을 판정했다. 한 단위(LanguageOptions)는 실제로 옮겼고, 파일 읽기 여섯 단위(Palette·Gif·FontCache·Type·Config·QuickHelp)는 웹의 기존 파서가 같은 결과 값을 내는지를 cpppj 고정 관측으로 대조했으며, 브라우저에 없는 FILE·키·부분 읽기 경계는 웹 제외로 기록했고, 나머지 소비자가 뒤 단계에 있는 단위는 해당 단계로 이월했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 새로 옮긴 단위: LanguageOptions (`www/core/language-options.js`)

원본 `004de390`(OS 언어 → 번호)·`004de410`(번호 → 이름)·`004de420`(이름 → 번호)·`004dea40`+호출자 `435248..43529f`(옵션 해석)를 옮겼다.

- `detectOriginalLanguage(langid)`: LANGID 낮은 WORD 의 primary ID 7=독일어(3)·10=스페인어(4)·12=프랑스어(2)·17=일본어(5)·22=포르투갈어(6), 그 밖은 하위 언어 2(영국식)면 1, 아니면 0. 높은 16비트는 버린다.
- `originalLanguageName(number)`: `english, english, french, german, spanish, japanese, portuguese` + 6차 `korean`(7). 0 과 1 을 별칭으로 합치지 않고 표 밖 번호는 영어로 대체하지 않고 거부한다.
- `findOriginalLanguage(name)`: 정확한 C 문자열 비교(ASCII 대소문자도 구별, 첫 NUL 뒤는 보지 않음)이며 미등록·빈 이름은 0.
- `applyOriginalLanguageOption(detected, current, published, { read, load })`: `currentLanguage` 를 읽어 번호를 고른다(미등록 이름이면 감지한 언어가 1 일 때만 1). 같은 번호면 거짓. 다르면 **current 를 먼저 쓴 뒤** 정규 이름과 raw 이름으로 용어표를 두 번 읽고 마지막에 published 를 쓴다. 첫 읽기가 실패해도 이미 쓴 current 는 되돌리지 않고, 두 번째가 실패하면 published 는 그대로다. 이름이 128 바이트 이상이면 원문 지역 범위 초과로 거부한다. int32 참조는 `{ value }` 상자.
- 웹에서 언어는 `?lang=` 로 고르므로(브라우저 언어 → 번호) OS 감지는 쓰지 않지만 원문 대조를 위해 둔다.

### 검사 (`tests/language-options.test.mjs` 6건)

- 고정 관측 `language-options-x86.tsv` **4,655행 전부**(LANGID 4,096·이름 13·옵션 546)를 재생해 결과·두 번 읽을 때의 전역 값과 순서·최종 current/published 를 대조했다.
- 손 시험(cpppj `LanguageOptionsTests`): 첫 읽기 중 callback 이 current 를 바꿔도 두 번째 읽기가 그 값을 보는 순서, 두 번째 읽기 실패의 부분 효과(current 만 남음), 같은 번호일 때 읽기 장치 없이 거짓, 장치 누락·128 바이트 경계(127 통과·128 거부)·NUL 뒤 긴 글자 무시.
- 변형 확인 19개를 넣어 **WORD 마스크 1개만 생존**했다(고정 관측이 WORD 범위뿐이라). 높은 16비트를 가진 LANGID 시험을 더해 잡았다.

## 2. 값 수준 대조만 한 단위 (기존 웹 모듈)

원본의 FILE 열기/위치/읽기/닫기 효과열, 키 할당·반납, 짧은 읽기(부분 파일)와 후크 예외 시나리오는 브라우저에 없다. 같은 입력 바이트에서 나온 **결과 값**만 대조한다. `tests/client-file-read.test.mjs`(3건).

| cpppj 단위 | 웹 대응 | 대조한 것 |
| --- | --- | --- |
| `PaletteFileRead` | `GamePalette.parse` | `palette-file-read-x86.tsv` 중 온전한 파일 12행(0x308 `.COL`·0x400 RGBQUAD, 키/하위 파일 변형 포함): 같은 256색 RGB, 길이 0·12·775·777 8행은 진단(0x940) 대신 오류. 진단 복귀 뒤 계속은 웹에 없다 |
| `GifFileRead` | `decodeGif` | `gif-file-read-x86.tsv` 중 온전한 입력 32행: 서술자 폭·높이가 관측한 치수와 같고 색 번호가 같으며(24행), LZW 코드 크기 1(4행)과 크기 0(4행)은 거부. 원문은 치수를 먼저 쓰고 해독이 실패해도 남기지만 웹은 전체 해독을 거부한다 |
| `FontCacheFile`(reader) | `BitmapFont.parse` | `font-cache-file-x86.tsv` reader 시나리오: 서명·버전 1·글자 수 256 판정(8·9 는 읽지 않음, 7 은 웹이 오류로 거부). **원문은 서명 마지막(열여섯째) 바이트를 0 으로 덮고 비교**(시나리오 10 이 읽음)하는데 웹은 거부하고 있어 `bitmap-font.js` 서명 비교를 열다섯 바이트로 고쳤다 |

글꼴 글리프 블록은 cpppj 가 바이트만 복사하지만 웹 `BitmapFont` 는 VFX 셰이프로 해독해 검증하므로(글리프 하나·프레임 하나), 가짜 글리프 바이트로 만든 관측 행의 글리프 부분은 대조하지 않았다. 빈 글리프 블록(크기 0)의 머리·표 값만 대조했다.

## 3. 웹 제외·대응으로 판정한 단위 (PORT_MAP 갱신)

| cpppj 단위 | 하는 일 | 웹 처리 |
| --- | --- | --- |
| `TypeFileRead` | `.type` 전체 읽기(`49c160` 이름 4부품 `\`,`d`,이름,`.type` 고리, `41baa0` check0/extra0) | `GameFiles.read('d/<이름>.type')` 전체 바이트 → E267 `TypeParser`/`typeProperty`. 이름 고리(`RawFileName`)는 `file-name.test` 가 대조. FILE/키 수명은 제외 |
| `ConfigFileRead` | 실제 선택한 설정 파일만 Probe/Load(비패턴 입력은 root/검색 장치를 부르지 않음) | `GameData.build` 가 설정 파일 바이트를 먼저 읽어 `Config.loadBytes`(E271 설정 파일 XOR 판정) 에 넘긴다. Probe 순서 제외 |
| `QuickHelpFileRead` | 도움말 `d/help.<언어>` 필수 읽기(`41baa0` length0/check0/extra2) | `help-window` 가 `Quick Help Spec` 경로에서 `GameFiles.read` 로 전체 바이트를 읽고 언어 파일이 없으면 영어 파일로 대체(`help.test`·`korean-help.test`). 추가 2바이트(LF/NUL)와 성공 캐시는 QuickHelpWindow 몫. `quick-help-file-read-x86.tsv` 63행은 FILE 효과열이라 제외 |
| `NativeFontCacheFile` | 글꼴 캐시를 현재 FILE 로 읽고 쓰기, 보호 원본 경로 판정 | 웹은 배포된 `.chfnt` 를 읽기만 한다. 실행 중 글꼴 캐시 쓰기와 보호 경로 판정은 제외 |
| `NativeShapeFile` | `_shapes.shp` 직접 map/view | `ShapeDatabase` 가 `GameFiles` 바이트 위에서 view 를 만든다. map 수명은 제외 |
| `StartupFiles` | 여섯 `setup.cfg` 후보 경로 탐색·CD 탐색·chdir/cwd·기본/보조 TAFF 준비 | E271 `StartupArchiveFiles` 와 같이 `GameData.build` 가 `netstorm.tarc` 를 등록한다. 후보 탐색·CD·cwd 는 제외 |
| `ClientOptionsFile`·`ClientOptionsSaveIO.h` | `options.cfg` 의 XOR/BOM 인코딩과 디스크 저장 | 웹은 옵션을 브라우저 저장소(`settings.js`)에 보관하므로 디스크 인코딩 제외. 원문 인코딩(CP1252 XOR)은 E268·E269 의 `config-interface`·`original-text` 시험이 대조하고, 설치 경로가 유니코드일 때만 쓰는 UTF-8 BOM 변형은 클론 전용 호환 저장이라 제외. `ClientOptionsDisk` 는 이미 partial 이었고 메모만 덧붙임 |
| `GameAssets` | TAFF·`.type`·SHP 를 로딩 순서로 묶어 `Find`(파일 별칭 → typename)·`FrameIndex`(layer·stride + cluster) 제공 | 웹은 `TYPE_LOAD_ORDER`·`buildTypeRecords`·`frame + layer * frameCount`(world-sprites)로 같은 일을 한다. `Find`·`FrameIndex` 와 SHP 블록 수 검사는 cpppj 도구 쪽이라 소비자 단계(5-A)에서 대조 — partial |

## 4. 뒤 단계로 이월한 단위

- `DataManagerFileRead`(전체 읽기 최소 4바이트 할당·`BindDataManagerFileRead`) → **4-B** RawDataManager 5종(Image 등)의 소비 단위와 함께.
- `ClientTemplateSectionNames`(Client 수명 lazy 512 바이트 이름표 소유자) → **4-B** Template 적재 소비자와 함께(`o/` 의 이름표 자체는 E19 `raw-template-section-names.js` 완료).
- `ClientFortGlobals`(현재/예약 Fort 이름 256 바이트 두 개·파일 키·목록·모드 전역) → **4-D** client 전역 조립.
- `ClientColorMapFile`(색표 캐시 파일 읽기/쓰기) → **5-A** `RawColorMapCache`(adapt)와 함께. 브라우저에는 쓸 수 있는 설치 폴더가 없어 계산 생성으로 대체할 예정.

## 5. 대응표·남은 경계

- `PORT_MAP.tsv`: `LanguageOptions` port, 파일 읽기 여섯과 Native/Startup/Options 일곱 port(웹 제외 경계는 메모), `GameAssets` partial, `DataManagerFileRead`·`ClientTemplateSectionNames`·`ClientFortGlobals`·`ClientColorMapFile` todo 유지(이월 메모).
- 실제 미션 자료(Tell·`.fort`·타입 표)의 조회 **순서** 통합 시험은 4-A 마지막 항목이다.
