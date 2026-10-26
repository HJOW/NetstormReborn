# 미션 시작의 실제 HTML 배경 초기화와 ESC 복구 — 단위459

HJOW-Athlon에서 원본10.78을 기준으로 작업했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 이번에 정밀 산출물을 바꾸지 않았다. 전체 State7이나 State1 복원 완료를 뜻하지 않는다.

## 재현과 원인

458의 실제 Early1 메뉴 → MORE/OK 이후 문자 ESC가 가로 메뉴를 열지 못했다. 임시 관측 제품 Source1에서 같은 입력을 다시 실행한 결과는 다음과 같다.

```text
ESC459 entry phase=4
ESC459 tutorial=0
ESC459 route=244 activated=0 help=0
```

안내 판정기는0을 반환했지만 공통 Gump 입력이 실제 배경 노드244에서 소비했다. 그 노드는 flags`0x80601`·깊이`-100`의 BackgroundGifGump이며 `nsstart` 파일이 없더라도 입력 장치를 가진 실제 노드였다. BackgroundGifGump의 원문 입력 몸체는 ESC를 소비하며 저장한 명령`020000c4,0,1`을 예약한다. 따라서 ESC를 가상 키로 바꾸거나 UI 앞에서 강제 처리하는 수정은 원인에 맞지 않는다.

원본 State.cpp의 State7은 선택 정리 뒤, 큰 전투 정리와 Zone 설치 및 HUD 생성 전에 `00565e70`의 첫BYTE를0으로 쓴다. 기존 [LocalMissionState 원본 자료](../../cpppj/tests/fixtures/local-mission-state-x86.tsv)와 [분석 보고서](../../cpppj/local-mission-state.json)에 이 동작이 포함돼 있다. 실제 HUD 전체 생성자도 이 첫BYTE가0이면 배경 메시지 노드를 만들지 않는다. 이번에는 해당 Native 몸체·기대 자료를 변경하지 않았다.

현재 로그인 어댑터는 전체 State7을 실행하지 않고 Zone 설치 뒤 State8부터 수행한다. 게다가 그 어댑터의 `title`과 UberGump의 HTML 배경은 다른 저장소였다. 그 결과 실제565e70 초기화가 누락돼 HUD 교체 때 살아남은 배경이 ESC를 소비했다.

## 수정

`ClientHtmlWindowGlobals`에 원본 HTML 배경80BYTE와 P 억제 전역을 함께 소유하게 했다. UberGump의 Tell/Help/HTML/HUD는 이 장기 저장소를 직접 참조한다. 실제 Campaign 서비스와 로그인도 같은 owner를 빌린다.

로컬 미션 시작의 Zone 설치 전 위치에 원문의 첫BYTE0 쓰기를 연결했다. 독립 Session만 기존 지역 입력을 사용한다. 이 쓰기는 나머지79BYTE, P 억제, 창 인스턴스나 Tell 이름을 초기화하지 않는다. Native `RawLocalMissionState::Starting`의 참조도 같은 실제 첫BYTE를 가리킨다. 실제 State7의 나머지 큰 전투 정리·seed·음악·UI 부모는 여전히 후속이다.

임시 ESC 소비 진단 코드는 원시 증거만 남기고 최종 제품에서 제거했다. BackgroundGifGump·GumpInput·튜토리얼·ESC 자체의 사건 순서는 변경하지 않았다.

## 검증 범위

신규 두 검사는 실제 Client/Session 로컬 로그인에서 첫 패킷 이전의 같은 배경 첫BYTE0과79BYTE/P 억제 보존, 같은 HTML owner를 빌리는 네 정상 시작 입력의 현재 이름 재초기화를 확인한다. 첫 집중에서 새 반복 검사는 ClearWorld 없이 앞선 CaptureEnd가 남은 상태 블록을 재사용해 Zone의 원문 진단 관문에 걸렸다. 이 독립 검사에는 각 시작의 정상 상태 입력을 제공하도록 고쳤고, 배경/꼬리/P 억제 단언은 유지했다. 같은 Client 상태의 실제 정리·재진입은 GUI로 검증했다. 기존 LocalMissionState Native 관측과 HTML·배경·HUD·가로 메뉴 회귀도 함께 실행한다. 최종 빌드·집중·GUI 및 보호 자료 SHA는 [실행 보고서](../../cpppj/mission-background-runtime-20261026.json)에 기록한다.

GUI는 원본을 실행하지 않고 기존 게임 자료 복사본으로 클론을 실행한다. 실제 메인 메뉴·MORE/OK·ESC·관측된 원문 메뉴 라벨만 사용하며 임의 사건 명령이나 Tell, 월드 객체, SP를 주입하지 않는다. 메뉴 복귀와 재진입은 같은 Client 연결표의 반납과 새 토큰도 확인한다. 실패 결과와 관측 제품을 최종 제품의 성공으로 집계하지 않는다.

최종 Source4는2,056소스, build4 경고/오류0, 집중263고유/263관측·신규2·실패0이다. 같은 게임`7a8e5387…`의 Early1 ESC 가로 메뉴를 실제 화면에서 검토했다. 별도 실제 메뉴 복귀·재진입에서는 연결표가2/0/0 → 0/0/0 → 2/0/0으로 바뀌고 client9 토큰이1828716545 → 1828716547로 갱신됐다. 이 반납에는 기존 호스트 수명의 Forget 경계가 포함되므로 Native State1 전체 정리 성공으로 취급하지 않는다.

456 뒤 세 단위의 변경을 같은 최종 제품으로 통합해 전체 CTest를 실행했다. **C++3,130개·실패0**, 등록 이름 누락/기존 이름 삭제0, 보호1,821파일 변경0이며349.897초가 걸렸다. 기존 검사 본문과653자료(TSV650개)도 보존했다. 이 전체 통과를 미션 자연 완주나 모든 UI/픽셀/음향 동일성의 증거로 확대하지 않는다.

원시 자료는 `extracted/cpp-escape-routing-459-*`에 있다. 전체 미션 자연 완주, 픽셀·음향 동일성, 실제 Edit 설치는 이번 수정의 완료 범위가 아니다.
