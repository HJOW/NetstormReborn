# 지정 ProcessForm 재수신과 Kernel 소유권

현재 복원 대상은 10.78 캠페인 1-1이다. 10.72는 판본 차이를 구별하기 위한 비교 자료다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-13.** AGENTS.md·두 LEFT_JOBS와 현재 호스트를 확인했고, 원본 게임/GUI를 실행하지 않았다. 보호 PE는 읽기만 한다.

## 원본의 실제 분기

`BaseProcess 0041bd20 / CD 0048f590`은 부모와 지정 Form SID를 프로세스 필드에 먼저 기록한다. 10.78은 부모가 free/dead이면 Kernel 등록과 통지를 건너뛴다. CD의 이 입력은 assert 이후 실행이므로 복원 API에서 거부한다.

살아 있는 부모에서 Kernel `00471950 / CD 004ed500`을 호출한다. 등록 함수는 첫 빈 슬롯을 기억하면서 **같은 포인터가 이미 등록됐는지 끝까지 검색**한다. 같은 포인터가 뒤 슬롯에 있으면 앞 빈 슬롯을 채우지 않고 기존 PID를 반환한다. 10.78의 타입 순환 helper `0049c2c0`는 이 검색 전에 실행되며, 중복 포인터도 순환 값을 전진시킨다. CD에는 이 helper가 없다.

지정 Form이 **free**이면 `Take(processFormType,SID)`를 호출하고 실제 프로세스 타입 BYTE·PID DWORD를 쓴 뒤 부모 체인에 부착한다. 프로세스 포인터가 이미 Kernel에 있는 경우에도 새 PID를 배정하지 않는다. Take는 SID FIFO/카운터를 건드리지 않으며 주변 payload를 보존한다.

지정 Form이 **live**이면 `Take`, `Unpop`, 타입/PID 쓰기, 부모 재부착, 삭제가 모두 생략된다. 기존 Form의 부모 getter와 PID가 전달 부모·Kernel 반환 PID와 같은지 확인하고 `OnAttached`만 호출한다. 전달된 프로세스 타입 인자가 달라도 기존 실제 타입은 유지된다. live client SID도 이 분기에서는 서버 Take 범위 검사를 거치지 않는다. **새 객체로 기존 live 슬롯을 교체하는 동작은 원본에서 부모/PID assert에 걸리는 입력**이다.

따라서 `SquidProcessHost::Reattach`는 Kernel이 이미 소유한 객체 참조를 받고 live 유지 또는 free 지정 수신을 수행한다. 신규 객체의 `Attach(unique_ptr,...)`는 그대로 free 지정 슬롯을 받고, 이미 소유한 같은 포인터를 넘기는 경우 참조 경로로 전환한다. 재수신 중 원본이 하지 않는 기존 다른 Form 삭제를 추가하지 않는다.

## 삭제 순서

`ProcessForm preDestroy 004ae240 / CD 004af450`은 현재 PID로 프로세스 포인터를 얻고 `OnFormDestroy(flags)`를 호출한다. **그 통지가 끝난 뒤 Form의 PID를 다시 읽어** Kernel 제거에 전달한다. 통지가 PID를 다른 등록 번호로 바꾸는 합성 입력에서는 그 다른 프로세스가 삭제되고 최초 포인터는 Kernel에 남는 실제 원본 결과를 보존한다.

Kernel 제거 `004719e0 / CD 004ed590`은 가상 소멸자 호출이 끝난 뒤 슬롯을 0으로 만든다. 기존 `unique_ptr::reset`만으로는 조회 슬롯이 소멸자 진입 전에 비는 차이가 있었다. `Kernel`은 raw 조회 슬롯과 단일 소유권 배열을 함께 두어 소멸자 안의 `Get`/`Find`/중복 등록은 기존 포인터를 보게 하고, 소멸 완료 후 슬롯/개수를 비운다. 기본 범위는 10.78의 `1..39999`, CD 비교 생성자는 `1..3999`를 사용한다.

전체 정리 `00471af0 / CD 004ed4d0`은 슬롯 1부터 끝까지 **한 번만** 순회한다. 패치는 가상 소멸자→현재 슬롯 0을 직접 수행하고 CD는 `Remove`를 호출한다. `Kernel::Clear`도 같은 순서를 유지한다. 소멸자가 이미 지난 빈 앞 슬롯에 새 객체를 등록하면 그 객체는 이번 Clear의 결과에 남는다. 아직 지나지 않은 뒤 슬롯은 같은 Clear에서 삭제된다. 소멸자가 미래 슬롯을 중첩 제거하면 이후 순회에서는 그 빈 슬롯을 건너뛴다.

