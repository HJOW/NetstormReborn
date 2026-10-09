# 사제 보호막 생성·소리·로컬 안내 요청

2026-10-10. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** AGENTS.md·두 인계·cpppj 문서를 읽고 호스트 일치를 확인했다. KST 자정 전에 `priestshield` 15/13/13개를 같은 PC에서 읽기 전용으로 내보냈으며 구현·검증은 자정 뒤 이어졌다. 원본 게임/복사본·클론 창은 실행하지 않았다.

후속 완료(2026-10-10): [보호막 일반 Pop·Regular·공간 삭제 수명](cpp-forcefield-lifecycle-reconstruction.md)을 실제 공통 공간/장부·form/Kernel에 연결했다. 아래 보호막 Pop/destroy 미복원 표기는 이 후속 단계 전의 범위다. SfxProcess/실제 소리 수명·월드/GUI 연결은 계속 후속이다.

후속 완료(2026-10-10): [소리 프로세스·소리 이름 표](cpp-sound-process-reconstruction.md)를 복원해 보호막 생성 wrapper의 소리 조회/소리 프로세스 부착에 연결했다. 이 문서의 SfxProcess/소리 프로세스 미복원 표기는 그 후속 단계 전의 범위다. 소리 장치의 실제 재생/정지와 월드/GUI 연결은 계속 후속이다.

변경된 AGENTS.md에 따라 **10.78 싱글플레이 복원 → Windows 10/11 안정 구동 → TCP/IP 로컬 멀티플레이와 outpost → 요구사항/MCP** 순서를 따른다. 이미 복원한 outpost/지역 투표는 보존하고 outpost 전용 후속 연결은 3차 목표로 옮겼다. 이번 구현은 싱글플레이 사제의 이동 불가 상태에서 필요한 보호막 생성이다.

[`RawPriestShield`](../../cpppj/src/o/RawPriestShield.h)은 `00493d30`/CD `0040bfb0` **전체 wrapper**를 복원했다. [실제 보호막 조회](cpp-priest-forcefield-reconstruction.md)와 원본 자산 생성자/소유자 지정에 연결하고 보호막 가상 Pop·오디오·안내 창은 필수 외부 경계로 둔다. 전체 사제 postPop/GUI의 완료는 아니다.

| 몸체/자료 | 10.78 | CD·추가 10.37 |
|---|---|---|
| 보호막 생성 wrapper | `00493d30` | `0040bfb0` |
| 실제 좌표/finder 보호막 조회 | `004918e0` | `0040bf00` |
| 현재 보호막 타입 DWORD | `005412f4` | `0051cbe0` |
| 자산 생성 `(type,2)` | `004af530` | `004ab390` |
| 공통 가상 소유자 지정 | `004adf00`, `+0x74` | `004aefa0`, `+0x74` |
| 보호막 Pop `(x,y,0)` | 가상 `+0x90` | 가상 `+0x90` |
| 소리 프로세스 new / 생성자 | `004e4391(0x28)` / `004ab240` | `004f1650(0x28)` / `00453800` |
| 파일명 cdecl 소리 조회 | `004a8ee0` | `00437d20` |
| 전역 로컬 안내 소리 | `004a9cb0` | `00438b50` |
| 로컬 플레이어 / 로딩 깊이 | `00540c70` / `005c89b8` | `0050f6c8` / `005178d4` |
| 0과 비교하는 double 시각 | `005c85f0` | `00564db8` |
| 번역 / 안내 창 요청 | `004a89c0` / `004cf960` | 없음 |

## 구현한 순서

같은 위치/소유자의 보호막을 실제 일반 finder로 조회한다. 이미 있으면 생성·소리·로컬 안내를 모두 생략한다. 조회 뒤 **현재 타입 전역을 다시 읽어 flags 2로 로컬 자산을 생성**한다. 생성 이후의 현재 사제 x/y를 캡처하고 현재 소유자로 새 보호막을 지정한 뒤 캡처한 좌표로 flags 0 Pop을 요청한다. 소유자 지정 효과가 사제 좌표를 바꿔도 이미 캡처한 Pop 좌표는 유지한다. 생성 훅 이후 좌표의 NaN/부호 있는 0도 비트 그대로 전달한다.

Pop 뒤 소리 프로세스용 `new(0x28)` 성공 여부를 읽는다. 성공하면 `priestForceField.wav`를 조회하고 **새 보호막 SID·소리 번호·0·1**로 소리 프로세스를 요청한다. 실패하면 이 두 효과만 생략하며 로컬 안내 조건 검사는 계속한다. 사제 SID에 소리 프로세스를 붙이지 않는다.

소리 조회는 **파일명 하나의 cdecl 함수**다. wrapper가 미리 push한 0/1은 다음 소리 생성자의 인자이며 조회 함수가 정리하지 않는다. 디컴파일의 추정 인자를 그대로 적용한 초기 실행은 SfxProcess ABI 검사에서 실패했고, 실제 push/call/add esp/ret를 읽어 바로잡았다. 원본 출력값을 바꾸어 맞추지 않았다.

