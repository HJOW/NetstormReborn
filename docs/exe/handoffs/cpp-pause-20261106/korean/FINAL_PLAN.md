# 한국어 최종 통합 준비 인계

2026-11-06 / HJOW-Athlon. 이 문서는 **제품 미적용 상태의 준비 후보**를 설명한다. 작성자는 제품 변경·빌드·C++ 검사 실행·게임/GUI/IME/MP 실행을 하지 않았다. 기존 두 pending과 고정 44개 번역 준비본은 보존했다. 사용자 중지 지시에 따라 여기에서 범위를 마감하며, 한국어 단계 전체 완료로 기록하지 않는다. LEFT_JOBS 갱신과 로컬 보존 커밋은 root 담당이다.

## 재개에 사용할 파일

| 경로 | 용도 |
|---|---|
| `final-pending/` | 제품 상대 경로를 유지한 최종 85파일: 코드/스크립트/CMake/검사 41개 + 본문/표 44개 |
| `final.patch` | **코드 41파일만** 포함한 단일 패치. 자산 44개는 포함하지 않음 |
| `final-manifest.json` | 최신 기준/결과 SHA256, 전체 경로, 진단문구 변경, 검사 이름 24개 |
| `FINAL_SHA256SUMS.txt` | 최종 85파일 바이트 해시 |
| `final-static-verification.json` | 적용 가능성·보존·진단 소비자 검색·폰트 해시의 정적 결과 |
| `final-base/` | 최신 제품 985의 수정 전 원래 바이트 38파일(기존 xlat 포함) |
| `final-base-manifest.json` | 최신 기준 파일 해시와 기준 HEAD |
| `final-base-983/`, `final-initial-base-manifest.json` | 최초 작업 스냅샷 보존. 당시 HEAD는983이며 진행 중이던984의 미커밋 바이트를 포함할 수 있으므로 파일별 해시가 기준 |
| `finalize-final.py` | 후보만 읽고 최종 패치/manifest를 다시 생성하는 정적 인계 생성기 |
| `final-tweaks.py` | 이전 후보에 넣은 교정의 작성 기록. **최종 패치를 적용하는 도구가 아니므로 재개 시 실행하지 않음** |

기준 HEAD는 `1f00a25add9619291f7978a18b2c07f2de0ee242`(985)다. 원래 통합/UI 준비본은 각각 `pending/`, `../cpp-korean-ui-preparation/pending/`에 그대로 남아 있다. 이 두 옛 패치를 다시 겹쳐 적용하지 말고 최종 패치 한 개를 사용한다. 검토 중 임시 `final-before-rebase.patch`와 `final-rebase-check/`는 적용본이 아니다.

## 확인한 정적 결과

- 최신 제품을 대상으로 `git apply --check --ignore-space-change extracted/cpp-korean-integration-preparation/final.patch`가 종료코드0이었다. 제품에 실제 적용하지 않았다.
- 코드 패치 경로41개, 중복 경로0, fixture 경로0. 기존 두 pending의 저장 해시 불일치0, 고정44 해시 불일치0이다.
- 최종44개는 모두 UTF-8 BOM이다. 고정44와 다른 파일은 최종 `config.korean` 한 개이며 `crash HTML`, `crash TXT` 표시문장 두 값만 다르다. 키/순서/HTML태그/URL/치환매크로/escape/서식 인자 순서를 대조했다.
- 관련 WB 번역11개와 기존 공유 xlat 입력은 이전 manifest의 SHA256과 동일하다. AGENTS/originals/originalCD/원문 fixture 경로의 추적 파일 `git diff`는 비어 있다. 저장소의 별도 기존 WB 작업 파일을 되돌리거나 포함하지 않는다.
- 최종 배포 스크립트의 PowerShell AST 구문 분석 오류0이다. 스크립트 실행이나 ZIP 생성은 하지 않았다.
- 새 C++ 검사24개는 **작성만 완료**했다. 구문/링크/기대값의 실제 통과는 확인하지 않았다. 기존 전체 검사 또는 게임 성공으로 합산하지 않는다.

