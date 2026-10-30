# 직접 카메라 중심 요청의 지연 적용 — 단위632

현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon/2026-10-23이다.631 e60133d5의 깨끗한 작업본에서 진행했다. 전체 목표는 active다.

## 원문의 서로 다른 호출 경로

10.78의497f20은 float 좌표의 음수16/11 곱→FTOL 하위DWORD→NEG→현재 화면 절반 차감으로 Renderer+50/+54 목표를 저장하고005c85dc를 증가시킨다. 현재 Renderer64BYTE·별도0059a900 snapshot은 바꾸지 않는다. 미니맵4803f0/480440과 사건의 직접 호출은 이를 사용하고, 실제 프레임의 Present 뒤497960에서 현재 카메라를 바꾼다.

433aa0은 별도로0059a8b0 증가→497f20→497960을 수행한다. 명시SID433ba0/Home/사제보기와 초기 Reset4340d0은 이 즉시 적용 경로를 사용하므로 일괄 지연으로 바꾸지 않았다.10.72의458990→458800은 목표X→00518954 증가→목표Y 순서여서 PATCH 본체의 순서와 다르다. 이번 CD 판독은 정적 근거이며 CD 실행 복원 완료로 세지 않는다.

## 제품 연결과 독립 원문 관측

ClientCameraGlobals::RequestCenter를 실제497f20 목표-only로 구현하고 RawCampaignSession의 같은 장기 Camera/ClientLocalStateGlobals를 거쳐 GameWorld::CenterCamera에 연결했다. 종전 CameraHome 임시 카운터만 증가시키고 현재view/표면/음향까지 즉시 쓰던 직접 요청을 교정했다. 지금은 실제005c85dc만 증가시키며 현재view/snapshot과 표시/음향은 프레임 말미의 기존 ApplyCameraFrame/CameraApplied에서 공개한다.

원본 PE의 실제497f20/4e49c0만 실행했다.140개 독립 입력을 최종 두 별도 프로세스의 x87 PC027f/037f에서 각각 관측해560개가 같다. 멤버 설명 주석 보정 전 초기 두 프로세스도 같은 출력이지만 최종560개와 합산하지 않고 native-v1/v2로 보존하며 마지막은v3/v4다. 네 fixture 바이트가 모두 같다. 정상 ret8·목표X→목표Y→revision의 정확한 세 DWORD 쓰기와 Renderer160BYTE 중50..57 외 모든 바이트·별도snapshot64BYTE 보존을 확인했다. 외부 callback/게임/OS 실행은 없다. 분수·부호0·음수·NaN/무한·FTOL 경계, 세 화면비/홀수 화면 크기·카운터 감김을 포함한다.

새 C++ 원문140행은 현재view64/target20/snapshot64·분모를 대조한다. 실제 GameWorld/Session 검사도 요청 뒤 현재view/snapshot·미니맵 기하·표면/위치음향·changed가 그대로이고 공유revision만 감기는지 확인한다. 뒤 실제 CommitCameraTarget을 실행한 후 같은 현재 좌표가 두 setter에 반영되고 CameraApplied에서 표시/음향에 공개됨을 확인했다. OS 효과는 명시 장치 경계이며 임의 카메라 setter로 성공을 만들지 않는다.

## 최종 검증과 배포

집중60/실패0·등록3548, Release 게임/검사 빌드 경고/오류0이다. 기존 CameraHome/명시SID/사제보기·카메라 Present/Commit/Pan/가장자리/미니맵/초기HUD·화면 호환 회귀를 포함한다.

동일 최종 한글 배포본의 미니맵6창/21표시 직후 버퍼↔창DC RGB가 통과했다. 클릭·두 바깥 드래그·해제·ESC/이탈과 Toggle 삭제/독립 재생성·현재 초기HUD를 세 화면비/1280×720 전체화면에서 확인했다. Toggle의 TEST02 english 한 줄은 별도 실행 사본에서만 수정하며 원천/배포는 불변이다.

같은 최종 제품의 일반31(Early6/Training9/캠페인16)과 실제Demo 메뉴3이34/34·실패0·재시도0·종료0이다. 실제 브리핑/Guide 버튼과 Demo 자연timeout 뒤 비정지 시각·현재 Session/미션·Frame 준비/정리·단일 Native 종류별 수집/분배와 표시 직후 창DC 전체RGB를 확인했다. 전체 진입 도구는 기존 최종 snapshot을 표시 직후 읽도록 맞추고 카메라 적용 뒤 버퍼를 final-after-camera.bmp에 별도로 보존한다. 원문에 없는 읽기 전용 검사이며 게임 처리 순서는 바꾸지 않는다. 이는 짧은 플레이 진입 검증이고 사용자 자연완주/승패/전체 원본 동일성의 완료가 아니다.

최종 게임 SHA-256 `2288f42bb55d1b94de1aa22efa7f9a974e69b9f040523f7e2a32c92f4c98fe68`, ZIP `3dce7d8e25ffe6b4dd82f10b4fbff2508304bdac9c013e6eba0a02e9178886a8`다. 배포 파일은 `extracted/cpp-camera-center-632-20261031/NetstormReborn-cpppj-632.zip`이다.1376압축항목/1358자산의 두 실제 실행 사본·압축·빌드/보호 원천 바이트가 같다. 기존 보호2627개는 불변이며 새 fixture1개를 포함한 현재 보호2628/fixture769를 다음 기준으로 쓴다.

[완결 집계](../../cpppj/camera-center-runtime-20261031.json), 원시 `extracted/cpp-camera-center-632-20261031/`를 따른다. 모든 빌드/검사/게임 프로세스가 종료했다.

## 다음 작업과 범위

Home/명시SID/사제보기/Reset의 기존 ApplyCameraPixels는 목표/두setter 조각이다. 다음633은433aa0의 공유WorldRedraw/revision 및497960의 전체 커서/Scroll/dirty/RedrawRoots 효과를 실제Client의 같은 적용 본체로 연결해야 한다. 독립 장치/초기 설치 시점 계약을 구별하며 현재 즉시 적용이 전체 원문 동일성까지 복원됐다고 주장하지 않는다. 후속 원문168입력/독립2프로세스672관측·next633-review.json의 같은Client 연결 계획은 별도 준비이며 이번632의140/560·집중/GUI/전체34에 합산하지 않는다.

전체 UI/Edit/시나리오·내용물/격자/grid/debug·파일/수명/호환성·MP/Outpust/Challenge 메타 표시의 무시험 사전 구현·4~7차와 자연완주/전체동일성은 남는다. 기존 VFX 범위와612 스택미정값 한계도 유지한다. 포획/희생/특정 적 공격/승리·공중3종 전용·신규MP/Windows11/PCM/물리키/DPI/포커스 검사는 수행하지 않았다.
