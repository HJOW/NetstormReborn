# 섬 추가 창의 실제 배치와 드래그 미리보기 — 단위478

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 정밀 C, 원본/CD, AGENTS.md는 수정하지 않았다. 앞 단위477의 실제 Image·MetaImage·21칸 목록을 사용할 창의 배치0047d280와 미리보기 Broadcast0047d5f0를 복원했다. 전체 창 생성과 공통 가상 분배는 다음 조립 단위에 남아 있다.

## 원본 배치

`EditorIslandLayout::Attach`는 같은 현재 지도 Image를 MetaImage에 연결하고 실제0047cbd0으로 폭/높이+1을 측정한다. 배경은 그 기본 크기에32를 더한다. 기본 Attach를 실행한 후 제목→배경→지도→숨김 글자→목록→Done→TELL 순서로 가상 Push를 호출한다.

이후 현재 부모/자식 기본 크기로 숨김 글자와 TELL의 영역, 목록 영역을 계산하고 원본 순서와 정렬 인자로 일곱 자식을 부착한다. 가상 호출 중 변한 크기를 다시 읽으며, 기본 float 저장과 배율 크기 계산 전의 x87 중간 정밀도를 구분한다. 원본이 음수로 계산한 영역도 임의로 보정하지 않는다.

`EditorIslandLayoutFields`는 창의 현재 Image·자식 토큰·입력 상태를 빌린다. 후보21행, 실제 월드, Territory20행의 별도 복사본을 만들지 않는다. 같은 처리기를 후속 실제 창 생성/Refresh에 연결할 수 있도록 배치와 입력 상태의 소유권을 분리했다.

## 미리보기와 실제 Session

`Motion`은 드래그 Push→현재 지도 Bounds 순서로 진행한다. 이벤트 정수 좌표에서 이미지 반크기와 지도 원점을 함께 뺀 뒤 원본 CRT64비트 절삭의 하위 DWORD를 취하고 signed /16으로 격자를 구한다. 클릭 드롭0047fb90의 두 단계 절삭과 혼동하지 않는다. 정수 감김과0방향 나눗셈도 보존한다.

먼저 드래그 색상 모드를 비활성으로 바꾼다. 두 격자 좌표가0..13일 때만 같은6BYTE record의 위치 nibble을 갱신하고 실제 CanPlace를 호출한다. 성공하면 현재 드래그의 색상 모드를 정상으로 돌린다. 지도 밖에서는 record와 충돌 검사 호출을 보존/생략한다. 이후 현재 지도 Position을 다시 읽고 격자 좌표×16을 DWORD로 더해 현재 루트에 부착하며 깊이는−100으로 쓴다.

`RawCampaignSession::CanPlaceEditorIsland`는 실제 설치 완료/ExistingEditor/Fort/비Battle 관문과 독립14×14 지도 계약을 확인한다. 같은 Session의 현재 프레임·puzzlePiece·Template owner를 사용하는 기존 원본0046de50 몸체를 호출한다. 월드·record·공유 난수를 변경하지 않으며 실제 섬 배치/지형 생성/Graph/Territory 저장을 대신 실행하지 않는다.

## 관측 범위와 결과

`tools/decomp_editor_island_layout.py`는 원본0047d280/0047d5f0 전체 몸체와 Builder/owner/CRT ftol을 격리 CPU에서 실행한다. 기본/자식 가상 Attach·Push, 지도 Bounds/Position과 CanPlace 반환은 명시 장치 경계다. 실제 창 생성/Refresh/Idle/수명·Native raster·GUI를 실행한 증거로 간주하지 않는다. 신규 몸체와 관측은10.78 범위이며 정밀 디컴파일의 두 함수에는10.72 대응이 표시되지 않았다.

92입력×두 x87 정밀도=184행은 창/일곱 자식/드래그의 float 저장 비트, record6BYTE, 색상/깊이와 가상 효과 순서를 포함한다. 부모/자식 크기와 배율, 빈/가로·세로 지도, 가상 효과 중 현재 크기 변경, 격자 양끝/지도 밖/정수 극값/충돌 반환을 구별한다. 먼저 원본 자료를 생성하고 C++ 몸체를 구현했으며, 이후 원본 CPU 재실행의 fixture 전체 바이트도 일치했다. 관측 도구/입력/기대 자료의 사후 정정은 없었다.

실제 Session 검사는 기존 Editor 설치 전 거부, 월드 지도/다른 크기 거부, 같은 지도에서 찾은 합법/충돌 위치, 같은 실제 후보 Image와 미리보기 색상/위치/깊이, 지도 밖의 record 유지/검사 생략을 확인한다. 전체 과정에서 실제 Pool·섬 목록·공유 난수는 바이트동일이다. 실제 게임·GUI·음향 장치는 실행하지 않는다.

최종 명령/종료값/로그 SHA, 빌드·집중/전체 검사 수, 보호 바이트/기존 검사 접두부와 실행 파일 SHA는 [실행 보고서](../../cpppj/editor-island-layout-runtime-20261027.json)에 기록한다. 함수 범위와184행의 SHA/경계는 [Native 보고서](../../cpppj/editor-island-layout-native-20261027.json)에 있다. 원시는 `extracted/cpp-island-window-478/`에 보존한다.

- Release build1은 종료0·경고/오류0이다. 신규2개를 포함한 집중102고유/103관측·실패0, 같은 실행 파일의 전체 C++3,254개·실패0이다. CTest wrapper 대신 실제 검사 실행 파일 전체를 직접 실행했다.
- 기존3,252개 검사 이름을 유지했다. 원본/CD/AGENTS·기존 검사/fixture3,181파일은 바이트동일이며 기존 Editor 검사117,039BYTE 전체 접두부를 보존하고 새 include만 추가했다.
- 최종 소스/검사2,830파일과 실행 파일은 빌드 이후 동일하다. 게임/GUI/음향 장치는0회이며 빌드·검사·원본 CPU 작업은 모두 종료했다.
- 게임 SHA256은 `1d7b84a6d06ca9ce5373b747b419be3a9870196aa8ecd21e0b7faa1d9da3e3ea`, 검사 실행 파일은 `973f465af93d7f2c01d9b977dddcc1940ce02ead75403f0e8a672b17eac9ad8a`다.

## 다음 조립 경계

432×412 창00480090 공장/0047f450 생성,0047f9b0 Refresh, 기존0047fb90/EditorIslandDrag 입력,00480130 Idle,0047f940/00480110 소멸과 단일창 참조 수를 같은 RawMenuControls 트리에 연결한다. 이번 `Attach`와 `Motion`은 그 창의 가상+20/+2c에 연결해야 한다. **현재 AddIsland press는 여전히 실제 전용 창 factory 필수 경계에서 멈추며 사용자가 창을 조작할 수 있는 완료 상태가 아니다.**

생성 시 숨김/제목은 실제 ScrollingText, TELL은 원본004c8f60의 PlacingIslands→serial→00464020 조회 경로를 복원해야 한다. 현재 Uber의 TryTell은 별도004cf730 표시/phase 래퍼이므로 성공 bool만으로 이 serial/자식 창을 대신하지 않는다. Idle의00419d50도 원본 등록 조회와 가상+48(true)의 조건부 활성화이며 일반적인 월드 갱신 호출이 아니다.

공통 조립 후 기존/신규 맵의 선택→목록 취소/거부/성공 배치→닫기→저장/재진입 GUI를 검증한다. 실제 구동과 사용자 개입 검증은 사용자 지시대로 뒤로 미룬다. Test Battle/언어별 시나리오, 모든 미션 자연 완주와 화면/음향 동일성 목표는 계속 진행 중이다.