최종 코드는 985 변경을 별도 shadow에 접합했다. `ClientMain.cpp`의 실제 시계 수명 주석, `UberGump.cpp`의 shared World/전송 callback/반납 가드, `src/CMakeLists.txt`의 MP 부품 등록을 보존했다. 기준 SHA가 바뀌면 사본 전체 덮어쓰기를 하지 말고 새 제품에서 패치 적용 검사를 다시 한다.

## 리소스 선택과 텍스트 경계

1. `BaseFileSystem::ProbeExact`는 이 조회 동안만 `.korean`→`.english` 대체를 억제한다. 기존 Open/Read/Probe/암호/TAFF 부모는 바꾸지 않는다.
2. `FindBuiltinKoreanFile`은 고정 공식43개 `d/<이름>.korean`만 찾는다. 정확한 기존 VFS 한국어 파일이 우선이며 그다음 가까운 `exe/lang/korean/d/`, 개발 저장소 `assets/lang/korean/d/` 번들이다. 절대/UNC/드라이브/상위 이동/서브디렉터리/다른언어/미등록 이름을 번들로 바꾸지 않는다.
3. 사용자 `CustomMaps`의 명시 읽기와 Config의 기존 사용자 홈 override를 보존한다. 같은 이름의 공식 미션이 있어도 명시 사용자 reader를 우회하지 않는다. 미등록 한국어 파일 부재의 영어 fallback은 기존 VFS가 담당한다.
4. `ReadLocalizedGameFile`, `MatchLocalizedGameFiles`, Client의 대응 API를 Config/Tell/미션 표시에 연결한다. 파일 목록은 기존 열거 순서를 유지하며 같은 영어 fallback stem의 행만 한국어 논리 이름으로 바꾸고 추가 번들만 뒤에 붙인다.
5. Config는 기존 Native FILE/키/DecodeOriginalText를 사용한다. MissionScript의 `LocalizedDisplayScript`는 언어가 바뀐 표시 title/Section만 별도 임시 객체로 읽는다. 실제 실행 Config의 AI/SP/tech/수정값/구간 순서를 덮어쓰지 않는다.
6. Help는 **UTF-8 BOM일 때만** `DecodeOriginalText`→`EncodeDisplayText`로 변환한다. BOM 없는 영어1252/NUL/제어BYTE는 바꾸지 않는다. 현재 창이 기존처럼 마지막 LF를 한 번 붙인다.
7. `HtmlTextParserGlobals.extendedText`는 원래 필드 뒤에 추가하며 독립 원문 파서/fixture는 기본false다. 실제 Client는 생성/언어 해석 때 항상true로 설정한다. UI 번역 선택과 English에서 확정 입력한 한글의 문자 보존을 구별한 정책이다. 유효성은 기존 `o::DecodeExtendedSequence`만 사용하며 유효4BYTE만 통째로 전달한다.
8. HTML 공장의 선택 callback은 동일 `HasExtendedDisplayText`를 사용해 한글만 있는 본문을 비어 있는 것으로 삭제하지 않게 한다. 기존 ASCII/독립 fixture 관문은 유지한다. 이것이 938의 실제 본문 삭제 경계에 대한 후보 교정이다.

## 언어 전환과 UI/입력 교정

