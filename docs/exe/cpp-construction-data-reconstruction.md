# 건설 프로세스 상태 메시지 0x37 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽고 깨끗한 작업 트리에서 시작했다. 현재 PC와 마지막 디컴파일 PC가 같다. `constructiondata` **4/4/4개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본 게임·analyzeManager·클론 창은 실행하지 않았다.

## 원본 몸체와 패킷

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| 상태 쓰기 | `004426d0` | `004d0430` |
| 상태 읽기 | `00442760` | `004d04d0` |
| 메시지 수신 | `004427b0` | `004d0540` |
| 실제 프로세스 가상 쓰기 래퍼 | `00443bd0` | `004d2b20` |
| Kernel 조회(응답 경계) | `00471aa0` | `004ed640` |

패킷은 little endian이다. 9바이트 헤더는 첫 바이트 **1**, 다음 바이트 **0x37**, +2의 전체 길이 WORD, +4의 form DWORD, +8의 변경 비트 BYTE다. 두 첫 바이트는 PE 전역값이며 API에서 현재 값을 공급할 수 있다. payload는 다음 순서다.

| 변경 비트 | 값 | 폭 |
|---|---|---|
| 1 | 부모 번호 | DWORD 4 |
| 2 | 시작 시각의 저장된 비트 | double 8 |
| 4 | 끝 시각의 저장된 비트 | double 8 |
| 8 | 객체 +0x20의 추가 값 | BYTE 1 |

최대 길이는 **30바이트**다. 상위 네 비트는 헤더에 그대로 남지만 payload를 추가하지 않는다. 시각은 실수 계산 없이 비트 그대로 옮겨 NaN·음수 0도 보존한다. 추가 BYTE의 게임 내 의미는 아직 확정하지 않았다.

쓰기의 force가 켜지면 먼저 dirty를 **0xff**로 둔다. 부모 **인자가 0이 아닐 때** bit 1을 켜고 부모를 쓴다. 길이와 변경 비트를 기록한 뒤 dirty를 **0**으로 만든다. 가상 래퍼는 `includeParent || force`일 때 실제 프로세스의 부모를 넘긴다. 기존 dirty의 bit 1이 켜져 있어도 부모 인자가 0이면 원본 쓰기는 부모 payload를 만들지 않는다. 따라서 이 특수 출력과 읽기를 무조건 왕복 가능한 형식으로 바꾸지 않았다. 패치 쓰기의 반환과 달리 CD EAX는 명확한 길이 반환 계약이 없어 C++ 반환은 **헤더에 쓴 길이**로 정의한다.

읽기는 부모 출력부터 0으로 두고 선택된 필드만 복원한다. 기존 **dirty와 sound는 보존**한다. 선언 길이·헤더 종류를 검사하지 않는 원본 몸체를 유지하며 메시지 등록/전달 계층이 이를 책임진다. C++ API는 실제 span 길이를 검사한다. null 필드 포인터는 시각/추가 비트가 없는 입력에서만 허용한다.

## 수신과 실제 연결

수신은 먼저 form DWORD를 캡처한 뒤 raw +11의 free bit 1을 본다.

- free이면 mask가 정확히 **0xff**여야 한다. 56바이트 할당 경계 → 원본 인라인 생성자의 dirty=0xff, 시작/끝=0 → 패킷 읽기 → 현재 프로세스 타입·부모 DWORD·캡처한 form DWORD·flags **0x10**으로 부착한다. 할당이 타입이나 패킷을 바꿀 수 있어 타입/본문은 경계 뒤 다시 읽고 캡처한 form은 유지한다.
- 기존 form이면 부모 bit 1이 없어야 한다. raw +18의 **DWORD 전체**로 Kernel을 조회한 뒤 해당 건설 필드를 읽는다. 프로세스가 없어도 실제 필드를 선택하지 않는 비트만 있으면 읽기를 끝낼 수 있다.

원본 수신 생성자는 추가 BYTE와 sound를 초기화하지 않는다. 응답 경계의 저장소가 제공한 값을 유지하고 추가 BYTE는 전체 읽기로 덮는다. C++ `ConstructionProcessFields`는 안전한 기본값 0을 제공하므로 실제 Start의 추가 BYTE 기본값도 0이다. 이는 원본의 미초기화 메모리 값을 추정한 결과가 아닌 **호스트 저장소 계약**이다. CD 수신의 SEH 설치/복구는 원본 실행기에서 검증했고 C++에서는 예외 계약으로 옮긴다.

