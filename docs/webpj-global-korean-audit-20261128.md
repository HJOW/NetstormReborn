# webpj 게임 전역 한국어 재점검 — WB04-E1485

2026-11-28, HJOW-Athlon. 사용자 제보: 미션 중 ESC → 게임 → 미션 나가기의 확인창이 영어로 표시된다. 착수 기준은 WB04-E1484이며 cpppj 오라클 기준선 225553df는 유지한다.

## 원인과 수정

`webpj/translations/tell.korean`은 메인 메뉴 중심의 28개 섹션만 가진 부분 파일이었다. 미션 나가기 `ABORT` 등은 공통 영어 파일로 대체되었으며, 기존 검사도 메인 메뉴의 시작점만 따라가서 이 누락을 찾지 못했다.

공용 확정본 `assets/lang/korean/d/tell.korean`에서 아래 32개 섹션의 문구를 가져오고 웹의 좁은 본문 폭에 맞춰 공백 위치에서 줄을 나눴다. 공용 파일과 보호 원본은 수정하지 않았다. 웹 공개 복사본과 자산 매니페스트도 갱신했다.

| 구분 | 추가한 섹션 |
| --- | --- |
| 미션 수명 | ABORT, TryAgain |
| 생산·회수 | DenySalvage, BuildNoBucks, NoTechTheme, UpgradeNoBucks, SalvageVortex, SalvageInf, AlreadyUpgrading, ProdPending |
| 희생·지식·제단 | NewTech, AltarUpgrade, SacrificeForStormpower, WarnCantUpgradeAltarOffHomeIsland, EarlySacrifice, NewRank |
| 송금·동맹 | AskSend, AllianceRequested, AllianceSpam, AllianceComplete, AllianceBroken, DrawNotAccepted |
| HUD 경고 | PriestImmobile, islandUnderAttack, WarnYourPriestTaken, WarnNeutralPriestTaken, WarnPriestTaken, MultiplayerPriestDead, MultiplayerOtherPriestDead |
| 기타 공통 안내 | TellGifWritten, BridgesKilled, NotImplemented |

이로써 공통 한국어 파일은 60개 섹션이다. 제목·본문·단추와 Warning 문장은 번역하고 명령, 인자, 조건, 치환 키, 링크와 글자 서식은 유지한다. 본문의 짧은 줄에는 `<br>`를 명시했다. 구조 검사는 이 추가 줄바꿈만 허용하고 원문 `<br>` 누락과 다른 토큰 변경을 거부한다. Warning은 창을 만들지 않고 기존 HUD 명령으로 전달한다.

추가 점검에서 찾은 고정 영어 표시도 보완했다.

- 미니맵 우클릭 제목 `Sky Overview` → `하늘 전체 보기`. RawMinimapControls는 보존하고 ClientMinimap 조립부가 기존 번역 장치를 사용한다. 도움말 주제 `minimapGump`는 바꾸지 않는다.
- 채팅 도우미의 Mimic·Sound·Icon·Emboss Offset과 색상/배경색/아이콘 번호 범위. ClientChatWindow에서 표시 라벨만 번역하며 삽입할 서식 문자열과 명령은 유지한다.
- 미션의 사제 수 `%d Priest%s`. 한국어는 `%.0s`로 영어 복수 접미사를 숨겨 0·1·2·8명 모두 한국어로 표시한다.
- 옵션의 `Mods` 번역 키. 실제 언어별 모드 패널은 기존 구현을 유지한다. 언어 인자를 생략한 옵션 작성기 검사도 새 항목을 포함하도록 갱신했다.

## 전역 점검의 범위

공통 Tell은 배포 자바스크립트의 문자열 리터럴과 보간 문자열 첫 부분을 AST로 조사한다. 영어 공통 섹션과 일치하는 이름 중 표시 대상이 한국어 파일에 없으면 실패한다. 웹 제외 기능·내부 키와 이름이 같은 값·문장이 없는 구역은 `NON_DISPLAY_SECTIONS`에 이유를 명시했다. 기존의 작은 메뉴 시작점 목록에만 의존하지 않는다.

메인/ESC 메뉴, 객체·생산·플레이어 메뉴와 커서 문구, 43개 한국어 언어 파일의 미션 명령/구조·이름, 도움말 126개 앵커/링크와 글꼴 자산은 기존 한국어 검사 전체로 다시 확인한다. 번역표는 534개 항목이다. Type은 유닛 도움말 주제를 만드는 접미사여서 번역하지 않는다. `.front` 등 입력 명령과 관련 Back/Front 용어, 그림/소리 식별자, 언어 선택의 English/Japanese와 크레딧의 사람·회사 이름도 그대로 둔다. 멀티플레이어·Edit·DirectDraw·데스크톱 종료의 제외 범위는 유지한다.

영어 도움말의 나머지 두 앵커도 따로 확인했다. `map`은 문장이 없는 그림뿐이며, `serverHelp`는 원본에 `DO NOT TRANSLATE YET`라고 적힌 서버 초안이다. 둘 다 한국어 도움말에서 연결하는 링크가 없으며, 서버 기능은 웹 제외 범위다. 정상 UI의 번역 누락으로 세지 않는다.