- queued `DispatchLanguageChanged` 뒤 Help 창을 닫고 성공 cache를 무효화한다. 다음 Help 생성은 현재 언어 FILE을 다시 읽는다. 현재 메뉴 클릭 중 창을 재진입 삭제하지 않는다.
- `TellFileMenus::InvalidateLanguageFiles`는 소유한 언어 Config의 내용만 같은 객체에 재적재한다. prefix 주소/원래 index/fileName/열린 `officalN.Overview` 링크를 유지한다. 일반 cfg/프로필/실행 미션은 재적재하지 않는다.
- 같은 알림에서 표시용 Script cache를 반납하고 **MainMenu일 때 `RebuildControls()`를 호출**한다. 기존 Main 부모/45초 deadline을 유지하며 바뀐 라벨만 SetLabel/재측정/Layout 한다. World/Chat 컨트롤은 이 이유로 재생성하지 않는다.
- `DisplayTextInput.h/.cpp`의 확정 입력 태그/UTF8·UTF16 변환/경계/필터/행 API를 두 준비안이 공유한다. InputQueue의 TryPushBatch는39개 실사용 사건 용량을 초과하면 일부도 넣지 않는다.
- TextEntry는 유효4BYTE 단위로 이동/선택/삭제/그림/완성을 처리한다. **이미 한글인 본문에 긴 ASCII를 붙일 때도 최종 물리BYTE 상한을 적용**한다. 선택 교체/여유0인 경우도 별도 검사 초안을 작성했다.
- 외부 선택 뒤 stale `shiftAnchor/shiftEnd`도 문자 경계로 보정한다. 한글 End는 length+1을 저장하지 않는다. 순수 기존 BYTE 분기는 원래대로 둔다.
- 실제 Unicode clipboard는 현재 창 owner와 CF_UNICODETEXT를 사용하며 기본 CF_TEXT 장치는 그대로 둔다. Unicode 형식 부재의 ASCII fallback 순서/할당 플래그를 독립 장치로 확인할 검사 초안을 추가했다. 실제 OS 실패/IME 메시지 순서는 미검증이다.
- Win32 IME 결과/WM_UNICHAR/중복 WM_CHAR 억제와 MCP text 연산은 기존 일반 큐→Gump를 사용한다. 물리 IME/후보창/클립보드 테스트는 하지 않았다. Save as Fort의 원래 ASCII/공백 파일명 정책을 한글 파일명 지원 완료로 해석하지 않는다.

## 번역 자산과 의도적인 원문 유지

고정 번역 준비본은 `../cpp-korean-preparation/`에 남아 있으며 43본문(공통11+미션32고유)과 xlat1개다. Tell302물리/194고유, Help138물리/128고유, xlat504블록/503고유의 구조를 보존했다. 상세 표시 잔여/명칭/미션 AI·SP 보존 근거는 그 폴더의 최신 `final-summary.json`, `coverage.json`, `verification.json`, `name-audit.json`을 참조한다. 이전 manifest의 최초 WB 치환 구간 수를 전체 최종 번역 수로 읽지 않는다.

- `serverHelp`의 원문 `DO NOT TRANSLATE YET` 자리38영어단어는 미복원 MP/외부 서버 경계로 의도 유지했다. 임의의 새 서버 안내를 작성하지 않았다.
- 매크로/조건부/AI·SP 설정/명령/리소스 키/경로/단축키와 UI 연결용 영문 괄호는 실행 구조 또는 조작 보조이며 번역 잔여로 합산하지 않는다.
- 부품명은 사제/신전/다리/발사기/간헐천/제단/거주 구역/고갈된 예외와 영어 음역 규칙을 따른다. 제네레이터·워크샾·훨리기그·훨리베이스·반더 타워를 사용한다. 세계관 일반명사/시/날씨를 부품명으로 일괄 치환하지 않는다.
- 고정44에 남았던 `config.korean`의 **휴면 외부 진단 문장 두 개만 최종 사본에서 추가 번역**했다. 원래 `bugs@titanic.com`과 troubleshooting 매크로/HTML/서식/옵션명은 보존했다. 현재 제품 src/tests의 정확 키/주소 검색 소비자는0이다. 이는 원본의 동적 키 구성이나 미복원 경로의 불가능 증명이 아니다. 외부 메일/브라우저 실행·전송 구현을 새로 연결하지 않는다. 이 값 번역과 폐쇄 외부 서비스의 복원을 구별한다.

## CMake와 배포 연결

최종 패치에 다음 등록안이 들어 있다. 제품 CMake에는 아직 반영하지 않았다.

