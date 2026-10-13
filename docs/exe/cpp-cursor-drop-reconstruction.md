# 커서 내려놓기 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS와 [로컬 건설 인계](cpp-construction-local-reconstruction.md)를 읽었다. 시작 트리는 깨끗하고 현재/마지막 디컴파일 PC가 일치했다. 새 `cursordrop` **7/8/8개**를 읽기 전용 Ghidra로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본 게임·analyzeManager·클론 창은 실행하지 않았다.

## 복원한 순서

[`RawCursorDrop`](../../cpppj/src/client/RawCursorDrop.h)는 **10.78 `004473e0` ↔ CD/10.37 `0043d660`**의 전체 몸체다. 대응은 신뢰도 A, `callers/strong` 0.681. 원본처럼 인자·반환값 없는 함수이며 커서 상태 전역을 현재 시점에서 읽는다.

1. 잠금 깊이가 0일 때만 커서 위치 판정을 갱신한다. 갱신 뒤 legal이 0이면 반환한다. 미리보기 SID는 이 시점에서 캡처하므로 이후 prepare가 전역 번호를 바꿔도 원래 객체를 처리한다.
2. 준비 → x/y 각각 **floor(double(float 좌표) + 0.5)** 후 float 저장 → 준비 순서다. 음수 절반은 0 쪽으로 반올림한다. 두 번째 준비가 바꾼 현재 좌표는 월드 이동에 그대로 쓰고, 새 건설은 다시 반올림한 지역 사본을 쓴다.
3. 강조를 0으로 지우고 **droppiece.wav**를 반복/음량/좌우/우선/한도 모두 0으로 재생한다. GUI가 나중에 거부하더라도 이 효과들은 앞서 발생한다.
4. GUI 부모가 없으면 월드 분기다. 커서 inputFlags의 **하위 BYTE bit 0x80**이 켜졌으면 가상 +0의 연관 번호로 기존 조각의 **Pop(x,y,0)**을 부른 뒤 미리보기를 플래그 0으로 삭제한다. 꺼졌으면 기록기가 켜진 경우 타입 **+4 이름 문자열**과 현재 좌표/인자/방향을 기록하고, 지역 좌표를 다시 반올림 → 잠금 증가 → 로컬 건설 배치를 부른다.
5. 월드 분기에서만 현재 costFlags의 **하위 BYTE bit 0x40**에 따라 **-현재 타입 비용**을 로컬 SP에 가산한다. 가산 뒤 타입/화면 전역을 다시 읽고, 절삭 비용을 `(screenX - signed(footX << 4)/2 - cursorWidth/2, screenY, 0)`에 표시한다. 정수 산술은 DWORD 넘침을 보존하며 `/2`는 음수도 0쪽으로 절삭한다. 이후 battle이 켜지고 blocked가 0이면 전투 표시를 갱신한다.
6. GUI 부모가 있으면 가상 +0x28로 반환 플래그 DWORD를 얻고 미리보기 연관 번호를 **두 번** 조회한다. 두 번째 조회 뒤 **+20 WORD 기존 플래그와 품질 BYTE(패치 +30 / CD +26)**를 캡처한다. 반환값과 기존 플래그의 겹침이 있으면 invalidGui만 부르고 즉시 반환한다.
7. GUI가 허용하면 **detach(0) → 반환값의 하위 WORD 쓰기 → parentSid 검사 → attach(parentSid,0x10) → GUI 수신 통지** 순서다. 수신 뒤 현재 inputFlags가 이동을 뜻하면 **첫 조회 번호** 객체를 삭제하고, 이동이 아니며 기존 플래그에 8이 있으면 **두 번째 조회 번호 DWORD 전체·기존 WORD 플래그·캡처 품질**을 생산 칸 반환 helper로 보낸다.
8. 성공 경로는 잠금 증가 → refresh(0,0,1) → cursorGump가 있으면 close(3,0,0) 후 0 → 미리보기 전역 0 → 활성화(1) 순서다. 새 건설은 앞의 지역 배치 전 잠금을 포함해 보통 깊이 **2**, 이동/GUI는 **1**이다. 깊이가 처음 0일 때만 활성화(0)와 하드웨어 커서 번호 1을 요청한다.

CD의 Ghidra 출력은 비용 차감을 world/GUI 분기 밖에 표시하지만, 실제 **0043d6f0의 지역 표시값 1과 0043d728의 GUI 경로 0 대입**, **0043d917의 검사**가 GUI 비용 차감을 막는다. 패치판과 결과는 같다. GUI 반환 플래그를 ushort라고 표시하거나 `unaff_BP`를 쓰는 출력도 실제 EAX/DWORD 검사·하위 WORD 쓰기와 다르다. 두 조회 결과/품질의 지역 변수 표시가 모호해 어셈블리와 전체 원본 명령 실행을 함께 대조했다.

SP 표시의 CRT 변환은 32비트 범위 제한이 아니다. **64비트로 절삭한 정수의 하위 DWORD**를 사용하므로 3,000,000,000도 그대로 전달하며 NaN/64비트 범위 밖의 x87 불확정값은 하위 DWORD가 0이다. 첫 C++ 재생에서 이 차이가 드러나 64비트 절삭으로 수정한 뒤 전체 관찰을 통과했다.

## 실제 연결과 계약

[`MakeCursorDropHooks`](../../cpppj/src/client/RawCursorDrop.cpp)는 같은 풀의 **RawConstructionLocal::Place**, **SquidReward::AddSp**, **SoundPlayer::PlayByName**을 연결한다. 패치 SP는 실제 ScrambledSpStore, CD SP는 기존 로컬 float를 쓰며 AI 지갑 갱신도 실제 보상 계층이 처리한다. 로컬 플레이어 전역과 타입/상태 자료의 일관성·수명은 호출자가 보장한다.

