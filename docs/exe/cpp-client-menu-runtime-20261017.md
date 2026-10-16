# 실제 Client 공통 트리의 메뉴 입력·출력 연결

2026-10-17, `HJOW-Athlon`. 마지막 전체 정밀 디컴파일은 같은 PC의 2026-10-16 산출물이다. 디컴파일 소스와 보호 PE/아카이브는 수정하지 않았다. 원본 게임은 실행하지 않았다. 이번 단위는 실제 클론 창과 내장 입력/캡처를 실행했다.

## 연결 범위

`RawMenuControls`는 기존 `RawButtonControls`의 실제 등록표·루트·serial·GumpCreate/Collection/Tree/Lifetime/Events/Input을 직접 빌린다. 메뉴용 게임 상태나 별도 사각형 입력 트리를 만들지 않는다. 안정된 파생 노드 저장소와 가상 분배만 추가하고, 생성·항목 작성·측정·배치·확장·선택·하위 메뉴·명령·삭제는 기존 `MenuGump*` 몸체를 사용한다. 장식·제목·버퍼·Slider·화살표·손잡이도 기존 몸체와 같은 트리에 연결한다.

실제 UberGump의 **팝업**은 원본 MenuGump 생성/입력/8비트 픽셀 출력으로 교체했다. 기존 옵션/해상도/음량/도움말/Game/객체 팝업의 `DialogAction`은 호스트 전용 토큰 `01ffffff`와 인덱스로 연결한다. 이것을 원본 게임 명령 토큰의 복원으로 합산하지 않는다. 캠페인·브리핑 대화 페이지 안의 목록 행은 기존 어댑터가 남아 있다. 사제의 실제 `RawPriestMenu`와 일반 건설·커서는 후속 연결이며 **Construct는 아직 비활성**이다.

팝업 아래의 실제 메인 돌 버튼은 유지한다. 기존 바탕 글자는 원본 글리프 몸체로 먼저 같은 픽셀에 합성하고, 그 위에 메뉴를 수집·정렬·출력한다. 월드 전용 `Renderer::SceneImage`는 원래 UI 글자를 제외하므로 이 용도로 쓰지 않는다. 보고용 항목 경계와 자동 입력 지점은 실제 메뉴의 측정/배치에서 읽는다.

## 통합 과정에서 확인한 원본 필드 오류

한 줄 TextGump, 다중 줄 제목, Slider, 화살표, 손잡이 생성자가 종류 BYTE를 `alignFlags`(+28)에 쓰던 기존 복원 오류를 수정했다. 원본은 signed BYTE를 **`command`(+20)** 에 저장한다. 잘못된 +28 값은 장식의 재부착에서 제목을 늘려 메뉴의 클릭을 가로챘다. `alignment`라는 기존 상태 멤버 이름은 API 호환을 위해 유지하되 주석은 종류 BYTE로 고쳤다.

`analyze_gump_identity_fields.py`는 10.78과 CD판의 실제 MOVSX와 +20 MOV를 **143고유 입력·286명령**으로 실행했다. CD 화살표 생성은 Slider 생성자 네 분기에 인라인되어 각각 확인했다. +28 표식은 보존되고 +20만 signed DWORD로 바뀐다. 이것은 두 대입 명령의 증거이며 **생성자 전체 실행이 아니다**. 전체 생성/하위 효과까지 관찰했다고 합산하지 않는다. 새 원문/주소/바이트/SHA는 `gump-identity-fields.json`과 별도 fixture에 보존했다.

원본 메뉴의 그림자/비활성 전역 `005c78d0`은 기본 회색이 아니라 팔레트 로더의 **RGB(47,24,17)** 결과다. 실제 `PaletteColorTable.named`에서 이 항목을 공급한다. 기본 흰색 별칭은 같은 RGB 검색 결과를 사용한다. 패치·CD의 실제 메뉴/Slider/제목 사건 DWORD도 보호 PE에서 확인했다.

## 검증과 경계

