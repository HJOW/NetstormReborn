# dotnetpj HTML 메타·전체 생성기·링크와 모달 — 2026-10-29

일반 HTML 창의 메타 해석과 전체 생성기를 기존 실제 장식/버튼/본문/메뉴에 연결했다. 같은 파서 상태를 두 번 쓰며, 링크를 실제 부모8192BYTE에 즉시 게시하고 기존 공통 dialog 표의 중첩 깊이를 보존한다. 전체 메뉴의 실행 진입이나 전체 게임 플레이 가능을 완료한 단위는 아니다.

## 근거와 구현

- 시작 HEAD는 `9ad3ff97f51ac3900fbc1b957a3367e339982af6`이며 작업 트리는 깨끗했다. AGENTS.md·현행 dotnetpj 인계·공통 제약을 다시 읽었다. 현재/마지막 전체 정밀 디컴파일 호스트는 모두 HJOW-Athlon(완료2026-10-23)이다. 새 디컴파일을 하지 않았다.
- cpppj `HtmlWindowMetadata.h/.cpp`, `HtmlWindowFactory.h/.cpp`, `DialogExitCommand.h/.cpp`, `ScriptCommand.h/.cpp`의 고리 부분, `RawMenuHtml.inc`의 ParseHtml/HtmlLink/FindModalLowest/SetModal과 대응 Native/Client 검사가 근거다. cpppj 소스·원본 fixture는 변경하지 않았다.
- `ScriptStringRing`은 같은 문자열도 매번 새 칸을 소비하는80칸 고리를 구현한다. 첫NUL·덮어쓴 토큰 거부·양수토큰 수명을 보존한다. 이 파일은 ScriptCommand 전체가 아니다. `QueueScriptCommand`/`ApplyScriptConfig`와 실제 명령 표/ConfigInterface 소비자는 후속이다.
- `DialogExitCommand`는 캡처80/256BYTE 한도→조회→code0 진단→첫BYTE 숫자 관문→atol/즉시고리복사→두필드저장을 수행한다. 페이지 helper의 단일 Dispose는 재진입 전에 필드를 비운다. 실제 `HtmlWindowGump.Destruct`는 이 helper를 호출하지 않고 원문대로 OnExit 필드를 보존하는 기존 몸체를 쓴다.
- `HtmlWindowMetadata`는9종 고정 우선순위, current20버튼/메뉴열, 키의 원시 공백/따옴표/대소문자, 체크 활성 단락평가, 숫자/고리 인자, Timeout/OnExit/Input 뒤 현재 큐 사건을 유지한다. 메타 제거는 캡처 끝이 아닌 현재 다음 LF/$/NUL까지다. `%` 치환은 원문처럼 merged 전체 대신 replacement 길이만 버퍼 앞에 복사하며 뒤 물리BYTE를 보존한다.
- 맵 목록은 같은 부모 `MenuColumns`를 직접 ref로 빌려야 하므로 기존 자동속성을 실제 필드로 바꿨다. 임시 사본/복귀 후 덮어쓰기로 콜백 도중 alias를 잃지 않는다.
- `HtmlWindowFactory`는 실제 생성/현재 createdSerial→Config치환→첫파서→ASCII영숫자관문→현재메타/기본OkButton→LF를BR로바꿈→같은상태의두번째파서→마지막LF/본문→현재화면/루트부착→모달→현재serial 순서다. 빈 출력은 실제xxx본문을 생성/부착/Kill하고, suppress비영 또는 생성0은 첫파서 효과까지만 실행한다.
- `HtmlWindowTextParser`는 현재 부모의 같은8192BYTE를 각 링크 효과 직후 게시한다. 매 파서의 링크cursor만0으로 돌아가고 뒤쪽 바이트·shown/hidden 상태는 원문 계약에 따라 유지한다. 죽음표시와 실제 소유 저장소 반납을 구별해 링크를 읽으며 실제 반납 때 주소별칭을 지운다.
- `GumpModal`은 기존16칸 dialog 표/현재 입력을 사용한다. 이전 창의 visible/alive/nonvoid 자식을 재귀 전에 비교하고 자신은 자식 뒤 비교해 최소 signed깊이를 찾는다. 새 창은 최소자식보다3단계 앞에 놓고 중복 등록 때 깊이를 다시 바꾸지 않는다.
- 실제 창/고리/공통idle, 일반/체크 메뉴 작성, 본문/파서/메타/지연수명 어댑터를 작성했다. 전체 ConfigInterface·파일/맵/기술 목록 owner와 상위 UI 소유자는 구현 완료로 세지 않는다.

## 검증과 제한

| 읽기 전용 원본 fixture | 실행한 관측 행 | SHA-256 앞부분 |
| --- | ---: | --- |
| dialog-exit-command-x86.tsv | 204 | `b09428751d30` |
| html-window-metadata-x86.tsv | 124/128 | `22aebb9dca3c` |
| html-window-factory-x86.tsv | 106 | `24e0e986a8dd` |