호스트 `~Kernel`은 두 배열이 살아 있을 때 Clear를 수행한다. 첫 단일 순회 뒤 새 앞 슬롯 소유권이 남았으면 **호스트 수명 종료에 한해 후속 Clear 순회**를 수행한다. 이 후속 정리는 C++ 소유권 종료 계약이며 원본 Clear가 한 번 호출됐을 때 모두 제거되는 것으로 표현하지 않는다. 모든 소멸자에서 이미 제거된 앞 슬롯 조회는 null이다. 소멸자 콜백은 유한하게 신규 객체 생성을 끝내야 하며, 끝없이 새 객체를 만드는 콜백은 호스트 종료 계약 밖이다.

## 독립 실제 명령 관찰

`tools/ghidra/processretake-functions.json`의 **10.78 13개 / CD 14개** 함수를 현재 PC에서 읽기 전용으로 내보냈다. CD `Take`의 이전 항목 setter인 `004accc0`와 두 판본 전체 정리 함수를 추가했다. 결과는 `extracted/processretake/<판본>/{creation.c,functions.tsv}`이며 기존 process 내보내기의 실제 helper 범위와 함께 SHA를 기록한다.

`tools/decomp_processretake_oracle.py`는 실제 두 PE를 Unicorn 32비트 가상 메모리에 읽는다. BaseProcess·Kernel 등록/삭제·Take·실제 타입 생성자·부착/Pop/Unpop·공통 destroy/깊이/SID 반납은 원본 명령으로 실행한다. 명시 경계는 프로세스 부착/삭제 가상 통지와 가상 소멸자의 외부 효과이며, 가상 소멸자에서는 그 순간 Kernel 슬롯에 자기 포인터가 있는지를 직접 기록한다. Kernel 등록을 상수 반환 경계로 바꾸지 않는다.

판본마다 다음 **194개**, 두 PE **388개** 입력을 저장했다. 두 x87 정밀도 재실행과 준비용 Reset은 새 독립 관찰 수에 더하지 않는다.

| 입력 | 판본당 수 | 직접 관찰 범위 |
|---|---:|---|
| live 지정 재수신 | 72 | client/server/끝 SID, 상태 8/0x88, 다른 타입 인자, flags 0/0x50, 순환 0/187, OnAttached·모든 raw 바이트 무수정 |
| free 지정 재수신 | 108 | 상태 1/3/0x83, 세 실제 타입, 머리 없음/Form/자산, flags 0x10/0x50, 기존 PID·연결·payload·FIFO/카운터 |
| Kernel 같은 포인터 등록 | 6 | 기존 번호 1/7/63, 앞 빈 슬롯 유지·같은 반환 PID |
| Form 삭제 | 4 | 기존 번호 1/7, 통지 중 PID 9로 변경 유무·현재 PID 제거·소멸자 안의 슬롯 존재·Unpop·반납 |
| Kernel 전체 정리 | 4 | 슬롯 증가 순회·다른 소멸자에서 앞 슬롯 null·앞 신규 객체 잔류/뒤 신규 객체 같은 순회 삭제·미래 슬롯 중첩 제거 |

fixture는 `cpppj/tests/fixtures/processretake-x86.tsv`, 보고서는 `cpppj/recovery-processretake-evidence.json`이다. TSV에는 사건 순서, 전체 Form/부모/이웃 raw, 프로세스 SID 필드, 앞 64개 Kernel 슬롯, 타입 순환, FIFO/카운터, 삭제 깊이, **Kernel 등록의 반환 PID**를 저장한다. 생성자/삭제의 반환 열은 의미 없는 EAX를 비교하지 않도록 0으로 정규화한다. 실행은 정상 EIP/ESP, x87 스택/제어 워드, FS:[0] 복귀와 허용 범위 밖 쓰기/실행 부재를 검사한다. `--verify`는 PE/도구/내보내기/fixture SHA와 입력의 정확한 집합·개수를 감사한다.