공통 트리 입력 검사에서 명령/등록ID/전체 DWORD 인자, 비활성 행, 삭제 뒤 기존 버튼 입력 복귀를 확인한다. 장식 메뉴 검사에서는 실제 제목/숨긴 Slider 생성, 여섯 종류의 +20/+28 구분, 하위 메뉴 확장/선택, 실제 SHP/글리프 픽셀과 행 padding 보존, 공통 파생 삭제를 확인한다.

실제 클론 창은 CLI로 실행했고 Windows 도구가 `NetStorm Main Menu` 창을 확인했다. `computer-use`의 창 캡처는 재선택 후 재시도까지 두 번 모두 `window capture timed out: timed out waiting on channel`을 반환했다. 관찰되지 않은 좌표로 OS 입력은 하지 않았다. 해당 검사 프로세스만 경로/PID를 확인해 종료했다. 이후 게임 내장 입력 큐/ControlPoint/프레임 보고/BMP 캡처로 실제 Client 경로를 검사한다. BMP는 게임 Renderer 결과이며 OS 합성 화면을 캡처한 증거는 아니다.

내장 경로는 메인 → 옵션 → 해상도 →800×600 변경 → 도움말 →About에 도달했다. 팔레트/배경 위의 실제 출력도 BMP로 확인한다. 기존 옵션 파일은 검사 전 사본을 보관하고 검사 뒤 원래 값으로 돌린다. `originals/Netstorm.exe`와 `netstorm.tarc`는 같은 SHA를 유지한다.

재현용 입력은 `cpppj/tests/fixtures/client-menu-ui.tsv`에 보존한다. 실제 창을 여는 다음 명령은 AGENTS.md의 실행 확인 규칙을 적용한다. 검사 전 옵션을 보관하고 종료 뒤 복구해야 한다.

```powershell
cpppj/build/bin/Release/NetstormCpp.exe --run originals --window --mute-audio --frames 65 --ui-script cpppj/tests/fixtures/client-menu-ui.tsv --ui-report extracted/goal-raw-menu-20261017-ui-script-final.tsv --render-stats --set "tellTips=0;autoDemo=0"
```

mapped 그림의 전용 치환표는 아직 공급하지 않아 해당 경계는 예외로 거부한다. 일반 원색 그림·체크 표시·텍스트는 실제 자산을 쓴다. 객체 Form 부착/선택 소멸 callback과 원본 객체 메뉴·생산/커서/건설 명령 연결도 남아 있다. CD 종류 대입은 실제 PE로 확인했지만 실제 CD판 UI 자산/창 검증은 하지 않았다. 10.78 자산을 CD 순서로 읽는 검사 시도는 SHP 판본 불일치로 거부되어 성공 검사에서 제외했다.

최종 Release 빌드13은 **경고0·오류0·종료0**, 새 집중2개는 통과했다. 전체 CTest는 **내부1,458개 중1,457통과·1실패·240.09초·종료8**이다. 기존 `Cursor_DefaultResourceMappingAndSoftwareFrames_AreUsable`의 시스템 IDC_ARROW 소프트웨어 이미지 불투명 픽셀 검사가 단독에서도 실패했다. Cursor.cpp는 이번에 수정하지 않았으며 원인은 미확정이다. 이 결과를 전체 통과로 바꾸지 않는다. 새 메뉴/기존 나머지 원문·Session 검사는 통과했다.

최종65프레임 내장 UI 실행은 종료0이며6개 상태·800×600·About 도달, 팝업 전후 실제 메인 버튼8개 전체 픽셀 동일을 확인했다. 초기 바탕 글자 누락과 어댑터 버튼의 차이를 발견해 실제 버튼 노드 보존/원본 글리프 선행 합성으로 교정했다. 옵션과 추적 fullscreen marker를 원래 상태로 돌렸고 보호 PE/아카이브 SHA도 유지했다.

로그 접두사는 `extracted/goal-raw-menu-20261017-`다. `ctest-output.log`에 내부 전체 결과, `cursor-isolated.log`에 단독 실패, `ui-script-final.tsv`/`ui-validation.txt`에 실제 UI 근거를 보존했다. 종류143/필요 에너지2352의 SHA 감사도 통과했다. 모든 실행 핸들은 종료했다. 전체 사용자 미션 완주·원본 동일성과 DirectDraw 수정의 완료를 뜻하지 않는다.
