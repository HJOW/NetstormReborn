# 실제 Renderer Frame/구름 표시 연결 — 단위620

2026-10-30에 시작하여2026-10-31에 완료한 단위다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 전체 정밀 날짜는 **2026-10-23**이다. 작업 전 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었다. 전체 목표는 active이며 이번 배경/Frame 연결을 전체 게임 동일성 완료로 세지 않는다.

## 실제 연결

618/619의4994b0 앞단·4977f0/497860 별도 구름4칸·499bfd..499cfe 배경 선택을 실제 PresentScene에 연결했다. 장기 ClientRendererFrame이 같은 Screen/카메라 snapshot·분모, RawButtonControls의 현재 지도/override, TileGroupCollection의 실제 Pool/공간 해시, ClientKencloudGlobals의 같은 cache/파일·설정/게임 난수를 빌린다. 초기 BSS19DWORD/구름4칸/count/별도 묶음 숨김을 미션/화면 교체로 다시 만들지 않는다. showSoftAsserts는 정밀 C의43354번째 줄에 있는 같은 설정 읽기 위치에 연결한다.

실제 signed 화면/지도/카메라 필드도 현재 참조로 재조회한다. 일반 Renderer dirty 표를 force 경계에서 실제로 비우며 구름4칸과 합치지 않는다. 같은 dirty의 지형 수집 뒤 kind4 커서 미리보기를 수집하고, 지형의 실제 덮임 판정으로 날씨/구름 배경을 선택한다. 월드와 현재 공통 Gump UI를 그린 뒤 화면 잠금을 반납하고 원문 구름 dirty 정리를 실행하며 마지막 출력/dirty 소비를 유지한다. 구름을 끄면 미표시 allDirty가 남는 원문 동작도 보존한다.

Native10.78에서 월드를 정적 GIF/SceneImage에 굳히던 경로를 제거했다. UI전용 dirty4는 월드/선택 표시를 다시 제출하지 않는다. 독립 표시/10.72의 기존 합성은 따로 유지한다. **원문66070 UI→487f10 건설 표시→433410 지형→일반 객체→kind4 preview의 완전한 통합 수집/깊이 정렬은 기존 분리 경계로 남는다.** 실제 extraCollection은487f10이 아니라 kind4 커서 preview이므로 지형 뒤에 연결한다.

Windows10/11은 원본 독점 DirectDraw 대신 기존 호환8비트 DIB를 사용한다. 비DIB 시차 전용 표면을 같은 DIB로 속이지 않으며 선택되지 않는 기존 명시 장치 경계를 보존했다. 이 차이는 코드 주석에 명시했다. 마지막 전체화면 확대/레터박스 출력은617에서 보정한 기존 GDI 경계를 유지한다.

## 독립 원문 좌표 관측과 집중 검사

`tools/decomp_renderer_projection.py`는 보호PE의499758 이후 실제 정수/세계 clip과 좌표 환산을 실행하여 첫433410에 전달한 float4개와 행 제한을 읽는다. UI/건설/화면 장치만 명시 경계이며 이번 oracle에서 지형 수집기 몸체/게임GUI를 실행했다고 세지 않는다. 세 화면비·전체/부분 지도·다섯 dirty·분수/음수 카메라의90입력이다. 한 완결 프로세스 안에서 PC027f 최초/재관측과 PC037f의 **270관측 차이0**을 확인했다. 제품 소스에서 기대값을 만들지 않았다. 새 `renderer-projection-x86.tsv`는 실제 제품 환산/행 제한에90행 모두 일치한다.

집중 **84개/실패0**, 실제 등록 **3530개**다. 새 Frame3개와 앞단720행/구름695행/기존 Kencloud944관측, Renderer·TileGroup·DrawSubmission·PreviewCollection·카메라·화면/입력의 관련 필터를 실행했다. 전체 CTest/보류된 자연 전투 검사를 실행했다고 세지 않는다. 최종 게임 빌드의 경고/오류와 최종 검사 증분 빌드의 경고/오류는 모두0이다.

## 최종 배포와 실제 창

최종 ZIP은 `extracted/cpp-renderer-frame-620-20261030/NetstormReborn-cpppj-620-final.zip`이다.1376항목/게임 자산1358과 EXE/런타임·CD 영상을 포함하고, 모든 압축/추출 바이트·게임 자산의 원본/CD 바이트를 전수 대조했다. 한글 설치 경로의 **gui-v3 실제10회/10성공**과 정상 종료0·실제 창DC RGB를 확인했고 설정 복구 후 모든 기존 배포 항목 바이트도 같았다.