[`RawConstructionData`](../../cpppj/src/o/RawConstructionData.h)의 Write/Read/Receive와 실제 WriteProcess를 추가하고 [`ConstructionProcessFields`](../../cpppj/src/o/RawConstructionProcess.h)에 추가 BYTE를 붙였다. 기존 form의 Receive는 실제 `SquidProcessHost::FromForm`·Kernel·`ConstructionProcess`에 연결했다. 새 객체 할당과 **수신한 지정 form 번호로 부착**은 명시 hooks다. 현재 host의 Attach는 새 번호 확보만 지원하므로 지정 form 부착을 구현했다고 간주하지 않는다. asset용 Take와 프로세스 form의 Take는 원본의 계약이 달라 그대로 공유하지 않았다. 메시지 디스패처/소켓/월드 전체 동기화도 후속이다.

짧은 버퍼, 잘못된 form 번호/종류/연결, free의 부분 mask, 기존 form의 부모 bit, 선택 필드가 있는 null 대상, 다른 풀 host, 누락 hooks, 할당 실패는 원본의 assert/null 접근 대신 C++ 예외로 거부한다. 쓰기와 읽기의 길이 검사는 필드/패킷 변경 전에 한다. 새 객체 저장소의 소유권과 부착 실패 정리는 hooks 공급자가 책임진다.

## 독립 원본 대조와 검사

[`decomp_constructiondata_oracle.py`](../../tools/decomp_constructiondata_oracle.py)는 세 PE의 쓰기·읽기·수신 생성자/분기·가상 래퍼를 전체 정상 반환까지 실행한다. **객체 할당·Kernel 응답·지정 form 부착만 명시 경계**다. OS/게임은 실행하지 않는다. 입력별 두 x87 제어값 **639/895**로 실행해 ABI/스택/보존 레지스터/x87 균형·SEH 복구·실행/쓰기 허용 범위·주변 form 불변을 확인했다.

각 PE **1,780개**는 쓰기 1,034개, 래퍼 40개, 읽기 512개, null 읽기 32개, 기존 수신 144개, 새 수신 18개다. 모든 256개 mask·부모 DWORD·force/래퍼 조건·임의 payload/NaN 비트·추가 BYTE·변경된 헤더 전역·DWORD Kernel 번호·할당 중 타입/패킷 변경을 포함한다. 새 저장 **5,340개**, 두 정밀도 정상 루트 반환 **10,680회**, 누적 **489,939개**(484,599 + 5,340)다. 원본 assert/게임/OS 호출은 0이다. [fixture](../../cpppj/tests/fixtures/constructiondata-x86.tsv)와 [근거 JSON](../../cpppj/constructiondata.json)의 SHA·행 폭·입력 순서·실제 진입/반환/경계 집계도 감사한다.

[`ConstructionDataTests.cpp`](../../cpppj/tests/ConstructionDataTests.cpp)의 새 **7개**는 원본 출력 필드·부모 출력·사건 순서·패킷 앞뒤 전체 64바이트·주변 form을 비교한다. 래퍼는 실제 host에 붙은 form 5/6을 사용한다. 통합 검사는 실제 Start → 전체 전송 → ResetEnd → 끝 시각 부분 전송 → 기존 form 수신 → 원래 완료 시각에는 Kernel 유지/HP 53 → 새 완료 시각에는 form 삭제/HP 100/abstract 해제까지 연결한다. 표시·음향·부모 Unpop/재등록은 명시 경계이며 GUI 플레이 검사가 아니다. 모든 최대 패킷 부족 길이, 불명확한 선언 길이, 무효 form/다른 종류/오래된 연결·상위 비트 Kernel 번호도 검사한다.

Release 경고/오류 **0**·CTest 내부 **648개·실패 0**(124.34초, 전체 124.43초), 근거 감사 **100종 모두 통과**. 새 상태 전송 변이 **8개 모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). 8개 변이는 dirty 초기화·부모 폭·force의 부모·읽기의 dirty/추가 BYTE·현재 타입·캡처 form·Kernel DWORD 폭을 바꿨다. 현재 트리의 전체 CTest와 변이 전 사본의 전체 검사를 모두 통과했다.

## 다음 작업

지정 form 부착(`0041bd20`/CD `0048f590`)의 기존 free form 경로와 메시지 **0x37**의 등록/디스패처를 실제 월드에 연결한다. 이어 건설 표시 효과(`00423840`/`00423900`, CD `004eab60`/`004eac30`), contained 삭제(`004b1a90`/CD `004eb310`), 타입 준비 소리(+0x90), 가상 표시/Unpop/재등록, 하위 경로/접근점·자원 선택/정렬·공간 Pop·소유 사제 이동/contained·타입별 파생 효과·법적 위치/미리보기·건설 메뉴/커서 GUI를 복원한다. **게임 내 건설과 미션 완주는 아직 불가능하다.**

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructiondata
python -X utf8 tools/decomp_constructiondata_oracle.py --verify
cmake --build cpppj/build --config Release
cpppj/build/bin/Release/netstorm_tests.exe --filter ConstructionData_
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_mutation_check.py --focused --only data-captured-form
```

로그는 Git 제외 `extracted/constructiondata-*.log`다. 변이는 별도 사본에서만 적용한다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시는 없다.
