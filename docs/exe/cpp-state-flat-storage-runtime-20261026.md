# State 정리의 같은 물리 저장소 연결 (단위453)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 수행일은 **2026-10-23**이다. 10.78 보호 PE와 같은 호스트의 정밀 C를 읽기 전용으로 사용했다. 디컴파일 결과·원본/CD·AGENTS.md를 변경하지 않았다.

## 원문과 먼저 고정한 관측

PATCH `004328a0`은 `0054d4e8`의 첫255DWORD를0으로 만든다. 이 표는 `00433c30`의 `PlayerNoMake` 경고 비트이며, 인접한 SP 추적값이나 TotalMade가 아니다. PATCH `004ab940`은 `005c7c44`가 가리키는 spot 표의65536BYTE를0으로 만든다.

[독립 관측 도구](../../tools/decomp_state_flat_reset.py)에서 제품 변경 전에 두 함수 × 네 입력 패턴 × 두 x87 설정의 **16관측**을 고정했다. 외부 함수 치환 없이 실제17/18BYTE 몸체를 실행하고, 모든 DWORD store의 주소·폭·순서·값, 전체 버퍼 체크섬, 인접32BYTE, 비휘발 레지스터와 FPU 보존을 대조했다. [최초 fixture](../../cpppj/tests/fixtures/state-flat-reset-x86.tsv)와 [Native 보고서](../../cpppj/state-flat-reset-native-20261026.json)는 C++ 출력으로 덮어쓰지 않았다. 별도 PE 직접 읽기로 경고1020BYTE의 최초0도 확인했다.

**CD 대응표 정정:** 자동 매칭은 PATCH `004328a0`에 CD `00487900`을 제시하지만, CD의 실제 `00487900`은 포인터 `0052fe48`에65536BYTE를 쓰는 spot 정리다. 두 함수를 같은255DWORD 경고 정리로 복원하지 않는다. 이번 제품 연결은 PATCH 전용이다.

## 실제 저장소와 소비 경계

- `ClientLocalStateGlobals::PlayerNoMakeWarnings()`는 같은255DWORD를 Client 수명에 보관한다. Session 생성·대여·반납이나 SP 초기화로 이를 지우지 않는다. 경고 패킷 수신 부모 `00433c30` 전체의 제품 연결은 후속이다.
- 두 작은 몸체는 실제 물리 범위만 변경한다. 추가 버퍼 칸은 보존하고, 지원 크기보다 짧은 호스트 버퍼는 쓰기 전에 거부한다. 원본의 잘못된 포인터 접근·일부 쓰기 후 메모리 오류까지 복원했다고 주장하지 않는다.
- `ResetClientStateWorldStorage`는 같은 Client의 World 대여 반납을 확인한 뒤 일곱 Effect를 선택 실행한다. TotalMade·Hash·경고 표·spot·Island·Chunk·Graph의 기존 원문 몸체를 재사용한다. 다른 Effect는 명시 거부하며 Pool·AI·Player를 새로 만들지 않는다. Chunk 뒤의 별도 mode1 쓰기는 원래 State 부모 위치에 남긴다.

이 연결은 Native State 부모가 재사용할 실제 하위다. 아직 자연 `Tick`의 전체 cleanup/prefix와 Edit 화면 설치까지 연결한 상태는 아니다. 전체34개 Effect·20개 pool-header·Runtime/Player 반납 coordinator·초기 HUD·State 확정은 계속 남아 있다.

## 검증 상태

Source1(2,039소스) Release 증분 빌드는159.023초, 종료0·경고/오류0이다. 집중104고유/108관측·실패0이며 신규4개와 기존 편집 설치50개를 모두 실행했다. Native16관측의 전체 버퍼와 이웃 보존, 짧은 물리 거부, 같은 Client/주소, World 대여 중 거부, 다른 Client 저장소 거부, 일곱 Effect의 선택 범위와 Graph 예약 highBYTE·틈3행·stack, Island padding, 집계256번째 칸, Hash 단계·Chunk mode·Pool 보존을 확인했다.

원본/CD 등 보호1,821파일과 동결 소스·제품 SHA는 검사 전후 동일했다. [실행 보고서](../../cpppj/state-flat-storage-runtime-20261026.json)와 원시 `extracted/cpp-state-flat-storage-453-{source1,build1,focused1}`를 본다. 이번 변경은 실제 메뉴 입력·타이머·저장 경로를 바꾸지 않으므로 GUI와 전체 CTest는 이번에 반복하지 않았다. 기능 묶음 통합 시 같은 최종 제품으로 확인한다.
