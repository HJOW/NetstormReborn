# webpj 표시 키·일반 유틸리티·글꼴/음질·시나리오 표시 입력 이식 — WB04-E25

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물다섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/ViewKeyInput`·`client/UtilityCommandInput`·`client/PresentationCommandInput`·`client/ScenarioEffectInput`(10.78)을 옮겼다. 모두 Guide/Gump를 지난 일반 입력 하나를 받아 소비 여부를 돌려주고, 외부 효과는 훅으로 주입하며 도달하지 않은 장치는 요구하지 않는 같은 모양이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/view-key-input.js` | 4d9670~4d998c | `dispatchViewKeyInput(event, globals, hooks)`. F8(수정키 없음)은 Fort가 있으면 첫 명령 `(0x02000091,0,0)` 예약, 없으면 메타맵일 때 `ChalIntro` Tell, 그 뒤 편집기이면 mode 1 강조. F3/F8의 Shift(Control 없음)는 `theammode` 설정(초기 0, 조회 실패해도 유지)으로 `SetIslands(0/1)` Tell. **그 뒤에** 현재 정지를 읽어 참이면 끝(Tell이 정지를 바꿀 수 있음). F2는 `redraw=1`과 건물 숨김 반전, F7은 배너 값의 반대 쓰기, F3(수정키 없음)은 `(0x02000088,0,2)` 예약. Ctrl+Shift+F7은 별도 진단 범위여서 소비하지 않음 |
| `www/core/utility-command-input.js` | 542768/2860/2a5c 등 | 다섯 명령. TakeScreenShot은 저장 뒤 `TellGifWritten` Tell(저장 실패면 Tell 생략). URLAlias는 `text(y)`의 첫 NUL까지 + `" URL"`로 키를 만들어(256바이트 이상이면 원문 버퍼 밖 쓰기라 `RangeError`) 설정 조회가 성공했을 때만 설정값(첫 NUL까지, 256 이상이면 `RangeError`)을 링크 정책에 전달. Run은 `run(문자열, 1)`, MoveIsland는 `y`를 읽지 않고 `highlight(6)`, LookAtSid는 `y` 전체 DWORD를 `center`에 전달 |
| `www/core/presentation-command-input.js` | 004d7051/004d7295 | ChangeFontFace: `fontFace(y)` → `rebuildFonts(face)`(범위 밖은 `null`=Arial, 유효 빈 이름은 빈 이름) → **높이 먼저, 폭 나중** 조회 → `invalidate(0,0,폭,높이,0)` → 현재 이름 재조회 → `set('fontFaceName', 현재)`. ChangeSoundQuality: `closeSound` → `config('soundQuality='+signed y)` → `sound('priestmove.wav',0,0,0,0,0)`(Config 실패 시 되돌림·소리 합성 없음) |
| `www/core/scenario-effect-input.js` | 4d77f3/4d7f45 | 연기·번개·무동작. StartSmoke는 `smoke(y 전체 DWORD,0,0)`. Lightning은 **폭을 먼저** 읽고 기술 부모를 조회한 뒤(기술 조회가 폭을 바꿔도 먼저 읽은 폭 사용) `lightning(부모,[0,60],[폭,60],20.0)`. CreateRandomGuns·DumpSquids·모든 뗌은 장치를 요구하지 않고 효과 없이 소비 |

문자열 인자는 원본 바이트를 글자 하나당 바이트 하나(0..255)로 담은 JS 문자열이다. 각 모듈은 이름→명령 번호 조회(`…ScriptCode`, ASCII 대소문자 무시·미등록 `0xffffffff`)와 명령 판별(`is…Command`, 상위 입력 표시만 제외)도 함께 낸다.

## 2. 검증 (`tests/view-utility-inputs.test.mjs`, 14건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음):

- `view-key-input-x86.tsv` 310행: 수정키·뗌·정지·테마 조회·Fort/메타맵/편집기 분기와 F3/F8 큐 인자를 실제 40칸 FIFO 위에서 (장치 호출 순서·건물 숨김·다시 그리기·활성 큐 전부).
- `utility-commands-x86.tsv` 355행 중 비멀티 283행(MoveIsland 72행은 cpppj와 같이 제외), `utility-command-names-x86.tsv` 48행·`presentation-command-names-x86.tsv` 21행(이름→번호 표만).
- `sound-quality-command-x86.tsv` 32행, `font-face-rebuild-x86.tsv` 일반 입력 부모 15행(높이→폭 재조회·dirty·이름 재조회·Clear/Create/Cache/Dirty/Set 중단 꼬리).
- `scenario-effect-input-x86.tsv` 576행(연기·번개·무동작·폭 선행 조회), `dump-squids-input-x86.tsv` 48행.

cpppj 계약 검사 이식: 장치 지연 요구와 Tell 뒤 정지 조회, 다섯 이름표의 번호 중복 없음, URL 별칭 조회 실패/소유 문자열/Run 모드/길이 거부, 화면 저장 실패 시 Tell 생략과 SID 전체 DWORD, 음질 부분 실패, 글꼴 읽기 순서, 번개 폭 선행 조회와 선택 장치 누락.

최초 실패와 교정: 14건이 처음부터 통과했다.

## 3. 경계

- 대본 문자열 고리·큐 예약(`QueueScriptCommand`/`ScriptStringRing`): 이름 fixture의 인자(`n0`/`s…`)·문자열 고리 인덱스·진단 열은 대본 큐 단위에서 대조한다. 여기서는 이름→번호 대응만 확인했다.
- MoveIsland 72행(MP 항해 강조)은 cpppj와 같이 제외했다. 모듈은 `highlight(6)` 정적 분배만 옮겼다.
- 실제 장치(`FontStore`·`ClientAudio`·`CameraHome`·`SaveNumberedScreenshot`·Config 파서·링크 정책)는 훅으로만 주입했다. 해당 장치의 cpppj 계약 검사(`ActualFontStore…`·`AudioNative192Rows`·`ExistingCurrentSidCamera`·`ScreenshotExistingSaveFailureOrder`의 실제 저장 부분)는 장치 단위에서 대조한다.
- CD 10.72는 범위 밖이다. 이 입력들을 `GameMenuCommands` 소비 순서(Console/정지 관문 앞뒤)에 끼우는 일은 Client 입력 부모를 옮길 때 한다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체(분배·장부·훅 연결), `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등). 남은 작은 client 입력 분배 후보는 `CreateAtInput`·`DetonateInput`·`ProductionShortcuts`·`RitualChoiceInput`이다(각각 미옮긴 하위 장치 의존).
