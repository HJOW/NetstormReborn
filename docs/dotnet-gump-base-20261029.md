# dotnetpj R3a — Gump 생성과 표시 수집 기반

2026-10-29, HJOW-Athlon. 기준 cpppj 커밋은 `2155b0f0855c`다. 이 단위는 `src/client/GumpCreate.h/.cpp`·`GumpCollection.h/.cpp`를 이식한다. 생성 노드와 표시 노드는 같은 객체를 공유하며 MonoGame·OS 장치에 의존하지 않는다.

## 1. 동작

`GumpCreate`는 0xa0 기본 생성, 미작성 +0x28/+0x74 보존, 판본별 가상 표 식별자, 원본 void/깊이/텍스트 문맥, 첫 빈 등록 칸, 미등록 플래그, DWORD count/serial을 유지한다. 표가 가득 찬 경우와 원본 개수 assert를 구별한다. 루트 생성은 표 비우기→new 경계→할당 뒤 현재 화면/카운터 읽기→등록/루트 기록 순서다. 할당 실패 때 표만 비우고 이전 노드·카운터·override는 남는다.

`GumpCollection`은 매 부모 뒤 float32 저장, 판본별 월드 부모 int→float 차이, signed64 CRT 절삭 하위 DWORD, 일반 위치 캐시와 월드 부착 자신의 캐시 보존을 유지한다. `Bounds`는 정수 위치와 float 크기를 더한 뒤 바로 절삭한다. `ScreenToLocal`은 `Position`의 단순 역함수가 아니며, 루트 위치 생략·월드 부모 뒤 일반 부모 생략·각 뺄셈 뒤 float32 저장을 따로 옮겼다.

수집은 원본 교차 경계, 화면 밖 자식 통과, 최상위 visible/dead 필터와 자식 visible만의 필터, 최상위마다 같은 limit을 유지한다. 따라서 전체 수가 limit을 넘을 수 있다. 최상위 next는 재귀 전에 저장하고 자식 next는 재귀 뒤 다시 읽는다. 원본 포인터 계약 밖의 순환·누락·실제 버퍼 초과는 C++와 같은 명시 오류로 남긴다.

## 2. 검증

- 기존 `gumpcreate-x86.tsv` **945행(3판본×315)**과 `gumpcollection-x86.tsv` **1,167행(3판본×389)**을 Link로 연결했다. fixture 목록86개·포트 맵142행이다. 전체 reflected-commit은 변경하지 않았다.
- 생성은 전체160바이트·현재 등록 표1000칸·전역·연속 연산 반환·할당 시점 관측을 대조한다. 수집은 전체32항목512바이트·양옆guard·모든 노드 캐시·투영 순서를 대조한다. 기대값은 모두 기존 원본 fixture 그대로다.
- 집중14개가 통과했다. 네이티브 재생6개와 C++의 독립 계약6개, 같은 생성/수집 저장소 연결과 로컬 좌표 계약2개다. 공통 Draw 제출/정렬/출력까지의 C++ 통합 검사는 해당 모듈 이식 후 잇는다.
- 변이: 기본 생성에서 미작성 Form을0으로 만들자 세 판본 모두 첫 입력의 전체 바이트 대조가 실패했다. 해당 변이를 제거한 뒤 전체 **1,527개 통과(Assets363·Core1030·Client134), 실패/건너뜀0**을 확인했다. Release 전체 빌드도 경고/오류0이다. 원시 로그는 Git 제외 `extracted/dotnet-gump-base-focused-20261029.log`·`dotnet-gump-base-mutation-20261029.log`·`dotnet-gump-base-tests-20261029.log`·`dotnet-gump-base-build-20261029.log`에 있다.

## 3. 이어갈 범위

다음은 같은 노드에 `GumpTree`의 부착/분리·정렬·깊이·이동을 옮기고, `GumpLifetime`→`GumpEvents`·`GumpInput`·`GumpFrame`→`GumpDraw`/배경/위젯을 연결한다. 글꼴/GIF 어댑터와 메인 메뉴는 그 위에서 조립한다. 이 기반 L1 완료를 실제 메뉴 화면 또는 플레이 완료로 세지 않는다. 기존 근사 경로와 검사 은퇴는 없다.

원본/CD·AGENTS.md·cpppj·공통 인계는 변경하지 않았다. 게임·창 실행과 실제 사제 포획/희생·특정 적 공격·승리 재현, 멀티플레이 검사는 수행하지 않았다.
