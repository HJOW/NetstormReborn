# 현재 수신 장치와 State7 인계 — 1174 부분 단위

2026-11-08 / HJOW-Athlon. M6-l1의 고정2·1a·1bh와 현재 Source/State7→8/Editor9 인계,7bh 고정 F를 접합했다. 원문89 Native 목록 중 폐기 CD 검증1개를 제외한 활성 대상은88개다. 현재11개/17type을 전환했으며 나머지77개와 각 실제 연결의 지연 큐·전체 장치 수명 통합은 미완료다. [기계 보고서](../../cpppj/packet-source-lifetime-20261108.json).

## 실제 제품 연결

1. 2번 종료 통지는 Client 장기 전역만 보유한다. NULL 연결은 비교용0이며 실제 저장/방송이 선택되는 위치에서만 현재 Session과 실제 전체 Runtime을 pin한다. 이전 Session을 고정한 수신 callable을 대기 값으로 보관하지 않는다.

2. 1a/004b4df0·1b/004b8b20은 장기 Client를 보유한 고정 F다. checking/서버·예약 조기 반환/기록/주소 부분쓰기/포트/State의 원래 순서를 공유한다.1a의 실제 복제와1b의 실제 Run이 선택되는 위치에서만 배송 connection의 같은 물리 Source를 선택한다. sender/phase/마지막 생성 World로 추정하거나 packet[1]로 다른 F를 다시 고르지 않는다.

3. ClientPacketSources는 실제 Meta/TCP/UDP/Composite/Editor를 weak 경로로 공개한다.1a의 next7은 weak Source와 실제 Zone 할당 token을 함께 기억한다.1b NULL 직접 호출은 현재 존재하는 실제 장치만 선택하며 없으면 기존 State8의 새 생성 흐름을 허용한다. 임의 Meta/연결/ACK를 만들지 않는다.

4. 일반 State7은 실제 카메라 뒤 CaptureLocalMissionStarting의 zonePending을 사용한다. 이전 World를 통째로 붙잡지 않고 같은 실제 전송을 새 현재 World로 인계한다. 기존 Editor9/pending/표/큐/depth도 MissionNetwork에 포함해 같은 장치를 계속 폴링한다. 일반 Assign/Flush가 실제 선택될 때만 기존 buffered 알고리즘을 적용하며 원래 Editor unbuffered 경로는 유지한다.

5. 수신 F와 복제 observer 표는 구별한다. Client가 현재 실제 표를 공개하고1a의 복제 위치에서 그때의 observer를 조회한다. RawPacketCopy의 두 입구는 같은 본문을 사용하며 raw 표 입구는 실제138슬롯(0..137)을 읽는다. 원문의0번 NULL observer를 허용하고256칸 호스트 배열을 Native 유효 범위로 확대하지 않는다. 메뉴/Source 정리 뒤 현재 표 owner를 명시 반납해 Registry 순환 소유를 피한다.

6. 7b는 기존 장기 CS 수신 본문·인자·캡처를 바꾸지 않고 Native4b4790을 canonical로 기록하는 한 행만 수정했다. 기존0b/C3/69는 이미 전환된 기준선을 보존했으며 새 완료량으로 더하지 않는다.

## 근거와 한계

같은 호스트의 정밀 디컴파일(클라이언트2026-11-03, R2026-11-07)을 읽었다. 생성 원문 변경0이다. `extracted/cpp-m6-delayed-prep/l1-bridge/source-route/`에19함수 full C/inclusive PE와 실제 표/소유권 대조를 보존했다. 적용 원시는 `rebase-1173/`의33파일 before/after/merge.json이다. 별도7b 사본은 `extracted/cpp-m6-client-default-canonical-patch/`의 한 hunk다.

전수 장부는 `source-route/canonical-status/`의 manifest.json·receiver-92-bootstrap-3.tsv·GROUPS.md다. 앱92행과 bootstrap3행의 고유89함수 전체를 기록하고 폐기7f를 별도로 분류했다. 미전환은 기존 함수 본문 전체가 없다는 판정이 아니라, 장기 고정 F/선행 자연 반환/첫 실제 Runtime 하위의 현재성 감사가 남았다는 뜻이다.

이번 범위는1a/1b의 배송과 State7 인계다. 모든 State19/메인 종료의 물리 장치 수명, 다른 F/observer의 전체 device pin,각 Node/VT Poll/closed/depth/큐 반납은 l3/l4에서 마감해야 한다. 강한 이전 World나 가짜 수명 token으로 이를 완료 처리하지 않는다. l2 ring·분포/Config·프레임 enqueue,자원 owner n1~n7 및 M7 준비본도 별도 미적용 상태다.

## 컴파일과 허용 확인

첫 빌드는 EditorLocalTransport의 기존 Registry 선언/정의를 중복 추가한 부분과 ClientSubscriberGlobals 완전 형식 include 누락으로 실패했다. 종료 뒤 신규 중복 선언/정의만 제거하고 include를 추가했다. 원래 Registry 본문과 첫 실패 로그는 보존했다. 교정 후 두 제품 Release/x64 빌드는 종료0·경고/오류0이다.

현재 빌드의 허용 비멀티 회귀에서 격리1920×1080 Tutorial1 정상 UI 다리 배치·서버SID15003/TYPE82/(63,49)·3eh0→1·90frame·정상종료0과 보호2800/설치/EXE 전후 hash 불변을 확인했다. 실제 값은 기계 보고서와 `extracted/cpp-resume-1174/`의 build/fix·frozen/final-source·live summary/TSV/PNG가 기준이다. 빌드 중 제품 소스,게임 중 소스/실행물은 변경하지 않았다.

새 MP/패킷/통신/단위·Source 전투 인계·공중3종 전용·포획/희생/공격/승리 검사는0이다. 허용 비멀티 회귀를 MP 실행 성공으로 확대하지 않는다. 현재 부분 단위를 문서화하고 로컬 커밋하며 전체 M6-l 및 목표는 계속 진행한다. push하지 않는다.
