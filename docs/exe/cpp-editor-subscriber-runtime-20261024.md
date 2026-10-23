# 기존 Editor의 실제 Subscriber와 Client 난수 수명 연결

2026-10-24 단위384 완료 기록이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 단위383은 `a9af29da / 20261024-05 CP`로 커밋했으며 전체2506검사/실패0,337.50초와 원본1394파일 동일성을 확인했다. 이번 연결을 전체 Editor 완료로 판정하지 않는다.

## 공유 소유자와 Subscriber

먼저 동결 owner 제안 `4a137a87f69beb985574b5e0a366791fce69da7dec35963ece0d6e14502df4be`, 다음 Subscriber 제안 `99ee09dc8b8427b7a51cca6b1404128b8213cf1af596ad2cb80dddd328e3a323`을 순차 적용했다. 두 제안은 앞서 별도 사본에서 check/apply 네 단계가 통과했고, 실제 공유 문맥에서도 네 단계 모두 종료0이다. 최초 patch는 바꾸지 않는다.

`ClientRandomGlobals`는 실제 CRT DWORD 하나와 같은 주소를 빌린 기존 CrtRandom wrapper, Subscriber 최초시드 DWORD를 Client 수명으로 소유한다. Session의 Seed/지형과 파편·새 ID 생성은 이 상태를 함께 사용한다. 소유자 없는 기존 독립 Session의 CRT0 fallback은 유지한다. GameRandom과 CRT를 혼합하지 않는다.

`LoadExistingEditorSubscriber()`는 최초 ExistingEditor의 World 시작 전에 같은 Template owner의 실제 Subscriber 노드, ID와 handle에 원문 로더를 연결한다. 정상 ID에서는 등록 설정·시계·난수 장치를 미리 요구하지 않는다. ID-1에서는 같은 Client Config.ReadInt에 같은 ID 객체를 전달하며, 필요한 경우 같은 CRT로 새 ID를 만든다. 원문 오탈자 등록 키와 Config 변경 callback은 기존 script/config 몸체가 처리한다. 파일 외곽의 State 통지와 이름 설정·제목·World 순서는 후속 전체 설치 부모가 맡는다.

## 검증과 후속 경계

Client 소유자5/실제 Subscriber4/Editor tail4/Storage 실제파일9/팔레트 전환2/Screen clear2, 총 신규26검사를 공용 등록했다. 최종 Release 제품/검사 빌드는 종료0·관측 경고오류0이며, 전체 **2532검사/실패check0·종료0/312.81초(전체312.85초)**가 통과했다. 원문 Subscriber694/새ID1024 관측을 새 관측으로 더하지 않는다.

State/UI 제안 `12ba89ce348e64110fadd07534aec2f94a10b4256597c84c951bf3d0e7820da6`은 Client의 같은5×144BYTE·활동기한, 실제 HUD includeHidden1, 섬목록 count와 원문 State4 공통확정을 연결한다. next4를 동기 drain하지 않는다. 실제 파일 로드 `836f938b97b05e43b8222a5a7c3caccf3f5b1c8f8bf0e3496ea7c50778909a95`는 기존 RawTemplateLoad와 원시 reader를 그대로 사용해 manager 선공개·정상false 정리·예외 부분효과를 같은 Storage에 연결한다. 기존 InstallFile의 교체·throw 계약은 보존한다.

Screen 제안 `7b1441e8f3fe5764ddfdde63a371d3c10a28331eb5fdc2a43add99ae5aaaa06e`은 일반GDI에서 clear1의 지우기/viewport 표시/clip 복구를 연결한다. paletteLoaded는 Client 수명으로 유지한다. 최초166관측의 tool/report와 당시 Screen 바이트는 `docs/exe/evidence/screen-palette-clear-20261024`에 그대로 보존했고 fixture도 byte동일하게 공용 위치에 복사했다. `tools/verify_screen_palette_clear_first.py`는 최초 보고서의 경로/SHA를 고치지 않고 역사적 입력을 해당 보존자료에 대응해7개 SHA를 모두 검증한다. fillBlt/DirectDraw/flip, Kencloud와 실제 ColorSort 입력 수명은 아직 필수 후속 경계다.

