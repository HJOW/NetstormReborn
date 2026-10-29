# 실제 Console의 로컬 명령 제출 — 단위581

실제 Chat Console의 Enter를579의 대상/본문 해석기와577의 점 명령 분배기에 연결했다. **빈 Enter, `.clear`, `.quit`**를 기존 입력 표시·공유 로그·종료 요청 장치에서 처리한다. 일반 본문의 서식·송신과 `format`/`back`/`front` callback은 아직 명시 미연결이다. 사용자 채팅 전송 전체가 완료된 것은 아니다.

현재 PC와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 일치한다. 디컴파일 산출물·기존 fixture를 수정하거나 원본 게임·네트워크를 실행하지 않았다. 기존0042da00/0043f6f0·clear/quit 관측 fixture와0042f1e0/0042f270의 원문을 참고했으며 새로운 독립 원본 관측을 생성하지 않았다.

## 실제 연결

`ClientDotCommandGlobals`가 기본 명령 슬롯과 Regexp 검색 상태를 Client 수명으로 소유한다. 제출 때 명령표나 상태를 복사하거나 초기화하지 않는다. `BindClientChatSubmit`은 현재 Identity·번역·Chat100행과 실제 수신자 할당을 사용한다. 같은 Console이 Enter 뒤 기존 접두 보존/정규화도 계속 수행한다.

빈 본문은 현재 Chat을 읽어 기존 `HudChatGeometry::SetInputVisible`을 호출한다. 일반 모드의 입력줄 숨김과 challenge의 강제 표시를 유지한다. `.clear`는 기존 원본 callback의 실제 공유 로그 지우기/스크롤을, `.quit`는 현재 종료 요청 사건 예약을 사용한다. 전투 중 `.quit`는 **QuitAbandon 확인창**을 거쳐 종료한다.

기본 say callback은 원문의 현재 manager0·빈 인자·앞 space 관문만 연결했다. 나머지 일반 본문과 미연결 callback은 명시 오류를 유지한다. 빈 성공으로 삼키거나 전송을 했다고 기록하지 않는다.

## 이름 변경의 증분 링크 수정

첫 실제 실행의 제품 SHA가580의 `511baeb9…`로 남아 있고 새 제출 연결 대신 이전 미연결 오류가 발생했다. 정적 라이브러리는 새 코드로 빌드됐지만 게임 EXE는 갱신되지 않았다. 생성된 프로젝트의 입력 `lib/Release/netstorm.lib`와 출력 `/IMPLIB:lib/Release/Netstorm.lib`가 Windows에서 같은 경로였고, 링커의 입력 추적에는 해당 정적 라이브러리가 빠져 있었다.

`NetstormCpp` 대상에 `ARCHIVE_OUTPUT_NAME NetstormCpp`를 추가해 import library만 `NetstormCpp.lib`로 분리했다. 실제 게임 이름은 계속 `Netstorm.exe`다. 네 구성의 생성된 import 경로와 수정 후 정적 라이브러리 입력 추적을 확인했다. 추가로 생성된 정적 라이브러리의 **내용 해시는 유지하고 수정 시각만 변경**한 뒤 빌드해, EXE 시각 증가와 실제 재링크를 확인했다. 첫 실패의 EXE/trace와 수정 전·후 링크 설정은 원시에 구별해 보존했다.

## 검증과 한계

집중 검사는 고유31개다. 새 실제 UI 조립4개를 포함한 ChatWindowClient16개와 ChatSubmit3개, DotCommandDispatch3개, ClientDotCommandGlobals3개, ClientChatRuntime6개를 확인했다. 첫31개 실행의 신규1개는 UI 문맥의 battle 표시와 실제 월드의 battle 모드를 혼동한 검사 구성으로 실패했다. 적 이름을 조회하는 실제 월드 모드를 설정하고 입력 문자열도 단언한 뒤 해당16개를 다시 실행해 통과했다. 총 실행47개, 최종 고유31개 실패0이며 제품의 Identity/원본 기대값을 수정하지 않았다. 등록 검사는3458개다.

실제 게임 검사에서는 `.clear` 입력/Enter 후 입력 비우기, 빈 Enter로 숨기기, 다시 열기, `.quit`의 확인창과 Quit 선택을 확인했다. 두 번째 실행은 원문처럼 확인창을 열었지만 검사 도구가 즉시 프로세스 종료를 잘못 기대해 실패했다. 확인창 선택을 추가한 세 번째 실행이 정상 종료했으며 최종 배포본도 별도로 압축 해제해 같은 입력·확인창/Quit 선택을 **17.294초 / 종료0**으로 확인했다. 각 실행의 제품 SHA와 최초 실패는 집계에서 구별한다.

기본 배포 스크립트로 수정된 빌드/ZIP1365항목을 다시 생성했다. 최종 ZIP의 클론 해시와 현재 제품, 원본 리소스 해시와 보호 원본을 각각 비교했고 사용자 설정 파일 제외도 확인했다. Release 빌드 로그의 경고·오류는0이다. 전체 C++/34경로·자연 완주·Windows11 실기 검증은 이번 범위가 아니다. 포획/희생/특정 적 공격/시나리오 승리, 공중3종 전용, 신규MP/통신 검사는 실행하지 않았다.

최종 게임 SHA-256은 `db33b4833b726cf5c1002bff1edfbd34a136ef580254a50d3b90f0ffec301c5f`다. 원시는 `extracted/cpp-chat-local-submit-581-20261030/`, 최종 수치·해시·실행/실패 구분은 [집계](../../cpppj/chat-local-submit-runtime-20261030.json)를 따른다. 원본/CD/정밀 자료/AGENTS.md1861파일은580의 작업 전 해시와 대조한다.

## 다음 작업

등록된 `format`/`back`/`front`의 원문 입력 해석·현재 manager 쓰기·Config 저장을 복원하고,0042dc30→0042d640 서식/0042d280 전달을 이어간다. 실제 일반 Enter 송신 완료로 확대하지 않는다. 기존 남은 UI/Edit·시나리오·수명·전체화면/Win11·전체 동일성 목표와 후순위MP 무시험 사전 구현, 4~6차 순서는 유지한다.
