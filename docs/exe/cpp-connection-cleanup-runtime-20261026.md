# 단위457: 연결표 정리와 실제 지연 반환

## 기준과 독립 관측

작업 PC는 **HJOW-Athlon**이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 이번에 산출물을 다시 생성하거나 수정하지 않았다. PATCH 10.78의 여섯 실제 PE 몸체를 [독립 도구](../../tools/decomp_connection_cleanup.py)로 제품 변경 전에 격리 실행했다.

| 실제 함수 | 처리 |
|---|---|
| 004df440 | 진입 depth 보관, flags 비트40, depth0에서 고정 소멸과 free |
| 004df520 | -31..-1의 현재 pending 종료와 개수 감소 |
| 004e2940 | 음수 pending 위임 또는 양수 연결 및 별도 버퍼 종료 |
| 004e29c0 | 양수 연결0..21 순회, 동기화 토큰 정리, 버퍼 보존 |
| 004e2a10 | 진입 server 비영에서만 양수 연결1..8 종료 |
| 004df4a0 | pending0..31 종료, 개수·동기화·양수표 보존 |

**580관측/중단268관측**은 두 x87 환경, 수신 깊이, 이미 닫힌 flags, 첫/끝/빈 슬롯, 같은 노드 별칭, 음수 한계와 양수22 진단을 포함한다. 소멸 callback이 뒤 슬롯·동기화·개수·서버·현재 depth를 바꾸는 입력으로 현재값 재조회와 진입값 보관을 구별했다. 각 중단의 완료된 BYTE/store/호출 prefix도 비교한다.

정리 부모와 지연 반환은 실제 CPU 명령이며 `004e03d0` 고정 연결 소멸, `004e4354` CRT free, `004e0620` 진단만 명시 외부 장치다. 장치는 관측 메모리를 실제로 해제하지 않으므로 중복 포인터 입력을 힙 할당기 안전성의 증거로 확대하지 않는다. 원본 게임이나 OS는 실행하지 않았다. [최초 TSV](../../cpppj/tests/fixtures/connection-cleanup-x86.tsv)와 [Native 보고서](../../cpppj/connection-cleanup-native-20261026.json)는 C++ 출력으로 다시 만들지 않았다.

정밀 자료의 CD 10.72 자동 대응은 `004df440→004199d0`, `004df520→0041a180`, `004e2940→004bffb0`, `004e29c0→004c0030`, `004e2a10→004c0080`이다. `004df4a0`의 직접 대응은 확정되지 않았다. 이번 CD 몸체의 실행·동등성 증거는 없다.

## 제품과 실제 Editor 연결

`RawConnectionCleanup`은 22연결/22송신버퍼/32pending과 기존 동기화 토큰·pending 개수의 같은 현재 뷰만 받는다. 전체 양수 연결 정리와 서버1..8 정리는 버퍼표를 보존한다. 개별 양수 종료는 연결 처리 후 그때의 버퍼 토큰을 읽어 free하고 그 칸을0으로 쓴다. pending 음수 종료는 callback 뒤 현재 개수를 읽어 감소시키고 pending칸0, 개수 store 순서를 유지한다. 전체 pending 정리는0칸도 방문하며 개수를 다시 세지 않는다.

`DeferConnectionClose`는 소멸 callback이 depth를 바꾸더라도 진입 depth0에서 선택된 free를 계속한다. 실제 `EditorLocalTransport::Disconnect`도 같은 몸체를 사용한다. 수신 중이면 closed 표식만 남고 기존 RawPacketDispatch의 최외곽 복귀가 반납한다. 이미 닫힌 수신 끝을 반납하는 기존 Retire 경계는 그대로 유지했다.

Editor의 기존 `id<-32` 예외는 원문에 맞춰 자연 반환으로 고쳤다. 원문 pending은 -32 이하와0 이상을 선택하지 않는다. 양수22 이상의 원문 진단이 반환했을 때의 물리 범위 밖 접근은 호스트 span 검사로 명시 거부한다. 이 손상 입력을 유효한 연결 종료로 가장하지 않는다.

`ClientConnectionGlobals`는 실제 고정 표의 주소를 Client 수명에 보관하고 Session 대여/반납에서 초기화하지 않는다. 동기화는 기존 ClientLocalStateGlobals의 같은005c8bf4를 빌린다. **기존 전송의 생성·재배치 writer는 아직 이 표를 사용하지 않는다.** 따라서 이 표가0이라는 이유로 현재 캠페인/Editor 연결이 없다고 판정하거나 전체 Native State cleanup을 성공시켜서는 안 된다. 다음 연결 단계에서 실제 연결 식별자/등록·해제와 같은 표를 공유해야 한다.

## 검증 기록

빌드1은 이미 닫힌 수신 경로도 사용하던 Retire 선언/몸체 제거로 C3861 한 건이 발생했다. 기존 경계를 복원하고 명시 종료만 새 부모에 연결했다. 첫 원시는 `extracted/cpp-connection-cleanup-457-build1*`에 보존한다. Source2 build2는275.873초·경고/오류0이다. 첫 집중의 Native580·Editor·관련 회귀는 통과했으나 새 Client 보관 사례 하나는 게임 자료 경로가 없어 netstorm.tarc 읽기에 실패했다. Source3에서 실제 자료 경로만 제공했고, Source4에서 sender/두 수신시각 보존 대조도 추가했다. Native 기대값·기존 단언·제품 본문은 바꾸지 않았다.

최종 **Source4(2049소스) / build4**는23.796초·경고/오류0, 집중2는230고유/230관측·신규6개·실패0이다. 게임 SHA는 `1eaab3e9f634751b1a69b89d246287442ce6b40c7830c1f525e3982a3e7e0046`이며 Source2 이후 제품 본문과 게임 EXE는 동일하다. 원본/CD 등 보호1821파일과 기존 fixture651개(TSV648개)를 실행 전후 대조했고 모두 같다.

[실행 보고서](../../cpppj/connection-cleanup-runtime-20261026.json)의 실제 소스/네 실행 파일/Native/원시 실패 SHA와 `extracted/cpp-connection-cleanup-457-{source4,build4,focused1,focused2,test-preservation}`를 함께 본다. 이번에는 실제 창·전체 CTest를 반복하지 않았다. 등록된3120개 전체를 실행 통과로 합산하지 않는다.

기존 Editor 테스트 본문/단언은 유지하고 Native580·필요 장치/실패 prefix·음수 경계·Client 장기 보관4개와 실제 Editor의 음수 한계/소멸 중 depth 변경2개를 추가했다. 기능 묶음 검증은 [456의 전체3114개](cpp-state-hud-runtime-20261026.md)를 마지막 전체 결과로 유지하며 이번에는 영향 부위 집중 검사를 수행한다.

## 다음 작업

실제 로컬 연결 생성·수락·재배치·해제를 Client의 같은 표에 연결한 뒤 `CloseAllConnections/EndConnections/disconnect`를 전체 Native State 부모에 설치한다. Player/Runtime 반납 coordinator, State1 prefix와 Tick의 Edit 예약 소비도 이어서 연결한다. 이번 원문 본체·실제 Editor 종료 검사는 전체 Edit 진입, 미션 자연 완주, 전체 화면/음향 동일성의 완료 증거가 아니다.