**이전 의미 해석 교정:**0042f540은 선택정보Gump가 아니라 현재 Chatmanager00548714와 CoreDataActive00595348을 검사한 뒤 CoreData+4의 rank로 Chatmanager+244 prefix를 갱신한다. 공통 Client 시작00439840에서0042e050이 무조건 manager를 생성하므로 BSS0 상수로 생략할 수 없다. 현재 채널표4800BYTE 및 ChatWindow 입력접두와도 별개다. 해당 실제 생성자·prefix·formatter와 현재 CoreData 수명은 다음 단위에서 복원하며 필수 hook은 계속 명시한다. startupStateCheck도 Config.GetInt0에서만 자연 반환하며 비영일 때의 저장상태 reader/벽시각/Tell은 별도 필수다.

최종 HUD/State/loaded와 실제 메뉴/전체 저장 후 Editor 재로드는 아직 남는다. 이번 단위는 위 모듈과 실제 소유자 접합을 검증한다.

## 실제 제품 회귀와 보존 자료

검사한 제품 SHA는 `4f8bdc3a4aeb53e2e57b7707b77fb224bce227f857cf4f7f8cf7665ebec1a3f5`, 검사 실행물 SHA는 `8e2d706eb0e8d975d43169a15b8a404693a4908e845d1b80ed82ed233e51325b`다. 원시 빌드·집중검사·CTest와 실행 자료는 `extracted/cpp-editor-subscriber-20261024-*`에 보존한다.

최초 집중검사의 팔레트2검사 중1개는 API의 ColorSort 오인명칭을 Cloud로 교정하면서 검사 입력 문자열 `sort`와 중단 레이블 `sort-write`까지 바꾼 root의 편집 오류로 실패했다. 해당 검사 소스를 `...-palette-before-wire-label-fix.cpp`로 보존하고 두 입력 문자열만 최초 fixture와 일치하도록 복구했다. 최초 도구/676관측 fixture/report와 제품 몸체는 이 수정으로 바꾸지 않았다. 검사 한 파일만 다시 컴파일한 build2 뒤 팔레트2검사와 최종 전체2532검사가 통과했다.

같은 제품의 Early1은 제품 타이머1과 기존 진행값을 보존한 입력으로 A~G, 실제 다리·사제 이동·수확/귀환, SP0→200→400→600, 완료 안내와 다음 Tutorial2의 Guide4 진입이 통과했다. 실제 실행178.40초, 마지막 게임시각106.843초이며 원본 파일은 복구 후 동일하다. 시작 완료값은1이어서 완료값0→1 및 영구 진행 저장은 이번 관측 범위가 아니다. 기존 화면 캡처 도구는 현재 창 선택 뒤 `FrameArrived timed out: timed out waiting on channel` 및 `window capture timed out: timed out waiting on channel`로 실패했고, 재선택·세션 재초기화 뒤에도 복구되지 않았다. 따라서 이번 결과는 목적별 게임 입력/상태 보고서 검사이며 직접 창 캡처 검증 성공으로 기록하지 않는다.

빌드 전에 바꾼 `ClientMain.cpp` 줄바꿈은 검사 후 HEAD의 원래 CRLF로만 되돌려 불필요한 전체 행 차이를 제거했다. 각 행 본문이 동일함을 별도 EOL 감사에 기록했고 컴파일된 코드 토큰은 바꾸지 않았다. 그 밖의 신규 자료는 UTF-8이며 최초 보존 자료의 원시 바이트와 SHA는 유지한다.

Early6·훈련9·캠페인16의31항목과 실제 Demo 메뉴3항목, 총34개 진입이 같은 제품 SHA로 모두 통과했다. Demo는 자연 timeout 뒤 실제 Demo 플래그/로컬 AI를 함께 확인했다. 전체 완주·승패·원본 전체 동일성은 이 검사로 판정하지 않는다. 최종 originals1394파일의 추가·삭제·변경은0이며 양 실행물 SHA와 UTF8/AST/JSON·로컬 링크·공백·보호경로를 확인했다. 초기 선택51파일 중 신규 Chat 후속문서 링크를 가진 조사문서1개는385로 이월하고 이번50파일만 인수인계 후 `20261024-06 CP`로 커밋한다.

최초 staged 공백 검사는 팔레트 fixture144행의 빈 마지막 trace 칸을 후행 TAB으로 보고했다. 이 칸은 자료 형식이므로 제거하지 않고 해당 경로에만 `whitespace=-blank-at-eol`을 지정했다. Screen166 fixture는 최초 CRLF 바이트를 Git이 LF로 바꾸려는 경고를 확인해 해당 경로에 `-text`를 지정했다. 최종 staged fixture의 바이트 SHA까지 원본과 대조하며 제품/최초 기대 자료는 수정하지 않는다.