```cmake
target_sources(netstorm PRIVATE client/DisplayTextInput.cpp client/DisplayTextInput.h)
target_link_libraries(netstorm PUBLIC imm32)
target_sources(netstorm_tests PRIVATE DisplayTextInputTests.cpp KoreanResourcesIntegrationTests.cpp)
target_sources(netstorm_korean_text_tests PRIVATE DisplayTextInputTests.cpp KoreanResourcesIntegrationTests.cpp)
```

추가 cache 검사는 기존 `QuickHelpWindowTests.cpp`에 있으며 이미 전체/`netstorm_file_tests`에 등록되어 있다. Native fixture 데이터/기존 기대값은 수정하지 않았고 같은 scene의 기본 영어 조회를 유지하는 선택 fileSpec callback만 추가했다.

`package-cpppj.ps1` 후보는 정확히43개 한국어본문을 `lang/korean/d/`, xlat을 `lang/korean/xlat.korean`에 넣는다. 기존 글꼴 동봉 경로는 최신 `fonts/D2Coding-Ver1.4.0-20261003-all.ttc`와 License.txt이며 파일 해시는 정적 결과에 기록했다. 게임 자산/원문 디렉터리에 번역본을 덮어쓰지 않는다. 배포 ZIP 생성/해제/실제 실행은 미검증이다.

## 후속 적용과 검사 순서 — 이번에는 미실행

1. 사용자 목표 재개 후 최신 제품 파일 SHA를 `final-manifest.json`의 base와 대조한다. 불일치하면 현재 변경과 hunk 병합을 먼저 한다.
2. 위 `git apply --check --ignore-space-change .../final.patch`를 다시 수행한 뒤 최종 패치 한 개를 적용한다. `final-pending/` 전체로 최신 제품을 덮어쓰지 않는다.
3. `final-pending/assets/lang/korean/`의 **내용44개**를 제품의 동일 상대 경로로 바이트 그대로 복사한다. `d` 아래43개와 xlat 한 개이며 BOM/AI/SP/줄끝을 다시 쓰지 않는다. 원본/고정 준비본/WB에는 쓰지 않는다.
4. 제품/전체 테스트를 컴파일한다. 아직 컴파일한 적 없는 새 API/검사이며 이 준비 단계에서 빌드 성공을 보증하지 않는다.
5. 새 검사 필터: `netstorm_korean_text_tests --filter KoreanInput_`(11개), `--filter KoreanIntegration_`(12개), `netstorm_file_tests --filter KoreanIntegration_ActualHelpCache`(1개). 전체 등록본 `netstorm_tests`도 사용할 수 있다. 기존 KoreanText/Config/BaseFile/QuickHelp Native98+Read62/HTML parser·factory/Tell file menu/입력·clipboard 원문 fixture도 관련 회귀로 실행한다.
6. 128개 실제 Help 주제·최대 battleOptionsHelp 끝 안내·영어128주제의 flag false/true 출력 동일·기존 AI/SP 수정값 유지·같은 이름 CustomMaps 우선권을 확인한다. 현재 8086BYTE 추산만으로 8192BYTE 실제 파싱 성공이라고 기록하지 않는다.
7. 허용된 클론 Main 언어전환/Help 재열기/미션 진입·대기/배포 사본 실행을 필요 범위에서 확인한다. 사용자 전용·사제 포획/희생·특정 적 공격·승리 재현·멀티플레이 실행 검사는 건너뛴다. 이번 준비에는 물리 IME 실행도 없다.
8. 실제 적용/검증 결과를 LEFT_JOBS와 문서에 기록하고 작업 단위 커밋을 한다. 준비본 보존 커밋과 실제 한국어 단계 완료를 구별한다.

## 미검증 경계

전체 C++ 구문/링크·새 검사24개의 통과·실제 한글 픽셀/클리핑·재사용 Main/HTML 입력창·긴 Help 링크 및 마지막 글자·물리 IME/clipboard OS 실패·읽기 전용 설치 위치의 배포 실행은 확인하지 않았다. 준비 범위를 마감했으며 새 기능 범위는 추가하지 않는다. 6차 한국어 실제 제품 완료, 사용자 검증 또는 목표 완료를 주장하지 않는다.