`ProcessRetakeTests.cpp`는 같은 입력을 실제 C++ 풀·Kernel·Form host·공통 삭제에 적용하고 저장된 출력만 기대값으로 사용한다. live/free의 타입 순환 helper는 host 경로에서 대조한다. **독립 Kernel 등록 6개 행 및 Clear 중 신규 등록 2개 행에서는 슬롯/반환/순서를 대조하고 타입 순환 열은 제외한다.** C++ 타입 순환 전역은 아직 `SquidProcessState`에 있어 Kernel의 독립 등록 API에는 연결되지 않았다. 이 제외를 Kernel 전체 전역 동작의 동일성 증명으로 보지 않는다. 삭제된 C++ 객체의 SID 필드는 해제 메모리를 읽지 않고 직전 살아 있을 때 저장한 필드와 대조한다.

추가 검사 세 개는 잘못된 live 소유권/부모/PID·미등록 참조 거부와 10.78의 inactive 부모 앞 SID 기록, 호스트 종료의 앞 슬롯 null/앞 신규 객체 후속 정리를 확인한다. 이 세 검사는 저장 PE 대조 388개에 더하지 않는다.

## 검증 및 남은 범위

최종 통합 Release 빌드 `extracted/campaign-runtime-build-final.log`는 **종료 0·경고/오류 0**이다. 실제 PE **388개 입력과 같은 388개 C++ 출력 대조**, ProcessRetake 신규 **5개**, 기존 ProcessForm **4개**, `process_` 기존 프로세스/사제/캐리어/섬 연결 **14개**가 모두 통과했다. 최종 `--verify`도 388개 SHA·입력 감사를 통과했다. 이 작업의 새 독립 관찰/대조는 각각 **388개**이며, 초기 380개·x87 재실행·준비용 부착을 다시 더하지 않는다.

로그는 `extracted/processretake-focused-final.log`, `processretake-processform-regression-final.log`, `processretake-process-regression-final.log`, `processretake-oracle-final.log`다. 초기 병렬 라이브러리 빌드의 UI 반환 타입 오류는 `processretake-build-library.log`에 남겼으며 최종 성공으로 덮어쓰지 않았다. 빌드 종료 후 C++ CD 입력의 존재하지 않는 타입 순환 상태를 0으로 교정하고 root의 최종 빌드에서 새 검사 소스를 다시 컴파일했다. 전체 CTest는 root의 통합 검증으로 별도 기록하며 이 필터 결과를 전체 통과로 표현하지 않는다. 게임/GUI/캠페인 플레이는 실행하지 않았다.

최종 검증에 사용한 소스/입력의 SHA-256은 다음과 같다. 보호 PE의 SHA는 독립 근거 JSON에서 최종 감사했으며 변경되지 않았다.

| 파일 | SHA-256 |
|---|---|
| `cpppj/src/o/Kernel.h` | `89da82dfce333e16a87539251b9ac4c954b326e49ac7efd2f389263c5cdbc7dd` |
| `cpppj/src/o/Kernel.cpp` | `3fece4276cf1ffc2950910642964be63f9db271446d15068c6cedd0ca7240a04` |
| `cpppj/src/o/SquidProcess.h` | `3c650fb9f53e2e303559971f022b99191dd6b7eacdb4118e5236ce90762ec5f8` |
| `cpppj/src/o/SquidProcess.cpp` | `fe1ed93b3ae2f9e86d4d163500c0e1818ab77fef527500f59f92bd1a904273ac` |
| `cpppj/tests/ProcessRetakeTests.cpp` | `63cd3352a1071d125a3545427d3b7d23bb51dfae364b03c743264b1290eacad7` |
| `tools/decomp_processretake_oracle.py` | `ab2db09f7c639cf04afbefe466f2b78367908792ebaf7818aa7072a1277adddb` |
| `tools/ghidra/processretake-functions.json` | `f4e39badf96d0b64e04b12dae815db68315be442464bf82fb62eb6d942527863` |
| `cpppj/tests/fixtures/processretake-x86.tsv` | `705276d822488e0177fc743d09d394d5f7dcbb3e796a7a77238044b7b88a09c8` |

프로세스 직렬화/멀티플레이 전송, live 다른 객체 교체, 잘못된 부모/PID의 assert 이후 실행, 미등록 참조의 신규 소유권 취득, Kernel 전역 타입 순환의 독립 API 연결은 범위 밖이다. 지정 Form 0으로 같은 객체에 새 Form을 중복 생성하는 원본 경로는 이번 재수신 API에서 받지 않는다. 원본 소멸자 자체의 자원 정리는 명시 경계이며 필요한 실제 파생 프로세스 몸체를 별도로 연결해야 한다. 전체 Client/비표면 월드와 명령/UI 연결, 원본과 캠페인 1-1 완주 대조는 이어지는 목표다. DirectDraw 전체화면 오류 수정은 제외한다.
