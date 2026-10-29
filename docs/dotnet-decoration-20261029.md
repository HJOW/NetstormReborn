# dotnetpj R3h-4c — 장식 배경과 원색 모서리

2026-10-29, HJOW-Athlon. 시작 HEAD는 `d5d05597`이며 AGENTS.md·현행 인계/공통 제약을 다시 읽었다. 직전 목표 턴은 메뉴 기본/제어부 두 단위의 구현·검증·커밋을 완료한 진행이다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. cpppj `GumpDecoration.h/.cpp`와 `menuwidgets.json`의 정적 계약을 사용했다. 도중 외부 TODO.md만을 담은 `53c597b8` 커밋이 들어왔고 해당 내용은 이 작업에서 수정하지 않았다. 포트 맵의 기준 HEAD는 그 커밋이다.

메뉴 생성자가 필요로 하는 선행 장식을 옮겼다. 실제 배경 생성 뒤 판본별 가상 표·signed 종류 BYTE·별도 모서리 타입/캐시 SHP/I 첫 프레임을 저장한다. Draw는 실제 배경 출력 뒤 위치를 한 번 저장하고 TL/TR/BR/BL 순서로 현재 크기/캐시를 다시 읽는다. 원색 모서리에는 색표/배율을 추가하지 않는다. 변경 영역은 자신/모든 부모 visible만 확인해 원점-1과 절삭한 크기+4를 사용하며 dead/void/erase를 추가 필터로 삼지 않는다. 기본 scalar와 기존 Draw/Redraw/수명 분배를 연결했다.

실제 SHP 블록/I 첫 프레임의 명시 대응과 물리 프레임 범위·특수 프레임·상대 RECT·공유 클립을 사용하는 픽셀 어댑터를 만들었다. 이는 기존 배경의 논리 클러스터 수/레이어 제한과 다른 캐시 물리 프레임 경로다. 전체 GameAssets 소유/초기화 연결은 아직 후속이다.

집중 **9개 통과**다. 같은 실제 기본 트리/등록/배경 RLE/모서리 RLE/현재8비트 표면/클립/수명에서 두 판본 생성과 네 고정 모서리 픽셀·guard, 앞선 출력 콜백 뒤 현재 필드와 저장 위치, visible/변경 영역·NaN/부분 생성·오류, 특수/물리 범위, Kill→Cleanup의 실제 등록/idle/overlay/텍스트·토큰 반납을 검사했다. 새 독립 PE 관찰/실제 메뉴 캡처/L1 대조는 아니다.

최초 컴파일의 Assets/Display 중복 이름4건은 불필요한 Assets using 제거로 해결했다. 최초 집중 검사 컴파일의 RedrawNode 인자1건은 기존 API에 맞춰 고쳤고 루트를 포함한 등록 개수는 기존 생성자 계약으로 수정했다. 원본 기대값은 바꾸지 않았다. 전체 **1,823개(Assets363+Core1030+Client430) 통과**, 실패/건너뜀0이며 Release 전체 빌드는 경고/오류0이다.

Git 제외 로그는 `extracted/dotnet-decoration-compile-20261029.log`, `dotnet-decoration-focused-initial-20261029.log`, `dotnet-decoration-focused-20261029.log`, `dotnet-decoration-tests-20261029.log`, `dotnet-decoration-build-20261029.log`다. 포트 맵은208행(L1 101/port107)이며 새 장식2행은 port다. fixture101개와 전체 reflected-commit은 유지했다. 기존 코드/검사 은퇴0, 게임/창/PE 실행0이다. 보호 경로·cpppj·공통 인계를 수정하지 않았다.

다음은 이 실제 장식과 기존 다중 줄 제목/항목 레이아웃을 MenuGumpWidgets의 생성·controlled 시작·부착·자식 우선/역순 항목 소멸에 연결하는 것이다. 이어 메뉴 입력/출력·실제 메뉴 조립과 R4 이후 전체 미션/월드/HUD/Edit 작업이 남는다. 전체 목표는 계속 진행 중이다.