생성/Pop/소리 뒤 사제의 **현재 owner BYTE와 localPlayer DWORD**를 비교하고 로딩 깊이 0·double 시각 0이면 `ourPriestImmobile.wav`를 `(0,0,0,1,0)`으로 요청한다. localPlayer를 BYTE로 줄이지 않는다. +0과 -0은 허용하고 NaN/0 아닌 시각은 제외한다. 패치는 뒤이어 `PriestImmobile` key를 번역하고 반환 문구로 안내 창을 요청하며 CD/10.37은 안내 소리만 요청한다. 기존 보호막 경로에는 이 안내도 없다.

[`MakePriestShieldCreationHooks`](../../cpppj/src/o/RawPriestShield.cpp)는 같은 풀의 `SquidFactory::Create`와 `SquidOwner::Set`을 연결한다. 실제 타입 167의 원본 생성자/공통 초기화·client 할당·소유자 BYTE 변경을 통합 검사에 사용한다. factory/owner/lookup이 다른 풀을 참조하거나 필수 경계가 빠지면 생성 전에 거부한다. 생성이 0/free/contained 등 잘못된 SID를 반환하면 이후 가상 호출을 거부한다. 외부 생성/Pop 이후 모든 실패를 되돌리는 트랜잭션은 제공하지 않는다.

## 독립 원본 관찰과 C++ 연결

[`decomp_priestshield_oracle.py`](../../tools/decomp_priestshield_oracle.py)는 세 PE 각각 **832개**, 총 **2,496개**를 두 x87 제어 워드 `0x027f`/`0x037f`로 실행했다. 전체 wrapper·실제 lookup/finder/좌표/CRT·공통 소유자 지정은 **정상 ret까지 실제 명령**이다. 자산 생성·보호막 가상 Pop·소리 할당/조회/프로세스·전역 소리·번역/창 몸체만 명시 대체한다. 실제 factory 생성자 연결은 별도 C++ 검사다.

768개 분기 격자는 등록 장면 8종·소유자 3종·소리 확보 성공/실패·시각 4종·로딩/로컬 조건을 교차한다. 같은/다른 소유자·다른 타입·매장·발자국/해시 단계·기존 보호막·현재 타입 167/168을 포함한다. 추가 64개는 생성 중 좌표/소유자 변경, 소유자 지정 뒤 좌표 변경, Pop 뒤 로컬 조건 변경, 소리 조회 뒤 NaN 시각 입력을 제공한다. Python/C++에서 존재/생성/안내 기대 판단을 계산하지 않는다.

각 PE wrapper/실제 lookup/정상 반환 **1,664회**, 실제 소유자 지정 **1,280회**, 생성/Pop/소리 확보 대체 각각 **1,280회**, 소리 조회/프로세스 각각 **640회**, 로컬 안내 소리 **224회**다. 패치 번역/창은 각각 **224회**, CD/10.37은 0회다. ESP·EBX/ESI/EDI/EBP·x87 제어/TOP·FS:[0] 정상 SEH 복구·허용 코드/쓰기·해시 불변·원본 this/인자·raw 전체/전역·assert/OS 0·SHA/입력/행·실제 진입/대체 수를 확인한다. C++ 예외에서 원본 SEH의 예외 해제 몸체를 재현한 것은 아니다.

새 [C++ 검사](../../cpppj/tests/PriestShieldTests.cpp) 5개는 세 PE 전체 관찰 재생과 실제 factory/owner→보호막 조회→중복 생성 방지→사제 삭제 준비의 보호막 조회/삭제 요청→재생성, 소리 확보 실패/성공, 잘못된 경계/풀/생성 SID를 검사한다. 통합의 Pop/가상 destroy 경계가 해시 등록/해제 입력을 공급하며 **전체 보호막 Pop/destroy의 원본 몸체는 미복원**이다. [fixture](../../cpppj/tests/fixtures/priestshield-x86.tsv), [근거 JSON](../../cpppj/recovery-priestshield-evidence.json)은 UTF-8/LF다. 앞 인계 345,466개에 이번 2,496개를 더한 누적 인계 집계는 **347,962개**다.

Release **경고/오류 0**, CTest 내부 **435개·실패 0**(118.77초), 원본 근거 감사 **65종 모두 통과**. 검사용 생성 흐름과 실제 GUI/오디오의 완료를 구별한다.

## 재현과 후속

```powershell
tools/ghidra/export_functions.ps1 -Name priestshield
python -X utf8 tools/decomp_priestshield_oracle.py
python -X utf8 tools/decomp_priestshield_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

다음은 **사제 postPop 후반의 Carrier/이동 불가·지면·낙하·보호막 생성/삭제 결합과 일반 사제 Pop**, 보호막 자체의 Pop/공간 수명·소리 프로세스, 실제 raw 월드/건설·경제·전투·승패 연결이다. outpost 전용 수명/멀티플레이는 3차 목표로 인계한다. 최종 빌드/전체 검사/감사 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다.
