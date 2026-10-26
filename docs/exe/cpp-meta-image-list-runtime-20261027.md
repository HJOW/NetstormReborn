# 실제21칸 후보 목록과 공통 UI 연결 — 단위477

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 정밀 C와 원본/CD 파일은 수정하지 않는다. 앞 단위476의 실제 MetaImage를 자식으로 사용하는 원문 목록을 복원하고 같은 공통 트리에 연결한다.

## 원본 저장소와 실제 배치

`MetaImageList`는 원본0047ccb0 생성,0047f360 추가,0047cf00 부착/배치,0047cd90 적중,0047ce60 선택,0047d070 선택 그리기,0047d170 ID/0047d1c0 Image 조회,0047d1f0 제거,0047cd80 소멸을 복원한다.0047d1f0은 제거 함수이며 앞 인수인계의 getter 묶음 표기를 정정한다.

기본 등록 뒤 목록 가상 표0050d0d4, 타입BYTE22, 마우스 플래그0x200을 쓴다. 유효 count와21개 자식 토큰을0, selection을−1로 초기화하고 열 수를 저장한다. 원본이 쓰지 않는 ID 배열과 셀 폭/높이는 이전 비트를 유지한다.

Add는 원문 ID 기록→실제 MetaImage 생성→자식 가상 Attach→목록 Push→기존 위치로 목록 가상 재부착 순서다.21개 상한에서 새 할당을 생략한다. 가로 배치는 열 수0이하이고, 격자 배치는 지정 열 수와 올림한 행 수를 사용한다. 셀 크기를float로 저장하고 각 자식 Push 뒤의 현재 셀/자식 상태를 읽는다. 루프 종료는 현재 count를 다시 읽지만 열 수와 좌표 증분은 앞서 계산한 값을 유지한다.

적중은 오른쪽/아래를 제외한 현재 Bounds를 사용한다. 표시 scale을 다시 곱하지 않는다. 가로 배치는 count×dx를 먼저32비트 정수로 계산한 뒤 기본 폭으로 나누고, 격자는 두 셀 크기로 각각 나눈다. 격자 index==count도 원문처럼 허용한다. Handle은 좌누름만 선택을 토글하고, 화면 밖의 index−1도 현재 selection과 비교한 인자로 부모에 Bubble한다.

선택 셀은 녹색 체크무늬로 채운다. 오른쪽/아래 좌표는 절삭된 왼쪽/위쪽에 셀 크기를 더해 다시 절삭한다. Remove는 자식 Kill만 예약하고 두 배열의 유효 접두부를 압축하며 마지막 중복 토큰/ID와 selection은 유지한다. 목록/MetaImage 소멸은 빌린 Image를 해제하지 않는다. 저장소 밖 ID/Remove 인덱스와 비정상 count는 호스트 진단 예외이며 해당 비정상 메모리 입력은 신규 Native 자료 범위 밖이다.

## Native 관측과 정정 경위

`tools/decomp_meta_image_list.py`는 실제 목록 몸체와 기본/MetaImage 생성·자식 MetaImage Attach·CRT ftol·기본 소멸자를 실행한다. 기본 Attach/Push, Bounds/Redraw/부모 Bubble/Kill, 픽셀 소비자와 자식 heap 주소는 명시 경계다. Native raster 관측을 주장하지 않으며 실제 C++ 공통 입력/픽셀/클립/지연 삭제를 별도 검사한다. 신규 확대 관측과 전용 C++ 본체는10.78 범위다.10.72 생성 대응은00471b60이다.

최초257입력/514행은 작은 Image였고, 이후224×112/112×224 이미지를 섞어 자식의 실제 축소까지 확대했다. 두 버전은 `extracted/cpp-meta-list-477/native-calibration1/`, `native-calibration2/`에 도구/fixture/보고서 원시 바이트째 보존한다.

첫 C++ 비교에서 Remove12행의 trace만 달랐다. 원본0047d226은 대상 자식을 ECX에 넣고0047d235에서00466480을 호출한다. 이는 실제 thiscall/ret0이며 관측 장치가 이를 스택 인자로 읽고 있었다. 제품과 검사 기대 로직은 유지하고 Native 장치의 대상 읽기를 ECX로 정정한 뒤 전체 자료를 다시 생성한다. 원문 PE/CODE·모든 입력·tail/기하/반환값은 보존하고 삭제 trace12행만 달라지는지 별도 비교한다.

## 최종 검증

- Release build1/2는 종료0·경고/오류0이다. 최초 원본 장치 정정 뒤 컴파일 소스와 검사 본문은 바이트동일이며 게임/검사 실행 파일도 동일하다. 바뀐 컴파일 트리 항목은 새 Native TSV의 삭제 trace12행뿐이다.
- 확대/정정한257입력/514행을 다시 CPU로 실행해 fixture 전체 바이트가 일치했다. 원문 PE/CODE·모든 입력·tail/기하/반환값/명령 수 보존과 삭제 trace12행만 변경됨을 별도 비교했다. 최초 두 자료와 잘못된 장치도 원시 보존했다.
- 신규2개를 포함한 집중98고유/99관측·실패0이다. ImageList2 + MetaImage3 + RawMenuControls14 + EditorImage2 + MenuCheck3 + EditorIsland4 + ExistingEditor58 + NewFort13이며 목록 Native 사례1개가 두 필터에서 실행됐다.
- 같은 제품의 전체 C++3,252개·실패0이다. 기존3,250등록 이름을 유지했다. CTest wrapper가 아닌 실제 검사 실행 파일 전체를 직접 실행했다.
- 원본/CD/AGENTS·기존 검사/fixture3,178파일은 바이트동일이다. 기존 메뉴 검사46,229BYTE 전체 접두부를 보존하고 새 include만 추가했다. 첫12CHECK 실패도 덮어쓰지 않았다.
- 실제 공통21개 자식의 클릭/ID/토글, 체크무늬 전체 픽셀과 padding, 삭제 후 압축/끝 슬롯/selection 보존, Image 비소유 수명을 확인했다. 최종 소스/검사2,825파일의 SHA는 마지막 manifest와 일치한다.
- 게임 SHA256은 `6066d89d264b9e4d1c0af1cb348dcbca1da2acc95840a1452bbf26ad474a6a16`, 검사 실행 파일은 `d11dd229d03632534eef790a3733125fe706e57da0f13cb10a621978737fbdb3`다.

실제 명령/종료값/로그 SHA·첫 불일치·정정 비교·소스 manifest는 [실행 보고서](../../cpppj/meta-image-list-runtime-20261027.json), 원문 함수 범위/관측 한계는 [Native 보고서](../../cpppj/meta-image-list-native-20261027.json)에 있다. 원시는 `extracted/cpp-meta-list-477/`에 보존한다. 게임/GUI/음향 장치는0회다.

다음은432×412 Add Island 창의 실제00480090 공장·0047f450 생성·0047f9b0 Refresh·0047fb90 입력/기존 EditorIslandDrag·00480130 Idle·소멸/단일 창 수명을 같은 공통 트리에 연결하는 일이다. 현재 지도crop0/후보crop1,21개 후보 record와 Territory20칸, 같은 Session/공유 난수/월드/작업본을 사용해야 한다. 전용 창이 완성되면 기존/신규 맵에서 생성→선택→목록 위 취소/배치 거부/성공 배치→닫기→저장/재진입을 검증한다. Test Battle/언어별 시나리오와 모든 미션의 자연 완주·화면/음향 동일성 목표는 계속 진행 중이다.
