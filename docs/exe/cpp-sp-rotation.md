# cpppj SP 비트 위치 회전

2026-10-13, 마지막 디컴파일 수행 PC **VM-W11-CODEX**. 원본/클론 게임·GUI를 실행하지 않고 10.78의 실제 `00491680`과 초기화/읽기/쓰기/난수를 읽기 전용으로 내보냈다. CD 10.72에는 대응 회전 몸체가 없다. 목록은 `tools/ghidra/sprotate-functions.json`, 출력은 `extracted/sprotate/originals/`, 로그는 `extracted/sprotate-export.log`다.

`ScrambledSpStore::Rotate()`가 다음 원본 순서를 복원한다.

1. `0054db24`가 0이면 기존 실제 `Initialize()`를 실행한다. 시각 재시드·offset `Next(100)`·순열 섞기의 난수 소비·항등 순열로 덮어쓰기·32개 9DWORD 확보는 그대로다.
2. **`00568af8`은 저장소 offset 자체**다. DWORD로 3을 더한 결과를 signed 비교해 9600보다 클 때 한 번만 9600을 뺀다. 9600과 같으면 유지한다. 부호 있는 덧셈의 넘침은 DWORD 비트로 보존한다.
3. 항등 순열의 배열 0~31, 소유자 0~8 순서로 전체 288DWORD를 왼쪽으로 3비트 순환한다. 원본은 sign→shift→OR1을 세 번 수행한다. C++의 `std::rotl`은 같은 DWORD 연산이다. offset과 물리 배열을 같이 옮기므로 모든 플레이어의 실제 SP DWORD가 유지된다. 추가 난수는 소비하지 않는다.

사제 `0x25a`는 HP가 이미 최대여도 이 함수를 실행한다. 기존 `RawPriestRegen`은 sample `Next(9)`와 expected offset `0054d4e0` 증가를 담당한다. 실제 월드는 SP 초기화가 만든 초기 offset을 `state.patch.sequence`에 같이 공급한 뒤, `patchAudit` callback에서 `store.Rotate()`와 `state.patch.observedSequence=bit_cast<int32_t>(store.Offset())`를 연결한다. 별도 관찰 카운터를 증가시키면 실제 Get의 비트 위치와 관계없는 상태가 된다. 원본 초기화 자체가 `0054d4e0`도 설정하므로 lazy 초기화가 이벤트 중 발생하는 호스트는 그 초기화 부수 효과도 동기화해야 한다.

`tools/decomp_sprotate_oracle.py`는 실제 PE 몸체 다섯 개만 허용하고 malloc(0x24)의 힙 채움과 시각 재시드만 명시 대체한다. 기대 회전·비트값·난수는 C++이나 Python의 회전식으로 계산하지 않는다. 61개 입력은 lazy 회전, 여러 힙 sign 패턴, 모든 9소유자에 다른 DWORD 저장, 회전 사이 재저장, 32/64 회전과 재초기화, 실제 offset0/97/98/99에서 9600 포함/초과 경계를 다룬다. 각 명령 뒤 **전체 288DWORD**·논리 SP9개·offset·난수 상태·별도 init_copy를 관찰한다.

일반 C++ 검사는 저장 fixture만 읽는다. native 전체 자료 대조, 9600 포함/초과 및 SP/난수 보존, lazy 재시드 누락, fullHP 실제 사제 회복과 current offset 공유를 검사한다. 검증 완료 로그와 관찰 수는 인수인계에 별도로 기록한다. 이 회전 복원만으로 건설/캠페인 완주를 증명하지 않는다.