- 메뉴 구름 켬/끔과 움직임, 미션 브리핑/ESC 막대 왕복, 실제 전체화면 시차의4회다. ESC 막대는 원문대로 자동 일시정지하지 않는다. 실제 Frame 준비/정상 정리 횟수, 같은 cache/phase·행 스캔과 표시 요청을 읽었다.
- 1024×768,1280×720,1280×800의 창/확대 전체화면6회다. 같은 논리 pan 입력의 두 축 이동량/요청 수, 해제, 이동 중 F2 소비/해제 뒤 F2, 최종 실제 창RGB가 일치했다.

추가로 별도 최종 배포 사본의 새 설치 인트로/ESC 종료와 재시작2회를 실행했다. 영상 픽셀은 독립 FFmpeg와 같고 실제 창DC RGB도 일치했다. 원문 상태/정지/팔레트 복원·완료 저장/메뉴 복귀를 확인했다. 첫 추가 실행은 기대 경로를originalCD로 두었으나 실제 배포 기본 경로가game/MOVIE여서 준비 오류로 실패했다. 제품은 정상 재생/ESC/종료0이었고 새 사본/동봉 경로의 완결2회와 구별해 보존했다. `movie-regression-v2/summary.json`을 따른다.

메뉴와 미션 캡처도 직접 확인했다. 사용자 물리 Alt/가운데 버튼·포커스 전환/DPI, Windows11 실제 장치/PCM 검사는 아니다.

| 산출물 | SHA-256 |
| --- | --- |
| 최종 Netstorm.exe | `21ce06666cfab1999455615a16201e84a0ecac5cb130bfbb00acca56edb56faf` |
| 최종 ZIP | `1d05b1eedd97039fb4c675f99ef5fff900f7889fd18bf8916d9050db75dac5b6` |
| 최종 netstorm_tests.exe | `42ea9365218724bd964c96c0e85d8f97accb8e3514d5728d2a068b4c9ef1382a` |

## 중간 실패/정정 보존

최초 MSBuild는 재생성 전 프로젝트를 읽은 상태에서 새 소스별 fixture define을 놓쳐 C2065 1건이 났다. 명시 CMake 재설정으로 고쳤다. 새 검사의 단일 std::fill 구문에서 int→8비트 경고가 템플릿 문맥으로10회 출력됐으며 uint8_t 값을 명시한 최종 검사 빌드는 경고0이다. 프로젝트 병렬만으로는 파일별 컴파일이 순차여서, 설치된 MSBuild 규칙을 확인하고 해당 소유 빌드만 종료/보존한 뒤 명령에 `-- /p:UseMultiToolTask=true /p:MultiProcMaxCount=3`를 추가했다. 전역/프로젝트 컴파일 옵션을 바꾸지 않았다.

중간 제품의 최초 GUI는 UserNotUpdated 안내를 닫지 않아 검사 실패했고, 다음 시차 검사는 창 모드가16비트를 지우는 원문 규칙을 놓쳐 준비 오류로 실패했다. 두 제품 프로세스는 모두 정상 종료0이었다. 실제 버튼과 전체화면187 입력으로 고쳤다. 이어 통과한 중간 GUI10회 뒤487f10 건설 표시와 실제 kind4 cursor preview의 혼동을 확인해 수집 순서를 정정했다. 그 제품의621은 Early6＋Training6의12완결 뒤13번째 bc7sun_bars76 중에 관련 Python2개/게임1개를 종료하고 원시 PID/자료를 보존했다. 이 부분 관측을 최종620이나 전체34 성공으로 합산하지 않는다. 최종 gui-v3와 별개로 중간 GUI12회(성공10/준비 실패2)도 보존되어 Frame 부분의 GUI 실제 실행은22회다. 추가 인트로 준비 오류1회/완결2회를 포함한 단위 전체는25회이며 최종 제품의 성공은Frame10＋인트로/재시작2회다.

## 보호와 다음 작업

보호 **2620파일/기존 fixture761 불변**, 현재 fixture **762**다. 원본/CD·AGENTS·양 판본 정밀 C/함수 표를 변경하지 않았다. 디컴파일 소스도 변경하지 않았다. [원문 집계](../../cpppj/renderer-frame-native-20261030.json)·[완결 집계](../../cpppj/renderer-frame-runtime-20261030.json), 원시 `extracted/cpp-renderer-frame-620-20261030/`를 따른다.

다음621은 `extracted/cpp-solo-renderer-current-621-20261031-v2/run.py`로 **같은 최종620 제품**의 일반31＋실제Demo3에서 짧은 진입·비정지 게임 시각·실제 창 출력을 재검증한다. 중간 제품의 기존621은 종료됐고 재사용하지 않는다. 이후 통합 Draw 수집/정렬·남은 UI/Edit·시나리오/파일·수명/Native·호환성과 MP무시험/4~7차를 계속한다. 포획/희생/특정 적 공격/시나리오 승리, 공중3종 전용 및 신규MP 검사는 하지 않았다. 자연 완주/전체 동일성은 미완료다.
