# 현재 표시 상세와 Regular 실패 객체 문맥

WB04-E1390, 2026-11-27, HJOW-Athlon. CP1377(`aed29e3d1`)의 공통 오류 문맥을 웹 console에 반영했다.

표시 진단은 같은 Client의 `assertionDetail` 상자를 계속 빌리고, 호출 순간의 현재 문자열을 기존 위치/조건 진단보다 먼저 warning에 남긴다. 상세가 비어 있으면 기존 assertion 행만 출력한다. 실제 장기 저장소에 첫 상세·변경된 상세를 순서대로 쓰고 같은 assertion 경계에서 각각 읽히는지 검증했다.

Regular 실패는 실제 현재 풀에서 SID·TYPE·event·count·float32 payload8자리와 signed프레임(+36)·비정렬raw42(+42)의8자리 비트를 기록한다. 범위 밖 SID도 unsigned번호와 풀 크기로 기록한다. 이 추가 기록 뒤 기존 한국어 오류 행과 원래 예외를 유지한다. 풀 조회나 로그 장치가 실패해도 부가 기록은 원래 예외를 덮지 않는다. 정상 게임 처리·SP·난수·슬롯 바이트는 바꾸지 않는다.

웹7차는 파일 대신 console을 쓰므로 Native ErrorLog의 파일 경로·Windows 버전 조회는 추가하지 않았다. 실제 SID 풀은PATCH1078이며 새로운 진단은 기존 Session Regular catch와 장기 표시 assertion에 직접 연결했다.

- Client 진단·실제 Regular 조립·앱 Session 통합 **18/18 통과**(5.316초). signed프레임-4·raw42`89abcdef`·payload`3fa00000`·범위 밖 SID·-0·기록 실패와 풀 불변을 확인했다.
- 실제 Client 저장소의 현재 상세 교체와 원래 Regular 예외 객체 보존을 확인했다.
- webpack **3,498,464바이트**, 번들 재현 **1/1 통과**.
- 실제 Chromium 미션 서비스 연결 **1/1 통과**(24.4초).
- 대응표 검사 **7/7 통과**(23.737초). 기존1,160행/분류/상태는 유지한다.

초기 새 검사의4슬롯 풀과 `slot()` 사본을 수정한다는 가정은 실제 풀 계약에 맞춰 교정했다. 실제 최소 크기를 만족하는 기본 풀의 `physical()` 저장소를 입력으로 쓰고, 진단은 기존 읽기용 `slot()`을 사용한다. 제품의 풀 크기/읽기 계약을 완화하지 않았다.

ClientDiagnostics의 기준선 이후 변화25줄 전체를 반영해 이 행의 base만`aed29e3d1`로 올렸다. ClientMain은 epoch/최초Capture, RawCampaignSession은 Residence의 공유savedGame이 아직 남아 기존base를 유지한다. 오류 문맥 완료를 이 두 파일의 전체 변경 완료로 세지 않는다.
