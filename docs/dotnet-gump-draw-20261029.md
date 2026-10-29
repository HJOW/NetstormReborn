# dotnetpj R3e — Gump 클립 복구와 버튼 픽셀 출력

2026-10-29, HJOW-Athlon. 기준 cpppj는 `39bc55e914fa`의 `src/client/GumpDraw.h/.cpp`·`GumpVisual.h/.cpp`다. 같은 수집기와 현재 클립을 사용해 파생 Draw를 호출하고 기존 8비트 출력 원시 함수에 연결했다.

## 1. 구현

`GumpDraw.Draw`는 현재 flags의 0x00400000이 켜졌을 때만 Bounds와 위치 캐시를 조회한다. Bounds 투영 콜백이 클립을 바꾸면 그 뒤의 현재 논리 클립을 보호 객체에 저장한다. 임시 클립은 원본의 각 좌표 제한 방식이며 빈 영역에서도 가상 Draw를 호출한다. 중첩 호출과 예외에서도 저장한 논리 클립 및 right/bottom-1 VFX 끝점을 복구한다. 최초 VFX 값 자체는 저장하지 않는 원본 동작을 유지했다.

`GumpVisual`은 파일 팔레트의 검정/흰색/외곽선 색과 세 text context를 사용한다. 버튼의 네 선 순서, 외곽선 생략 비트, 눌림/비활성 우선순위, signed DWORD 감김 및 0 방향 나눗셈으로 글자 위치를 계산한다. `GumpPixelTarget`은 현재 VFX 클립에서 선·이미지·확대·원시 비트맵·단색/체크 fill을 처리한다. 원시 비트맵은 보이는 flat 주소만 읽으며 음수 기준 주소가 최종 할당 안으로 들어오는 경우도 허용한다.

## 2. 검증

- `gumpvisual-x86.tsv`192행과 `gumpdraw-x86.tsv`1,428행(3판본×476)을 Link로 연결했다. 전체 사건·클립·노드160바이트·위치 캐시·투영 순서와 32바이트 text context를 독립 기대값과 대조했다. fixture 목록94개, 포트 맵158행이며 전체 reflected-commit은 유지한다.
- 집중17개가 통과했다. C++의 원본 재생/독립 계약 외에 원시 비트맵의 음수 기준 주소·보이는 할당 밖 주소·예외 전 부분 쓰기·64비트 넘침, 체크 fill과 비공유 임시 선 클립을 추가 확인했다. 실제 트리 수집→Draw→버튼 선/투명 이미지→보호 버퍼 출력도 확인했다.
- VFX right 끝점에서 -1을 빠뜨린 변이는 원본 세 판본 모두 실패했다. 변이는 제거했으며 fixture 기대값은 수정하지 않았다.
- 원복 후 전체 **1,591개 통과(Assets363·Core1030·Client198), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 실제 실행 캡처 비교는0개다. 이 단위는 L1과 헤드리스 픽셀 통합이며 실제 메뉴/전체 플레이 완료가 아니다.
- 원시 로그는 Git 제외 `extracted/dotnet-gump-draw-focused-20261029.log`·`dotnet-gump-draw-mutation-20261029.log`·`dotnet-gump-draw-tests-20261029.log`·`dotnet-gump-draw-build-20261029.log`에 있다.

## 3. 남은 연결

공통 DrawSubmission의 kind2 분배/CRT 정렬은 해당 모듈을 옮기는 R5에서 연결한다. 이번 픽셀 통합 검사는 수집 순서의 직접 Draw이며 정렬/분배를 가짜로 대체하지 않았다. 다음 R3f는 `GumpBackground*`와 배경 위젯이다. 이후 글꼴/GIF·버튼/메뉴·실제 호스트 프레임/입력 큐를 조립한다. 기존 근사 코드/검사 은퇴는0개다.

cpppj·공통 인계·원본/CD·AGENTS.md는 변경하지 않았다. 게임/창·금지된 실제 재현·멀티플레이 검사는 수행하지 않았으며 외부 TODO.md 변경도 커밋 대상에서 제외한다.