[요청 통합 검사](../../cpppj/tests/ConstructionRequestTests.cpp)는 두 판본에서 **커서 → 실제 로컬 factory/abstract 예측 → 요청 필드 → 전체 MayPlace → 서버 확정/factory → 통지 → 예측 정리 → 서버 배치/소유자**까지 이어진다. 커서에서 (20.25,21.5)를 (20,22)로 반올림한다. 단가 32.75, SP 1000이 **967.25**로 한 번만 줄고 지갑은 32만 차감된다. 실제 소리 표/재생 계층이 droppiece.wav를 적재/재생하며 장치 호출만 기록한다. 지면 거부는 예측을 보존하고 허용 전역을 켠 재요청은 아홉 서버 조각으로 교체한다. 서버 처리 뒤에도 SP는 967.25다.

Form/Gump 생성·가상 연관 조회/부착/삭제, 실제 좌표 판정/준비, 생산 칸 반환 `004468b0`/CD `0043e930`, 타입별 Pop, 기록기·표시 효과는 아직 명시 경계다. 커서 활성화/하드웨어 커서 선택도 경계이며 실제 입력 루프에 연결하지 않았다. 통합 검사의 Pop/삭제 공간 효과는 좌표·해시 삽입/해제에 제한한다. wire 메시지 전송/수신은 연결하지 않았다.

C++는 필수 경계 누락/빈 타입 자료, 예약/풀 밖 미리보기, 실제 사용 시 없는 타입·비유한 좌표·잘못된 이동 연관 번호를 거부한다. GUI 플래그 쓰기는 SidPool::AllocatedBytes를 사용하므로 해제된 슬롯을 거부한다. GUI parentSid 0은 원본 assert가 발생하는 지점에서 예외로 거부한다. 원본 assert 이후 계속 진행하는 오류 경로를 재현하지 않는다. 효과가 예외를 던지면 앞선 효과를 되돌리지 않는다. raw Form은 일반 Squid와 필드 의미가 다르며 호스트 객체 계층이 아니라 SID 슬롯 자료로 표현한다.

## 독립 검증

[`decomp_cursordrop_oracle.py`](../../tools/decomp_cursordrop_oracle.py)는 전체 커서 몸체·반올림/스냅·CRT floor/ftol·CD 잠금 조회/증가를 실제 명령으로 실행한다. CRT는 정적 PE 초기 상태의 x87 경로를 사용한다. 위치 판정/준비·Form/Gump 가상 효과·강조/소리/기록/로컬 배치/SP/표시만 대체한다. CD SP 가산의 ST0 반환 ABI는 도구의 **fldz 한 명령**으로 공급하고 루트가 이를 실제로 pop하는 것까지 검사한다. GUI 메시지 식별값 901/902와 부모 123은 합성 입력이며 실게임 메시지 등록 번호로 주장하지 않는다.

새 저장 입력 **2,058개**(686×3), 두 x87 정상 반환 **4,116회**, 누적 **469,068개**(467,010 + 2,058)다. [fixture](../../cpppj/tests/fixtures/cursordrop-x86.tsv)·[근거 JSON](../../cpppj/cursordrop.json)에 사건 순서/인자와 최종 전역/raw 14개를 저장했다. 기대 출력은 원본 실행에서만 얻는다. 조회/준비/두 번째 연관 조회/detach/GUI 수신/로컬 배치/SP 가산 경계의 상태 변경도 입력에 포함한다. 전체 정상 진입/반환, 스택·보존 레지스터·x87, 허용 실행/쓰기 범위·SHA·사건 수를 감사하며 원본 assert/게임/OS 호출은 0이다.

새 C++ 검사 **5개**와 기존 전체 건설 통합 검사를 확장했다. Release 경고/오류 **0**·CTest 내부 **594개·실패 0**(136.22초, 전체 136.25초), 근거 감사 **93종 모두 통과**. 새 변이 12개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과).

## 다음 작업

1. **생산 칸 조각 반환 `004468b0` ↔ CD `0043e930`**의 본체·contained/Form 생성과 품질/방향 효과를 복원해 로컬 배치·GUI 반환 양쪽에 잇는다.
2. 요청의 유닛 밀어내기/실패 정리와 확정의 실제 사제 선택, 사제 이동/건설 시작·진행·완료를 잇는다.
3. 같은 조각 삭제 플래그 `0x2000000`·타입별 Pop/삭제 공간 효과·메시지 `0x2f`/`0x6c` 수신·실제 건설 메뉴/커서 GUI를 연결한다.

**게임에서 건물을 지어 미션을 완주하는 흐름은 아직 연결되지 않았다.** 한국어 3차 → outpost/LAN 4차 → 요구사항/MCP 5차 순서를 유지한다.

## 재실행

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name cursordrop
python -X utf8 tools/decomp_cursordrop_oracle.py
python -X utf8 tools/decomp_cursordrop_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기본 CTest는 저장 fixture만 읽는다. 원본 게임/OS/창·Python은 실행하지 않는다. 새 내보내기/로그는 Git 제외 `extracted/cursordrop/`·`extracted/cursordrop-*.log`에 있다. 변이 도구는 `--focused --only cursor-drop-…`로 고른 12개만 저장소 밖 사본에서 실행한다. 변이 전 사본은 전체 검사를 통과해야 한다.