한국어 문장 자체의 사람 검수와 모든 캠페인의 중간 안내 자연 도달은 별도다. 이번 작업에서는 사용자가 이월한 사제 포획/희생·특정 적 공격·승리를 자동 수행하지 않는다. 공통 희생 안내는 표시 입구와 구조로 검사한다.

## 검증

검사 결과와 실패 교정, 실제 화면 증빙이다.

- 초기 확대 검사에서 공개 복사본 갱신 전의 32개 누락을 검출했다. 갱신 후 구조/창 검사 14/14 통과.
- 관련 검사 117/117 통과. 소스 전체 참조 검사의 새 제외 경계 세 가지와 구분선 스타일/새 모드 항목 기대값을 보완했다. 줄 나눔 뒤 Tell/미션 사제 수 등 18/18을, 명시적 `<br>` 뒤 Tell11/11을 다시 확인했다.
- 운영 빌드 성공, 최종 번들 5,857,763바이트. 초기 빌드는 e2e 지역변수 이름 중복을 ESLint가 거부했으며 이름을 교정한 뒤 재빌드했다.
- 전역 ESLint 성공.
- 첫 브라우저 점검에서 일본어 선택 항목이 추가되기 전의 허용 목록, 4:3 고정 논리 해상도와 CSS 크기를 혼동한 검사, 단독 창의 미해결 치환 키를 보완했다. 실제 `EarlySacrifice`의 47픽셀 본문 폭 초과는 새 공통 번역의 짧은 줄 나눔으로 교정했다. 채팅 라벨의 중복 번역은 원본이 번역하지 않던 네 키만 처리하도록 좁혔다.
- 번들 동일성 검사에서 실행 디렉터리 차이로 내장 모드 실행기의 모듈 ID가 바뀌었다(5,857,708 대 5,857,736바이트; 부모 코드 동일, 내장 실행기 길이 20자 차이). webpack의 `context`를 webpj로 고정하고 기존 번들 시험이 저장소 루트에서 API를 호출하도록 확장했다. 정상 런타임 코드를 비교 예외로 제외하지 않는다.
- **최종 Chromium 6/6:** 66.787초, 실패/건너뜀/재시도 0. ESC 전 메뉴와 실제 나가기·계속·메인 메뉴 복귀, HUD 제목, 채팅 도우미를 포함한다. 줄바꿈을 추가한 뒤의 본문 비교는 공백만 정규화하며 문장은 그대로 검증한다. [실제 미션 창](webpj-e1485-evidence/mission-leave.png) · [채팅 도우미](webpj-e1485-evidence/chat-helper.png) · [전체 결과](webpj-e1485-evidence/browser-results.json).
- **공통 창 46행:** 23개×640×480/1024×768 CSS 표시에서 실제 갈무리 글꼴을 사용했다. 4:3 논리 화면은 둘 다 1024×768이다. 본문 초과 0, 가장 여유가 적은 줄도 1픽셀 안쪽이며 창의 최대 폭/높이는 340/306픽셀이다. [측정 행](webpj-e1485-evidence/common-dialogs.json). 실행 환경은 Windows/HJOW-Athlon, headless Chromium156.0.8078.4·Playwright1.64.1·제어 Node26.13.0이다([환경](webpj-e1485-evidence/browser-environment.json)).
- 첫 Node22 전체 시도에서 기존 `popup.test.mjs`가 E1483의 Mods 행을 기대값에 빠뜨린 실패를 찾았다. 입력 2,443개는 바이트 변경 0이었다. 이를 기록하고 시도를 중단한 뒤 기대값을 교정했으며 집중14/14가 통과했다([실패 원문](webpj-e1485-evidence/full-first-failure.log)). 최종 전체 회귀는 별도 실행으로 기록한다.
- **최종 Node22.23.3 전체 7,032/7,032:** 실패/취소/건너뜀/todo 0, 758.026초, 종료0. 저장소 루트에서의 번들 동일성 검사도 포함한다. 검사 전후 작성 소스/검사/번역/설정 입력2,443개는 모두 같은 바이트였다. [전체 로그](webpj-e1485-evidence/node22-full.log) · [입력 해시](webpj-e1485-evidence/input-hashes.json) · [출처/산출물 해시](webpj-e1485-evidence/provenance.json). E1485 종료 뒤 주기는 v26=3/4·v22=0/8이다. Node26 전체를 이번에 실행한 것은 아니다.

대표 화면: [미션 나가기 창](webpj-e1485-evidence/leave-dialog.png) · [640×480의 신전 회수 경고](webpj-e1485-evidence/salvage-640.png). [관련 117검사](webpj-e1485-evidence/focused.log) · [최종 줄바꿈 뒤 Tell 11검사](webpj-e1485-evidence/tell-structure.log) · [23창×두 표시 크기](webpj-e1485-evidence/common-dialog-layout.log) · [빌드](webpj-e1485-evidence/build.log) · [초기 본문 폭 실패](webpj-e1485-evidence/layout-before-br.log) · [초기 번들 검사 실패](webpj-e1485-evidence/bundle-check-before-context.log).