사용자의 멀티플레이 무시험 결정에 따라 `$Zsend`가 있는 원본 입력4행은 실행 전에 제외했다. Send는 원문 분배의 선택 호출 경계만 작성했고, 전송/패킷/네트워크 검사나 새 원본 관측은 하지 않았다. 위 제외와 xUnit 테스트 건너뜀 개수는 별개다.

총434개 실행 관측에서 명령/인자/고리/큐·메타의 전체8192BYTE/20행/기한/현재전역·전체생성의 두출력/부모링크/현재표식/반환·부분중단/효과순서가 최초부터 일치했다. Native3개다. 이 테스트는 하위장치를 명시 입력으로 공급하며 실제 x86 실행을 다시 한 것은 아니다.

추가16개는 실제 Config 층/치환, 보호 캐시 글꼴, 장식/20돌버튼/동적본문/소유버퍼/Slider, 실제 controlled 메뉴/체크상태/스택반납, 현재 idle/OnExit/고리, 실제 모달 입력/중첩최소깊이/반납, 두번파서/즉시링크/배경/가림상태, 빈본문/창생략, 긴본문의 화면제한·스크롤/픽셀·guard와 ref/부분실패를 검증한다. 배경 RLE와 화면/뷰/색은 명시 검사 입력이므로 전체 보호자산/전체화면 비교로 합산하지 않는다.

최초 연결 검사 코드는 GumpInput.Route의 raw 세인자 서명을 고쳤다. 이어 두 실제 입력 검사에서 드러난 기본 버튼의 미연결 BaseEvent를 복원된 GumpHelp에 연결했다. 공통 테스트 장치의 같은 ButtonHooks를 공개해 상위가 그 실제 경계를 설치하도록 했다. 제품의 기본 사건 처리를 임의0이나 검사 전용 성공으로 바꾸지 않았다. 기존 두열 테스트도 실제 메뉴 생성자를 거쳐 생성/소멸 count가 맞도록 조립했다. 기존 원본 기대값은 바꾸지 않았다.

- 집중: 새19개(Native3 + 실제/계약16)와 기존 HTML10개가 통과했다. 긴본문1개는 같은 최종 코드로 따로 실행했고, 마지막 전체 회귀가 모두를 함께 실행한다.
- 전체 **2,002개(Assets363 + Core1030 + Client609) 통과**, xUnit 실패/건너뜀0이다. 위 전송 입력4행 제외는 그대로 유지했다. 같은 최종 코드의 전체 Release 빌드도 경고0/오류0·종료0이다. 모든 검사/빌드 핸들의 정상 종료를 확인했으며 이 작업의 살아 있는 게임/검사/빌드 작업은 없다.
- 포트269행(L1 145/port124), fixture목록126개. 새 L1은 DialogExit/Metadata/Factory6행이며 Metadata는 위124행 관찰 범위다. ScriptStringRing의부분2행과 RawMenuHtml의 파서/모달부분2행은 port다. 기존 HtmlWindowGump2행만 같은ref필드 재대조 기준을 갱신했다.
- `reflected-commit`은 `f95a8db28ceae6c036fa0d65c1f7626bcc0c31a3`을 유지한다. 일반TSV의 목록 행 수는 머리행도 포함한다. 새근거소스의 기준은 시작 HEAD이며 전체 반영 범위를 올리지 않았다.
- 원시 로그는 이 PC의 Git제외 `extracted/dotnet-html-factory-compile-initial-20261029.log`, `dotnet-html-factory-native-initial-20261029.log`, `dotnet-html-factory-focused-initial-20261029.log`, `dotnet-html-factory-focused-recheck-20261029.log`, `dotnet-html-factory-focused-final-20261029.log`, `dotnet-html-factory-long-body-initial-20261029.log`, `dotnet-html-factory-tests-20261029.log`, `dotnet-html-factory-build-20261029.log`다. 최초 실패 로그는 보존했다.

## 중단 인계와 다음 순서

사용자가2026-10-29에 “문서 작성 및 커밋 후 목표 일시 정지”를 요청했다. 진행 중 검증 결과와 이 단위를 문서/로컬커밋으로 남긴 뒤 목표를 일시 정지한다. 다음 구현 단위를 시작하지 않는다.

재개 후에는 실제 스크립트 명령/ConfigInterface·Tell/파일·맵/기술 목록, MessageGump/대화창, 상위 메뉴 등록/자산/음향/호스트를 잇는다. R3/D1의 실제 전체 메뉴 픽셀 비교, R4 이후 전체34미션/월드/HUD/Edit/설정/배포·후순위 멀티플레이 사전 구현은 남는다. 전체 목표를 이 단위의 검증 범위로 줄이지 않는다.

이번에는 게임/별도창·원본/PE실행·금지된플레이재현·멀티플레이검사를 하지 않았다. 기존코드/검사은퇴0·전체메뉴/게임캡처0이며 전체게임의 동일성/플레이완료를 주장하지 않는다. AGENTS.md·원본/CD·cpppj·공통인계·외부TODO.md를 이 단위에서 수정하지 않았다.
