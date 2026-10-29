# dotnetpj R3h-4b — controlled 메뉴 작성과 실제 제목 연결

2026-10-29, HJOW-Athlon. 기준 cpppj는 `5f1ee0b7e8af`의 `MenuGumpControls.h/.cpp`와 기존 `cpppj/menucontrols.json`의 두 판본 정적 근거다. 현재/마지막 정밀 디컴파일 PC가 같음을 확인했다. 원본 게임·GUI·PE 에뮬레이터를 실행하거나 보고서를 다시 생성하지 않았다.

## 1. 구현

`MenuGumpControls`는 실제 MenuGump/Layout/ScrollingTextGumpMarkup과 같은 기본·메뉴·항목 저장소를 사용한다. 10.78의 include 전 현재 메뉴 검사, 전경→배경 설정 조회, 조회 뒤 현재 controlled 칸과 입력 BYTE 재조회, command0인 체크 항목의 쓰기 비트를 보존한다. CD의 null 현재 메뉴 생략·색 조회 생략·원문 전달과 command0 필드 보존도 구분한다.

일반/체크/자식은 각 Menu 색 키를 사용하며 disabled의 무인자 서식과 forceColor의 `%s` 데이터 경로를 구별한다. 무인자 `%%`만 축약한다. 인자가 필요한 다른 변환과 원본256바이트 지역 버퍼 밖 결과는 cpppj처럼 호스트 오류로 거부한다. 첫NUL/비ASCII BYTE는 그대로 보존하며 정수 서식은 문화권과 무관하다. 실제 Append가 스타일 소유/재배치·자식 숨김/등록 ID/트리 부착을 수행한다.

제목/머리는 판본별 `~3~E`·색·복귀 서식을 만들고 fancy는 실제 다중 줄 Parse만 수행한다. 일반 메뉴는 실제 disabled 항목으로 추가한다. fancy 뒤 별도 메뉴 Reflow를 추가하지 않는다. 구분선은 기존 Append의 선두·연속·40개 한도와 미작성 필드 보존을 사용한다. NoMenus는 표/visible/dead 대신 실제 생성 개수만 읽으며 그 개수의 생성·소멸 증감은 다음 Widgets 단위다.

CD 참고 helper는 실제 등록 ID를 따라 숨김/dead도 포함한 자식 기본 문맥8DWORD만 복사한다. 같은 입력 배열 참조·자식별 복사→재귀·현재 signed 개수를 유지하고 실제 순환만 거부한다. 자신과 저장 StyleText 문맥은 별도로 갱신하지 않는다. 선택 helper는 방문 항목 강조를 먼저 지우다가 첫 비영 등록/visible 자식에서 즉시 반환하므로 뒤 항목은 보존한다.

설정 읽기는 기존 ConfigStore의 실제 층 조회/치환과 ConfigNumbers의 CRT 숫자 판독으로 연결했다. 누락 키는 현재 값을 유지하며 제어부는 반환값을 사용하지 않는다. cpppj ConfigInterface 전체의 시작 순서·설정 파일 저장은 아직 이식하지 않았다. 이 바인딩을 그 전체 모듈의 완료로 세지 않는다.

## 2. 검증과 한계

- 집중 **17개 통과**다. 같은 실제 트리/등록/소유 StyleText/다중 줄 버퍼/Slider/글꼴/팔레트/픽셀을 조립해 항목·체크·disabled/forceColor·CD 원문, 색 조회 중 현재 메뉴/입력 변경, 실제 자식 부착, fancy/일반 제목과 머리, CD 문맥 참조/재귀/부분 선택·오류 전 쓰기·생성 개수 조회를 확인했다.
- 실제 대괄호 색 적용 직후와 최종 조각 문맥을 각각 관측한다. 원본 고정 접두 `~[C...]~[B...]~E`의 마지막 `~E`가 문맥5의 배경/그림자색을 검정으로 다시 쓰는 순서를 보존한다. 제목/머리의 `~E`는 해당 색 명령보다 앞이므로 결과가 다르다.
- 첫 집중 검사에서 새 검사4개가 문맥4를 배경 필드로 오인하고 마지막 `~E`의 다시 쓰기를 반영하지 않아 실패했다. cpppj와 기존 StyleText/Markup의 필드5·명령 순서를 재확인해 새 검사와 관측을 고쳤다. 제어부 구현이나 원본 기대값은 바꾸지 않았다. 이어 집중17개가 통과했고 검사 분석기 경고5개는 Assert.Single의 조건 인자 사용으로 정리했다.
- 전체 **1,814개(Assets363+Core1030+Client421) 통과**, 실패/건너뜀0이다. Release 전체 빌드 경고/오류0이며 최종 전체 검사에도 분석기 경고가 없다. Git 제외 로그는 `extracted/dotnet-menu-controls-compile-20261029.log`, `dotnet-menu-controls-focused-initial-20261029.log`, `dotnet-menu-controls-focused-20261029.log`, `dotnet-menu-controls-tests-20261029.log`, `dotnet-menu-controls-build-20261029.log`다.
- 새 독립 PE 관찰0, 실제 메뉴/게임 캡처0, 기존 코드·검사 은퇴0이다. 포트 맵2행은 **port**이며 L1로 세지 않는다. 전체206행은 L1 101/port105이고 fixture101개/reflected-commit은 유지했다. 이전 MenuGump/Layout의 원본568행 대조와 이 정적 계약 검사를 구분한다.

## 3. 다음

MenuGumpWidgets의 실제 메뉴 생성·장식/제목 자식·controlled 시작·수명과 MenuGumpInput/Draw를 이어서 옮긴다. 실제 메뉴 전체 자산/ConfigInterface/음향/GIF/호스트 조립과 R4 이후 월드/세션/HUD·전체 미션·Edit도 남는다. 전체 사용자 목표는 계속 진행 중이다.

AGENTS.md·cpppj·원본/CD·공통 인계를 수정하지 않았다. 금지된 실제 재현과 멀티플레이 검사를 하지 않았으며 외부 TODO.md는 작업과 커밋에서 제외한다.
