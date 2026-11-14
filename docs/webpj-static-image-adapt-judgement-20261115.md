# webpj 점 명령 정적 이미지 연결과 파일·시작·설정 fixture 판정 (WB04-E480, 2026-11-15)

3.8절 이월 목록 중 `dot-command-static-image.hex`를 원본 10.78 PE 정적 표에 대조했고, 웹에 대응이 없는 파일·시작·설정 fixture를 PORT_MAP의 adapt 판정에 따라 "연결 대상 아님"으로 정리했다.

## 1. 점 명령 정적 이미지 (연결)

`dot-command-static-image.hex`는 보호 원본 PE 정적 데이터(00531b30~00532537)의 점 명령 인접 표 **2568바이트** 전체를 동결한 것이다. `ClientDotCommandGlobalsTests`(cpppj)의 세 시험을 `tests/command-native-fixtures.test.mjs`로 옮겨 대조했다. 모두 첫 실행에서 전체 일치했다.

| 시험 | 비교한 것 |
| --- | --- |
| 정적 표 전체 | `new ClientDotCommandGlobals().bytes()`의 2568바이트 전체가 PE 이미지와 같고 같은 물리를 빌려주며 count 가 1 |
| 공유 등록 | `format`·`back` 두 이름 등록이 최초 say 슬롯과 앞선 슬롯을 보존하고 같은 표에 쌓이며(count 3, 함수/표시 표) 새 소유자는 초기 상태 |
| 복사 중단 | 이름 복사 뒤 중단해도 같은 표의 부분 효과(이름 바이트)가 남고 callback/표시/count 는 도달 전 |

`PORT_MAP`의 "전용 PE fixture 없음(소스 계약)"과 `tests/dot-command.test.mjs` 머리의 "cpppj에도 직접 시험이 없다"는 오기였다(같은 감사에서 찾은 오기의 하나). 둘 다 정정했다.

### 변형 확인

모듈 `www/ui/dot-command-globals.js`의 자동 변형(상수 +1) 11개와 수동 의미 변형 9개(say 세 글자·함수 값·이름표 주소·count·DWORD 비트 이동·`bytes()`/`register()`의 복사본 반환) 모두 잡혔다. 생존 0.

## 2. 웹에 대응이 없어 연결하지 않는 fixture (PORT_MAP 확인)

아래 fixture 는 cpppj 의 C 런타임 FILE 상태 기계, 파일 열거, 설정 파일 쓰기를 관측한다. 웹 PORT_MAP 은 해당 단위를 모두 `adapt`(웹 제외/대체)로 판정했고, 웹 모듈에는 같은 형태의 경계가 없어 입력을 재생할 곳이 없다. 이 판정을 PORT_MAP 해당 행 메모에 기록했다.

| fixture | cpppj 소유자 | 웹 대응(PORT_MAP) |
| --- | --- | --- |
| `basefile-io` 323·`basefile-open-policy` 1,365·`basefile-read-policy` 1,025·`basefile-whole-read` 134 | RawBaseFileIO/OpenPolicy/ReadPolicy/WholeRead | `GameFiles.read`(전체 바이트)·`taff.js xorDecode`로 대체 — FILE 열기/읽기/키 전환 상태 기계는 웹 제외 |
| `basefile-read-key` 236·`taff-registration` | BaseFile | 같은 대체(느슨한 파일→TAFF 조회), 읽기 키/열거 등록은 웹 제외 |
| `startup-archives` 55 | StartupArchiveFiles | `GameData.build`가 `netstorm.tarc`를 `addArchive`로 등록 — 열거 순서/CRT 효과 웹 제외 |
| `xlat-file-read` 253 | XlatFileRead | `initializeRawXlat` 직접 조립 — CRT FILE 순서는 웹 제외 |
| `startup-file-names` 25·`options` 16 | ConfigInterface(Startup, SaveText+EncodeConfigFile) | Startup 은 `game-data.js` 직접 조립, options.cfg 쓰기는 `settings.js`(저장소 키 보관) — 10칸 이름 고리/파일 쓰기 웹 제외. 이름 고리 자체(RawFileName)는 `file-name-*` fixture 로 이미 대조 |
| `audio-reinterpret` 193·`font-face-enum`/`font-face-lookup` 389 | ClientAudio(Interpret)·FontStore | 웹 `audio-system.js`는 Web Audio 적응(partial)이고 글꼴 목록은 GDI 열거라 브라우저에 대응 없음. 적응 판정(3.6절 ①②)이 먼저이므로 사유와 함께 3.8절에 남김 |

이 판정은 "원본에 이 동작이 없다"는 뜻이 아니다. 같은 동작을 웹에서 어떻게 보존하는지는 해당 adapt 행의 메모와 `docs/formats/vfs.md`가 설명하며, 원본 관측 수치로 직접 검증하는 경로는 cpppj 쪽이다. 웹이 읽는 파일 내용(아카이브·설정 XOR 복호 등)은 기존 시험(`taff`·`config`)이 같은 원본 바이트로 확인한다.

## 3. 남은 것

3.8절에는 `html-input-dialog` 생성/공장 20행(SaveFortNameDialog 이식 때)·`detonate-input`(RawObjectConversion 단위)·`audio-reinterpret`/`font-face-*`(적응 판정)·텍스트 입력/채팅창/HUD/minimap/menugump/inventory-about·`renderer-projection`·`scenemusic`이 남아 있다.
