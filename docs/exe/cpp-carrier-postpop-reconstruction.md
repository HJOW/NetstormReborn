# Carrier·Damageable postPop의 직접 호출 복원

2026-10-08 후속(`HJOW-Athlon`): [사제 HP/지면 상태 조회와 HP setter](cpp-priest-state-reconstruction.md)를 복원했다. 절반 HP 전환에서 실제 가상 공간 효과는 여전히 외부 훅이며, 전체 사제 Pop/회복 이벤트/보호막/낙하/GUI는 후속이다. 최신 디컴파일 수행 호스트는 [내보내기 문서](ghidra-exports.md)를 따른다.

2026-10-08, 기준 판본 **10.78**. 마지막 디컴파일 수행 PC: **VM-W11-CODEX**(현재 호스트 `VM-W11-Codex`, IP 10.0.0.17). 최신 AGENTS.md와 두 인계 문서를 읽고 사용자가 지정한 **30분 이내에 구현·검증할 범위**로 진행했다. 같은 PC에서 `carrierpostpop` 목록을 Ghidra 읽기 전용으로 내보냈다(세 판본 각각 5개). 원본 게임/복사본·클론 창 실행·보호 파일 변경·AGENTS.md/dotnetpj 수정·커밋/푸시 없음.

[`RawCarrierPostPop`](../../cpppj/src/o/RawCarrierPostPop.h)은 **Carrier → Damageable → 공통 Squid postPop**의 직접 호출 흐름을 복원한다. 이전 [사제 목록·회복 예약 prefix](cpp-priest-postpop-reconstruction.md)와 실제 공통 장부를 합성했다. 사제 전체 가상 postPop 및 비표면 GUI 월드의 완성은 후속이다.

| 동작 | 10.78 | CD / 추가 10.37 |
| --- | --- | --- |
| Carrier postPop 전체 몸체 | `00427720` | `004e6360` |
| Damageable postPop 전체 몸체 | `0044bf80` | `004621c0` |
| 최초 flag의 순수 genus 조회 | `004ad080` | `004acdb0` |
| 타입 flags1 조회 | `004ac1e0` | `004abac0` |
| 외부 지면 소유자 갱신 | `0044bd20` | `00461f00` |
| 실제 공통 postPop | `004b0d30` | `004ae180` |
| 사제 가상 표의 +0xcc 대상 | `00426fc0` | `004e50d0` |

## 동작과 연결 경계

Carrier는 `flags & 1`이 없고 boss 권한이 0일 때만 **가상 +0xcc**를 부른다. extra의 abstract/buried 차단 여부와 무관하며 반환값은 버린다. 이후 항상 Damageable을 같은 flags로 부른다. Damageable의 직접 호출 API는 이 carrier 가상 확인을 거치지 않는다.

Damageable은 `flags & 1`이면 genus `& 0x130000`을 조회하지만 결과를 사용하지 않는다. 이 함수는 순수 타입 읽기이며 HP를 변경하지 않는다. C++은 이 무효과 조회를 별도 효과로 취급하지 않는다.

지면 소유자 갱신은 아래 조건이 모두 참일 때만 요청한다. 이 조건은 최초 flags와 무관하다.

- 요새 로딩 중첩 횟수가 0이 아니다.
- 현재 타입이 geyser 타입(기본 **122**)이 아니다.
- extra(+40/CD +35)의 `& 9`가 0이다.
- 현재 타입 flags1에 **`0x400`**이 있다.

지면 효과에 **float 좌표의 원래 비트와 현재 소유자 BYTE**를 전달하고 이후 항상 공통 postPop을 부른다. `carrierCheck`가 타입·extra·좌표·소유자를 바꾸면 Damageable은 그 뒤의 raw 필드로 조건을 평가한다. 지면 효과가 바꾼 필드도 공통 호출 시 다시 읽는다.

가상 +0xcc 몸체와 지면 소유자 갱신의 실제 공간 순회는 이번 구현에서 명시적인 외부 훅이다. 원본 지면 함수는 주변 표면/섬 받침/종유석의 소유자·표시를 바꿀 수 있으므로 호출 기록을 전체 공간 효과 구현으로 간주하지 않는다. 실제 사제 flags1 **`0x00069012`**에는 `0x400`이 없어 이번 사제 통합에서 지면 갱신은 요청되지 않는다. 조건 대조는 해당 비트를 추가한 합성 입력으로도 실행한다.

## 공통 몸체 재사용과 보호

기존 [`SquidPostPop`](../../cpppj/src/o/SquidPostPop.h)에 `ValidateBase`/`PostPopBase`를 추가했다. 이는 원본처럼 파생 몸체가 **공통 함수 주소를 직접 호출**할 경로다. raw 자산·Graph·비용·목록·AI/배치의 미복원 보호 검사는 유지하고, 해당 파생 클래스의 *전체 가상 흐름* 지원 여부 검사만 제외한다.

기존 `Validate`/`PostPop`/`Activate`와 `SquidPop`의 자동 분배는 기존 지원 검사를 유지한다. 사제는 여전히 일반 가상 Pop/Activate에서 거부한다. 직접 공통 호출이 사제 전체 lifecycle 지원을 뜻하지 않는다.

`MakeCarrierPostPopHooks`는 같은 풀인지 확인하고 실제 공통 사전 검사와 장부 몸체를 연결한다. Carrier는 외부 효과 전에 raw/공통 상태를 검사하며, 잘못된 SID·free/contained·타입·누락된 경계를 거부한다. 공통 postPop은 비용/통계를 갱신하고 깊이를 **한 번만** 감소시킨다. 공통 장부 억제 상태에서도 앞 단계 Damageable의 지면 요청은 원본 순서대로 실행되며, 공통 몸체는 깊이만 줄인다.

## 독립 원본 대조와 통합

[`decomp_carrierpostpop_oracle.py`](../../tools/decomp_carrierpostpop_oracle.py)는 세 실제 PE에서 Carrier/Damageable의 **전체 몸체와 타입 getter**를 실행한다. **가상 +0xcc·지면 소유자 갱신·공통 Squid postPop만** 인자/호출 시점 기록으로 대체한다. 원본 전체 몸체의 정상 반환과 thiscall의 스택 복구를 확인한다. 원본 게임·OS는 실행하지 않는다. 허용 함수 밖 명령이나 스택 밖 원본 명령 쓰기는 거부하며, 외부 raw 변경은 대체 경계에서 입력 효과로 주입한다.

판본마다 **2,448개**, 총 **7,344개**다. Carrier 1,968개(조건 격자 1,920 + 필드 변경 48), 직접 Damageable 480개다. 최초/비최초 flags·boss·로딩·geyser 제외·extra·flags1·상태/소유자·좌표 비트·외부 효과 뒤 재조회를 교차한다. 좌표에는 음의 0과 quiet NaN 비트도 포함하며 이 몸체는 좌표를 계산하거나 정수로 변환하지 않는다. 실제 공간 지면 함수가 비유한 좌표를 처리할 수 있다는 의미는 아니다.

[`carrierpostpop-x86.tsv`](../../cpppj/tests/fixtures/carrierpostpop-x86.tsv)의 사건 순서/좌표 비트/인자와 raw 슬롯 전체가 C++과 일치해야 한다. [`recovery-carrierpostpop-evidence.json`](../../cpppj/recovery-carrierpostpop-evidence.json)은 PE/도구/내보내기/입력 목록/fixture SHA, 실제/대체 진입 수, 정상 반환 및 assert/OS 호출 0을 기록한다. 일반 CTest는 원본 파일/Python/Ghidra 없이 저장 fixture만 읽는다.

[`CarrierPostPopTests.cpp`](../../cpppj/tests/CarrierPostPopTests.cpp)는 독립 재생 외에 다음을 검사한다.

- 실제 공통 비용/로컬 두 통계/전역 통계/깊이, AI 미복원 조건의 효과 전 거부, 억제 상태의 깊이 감소, 서로 다른 풀 연결 거부.
- 두 판본 server/client 사제 생성자 → 실제 소유자 지정 → 회복 prefix/ProcessForm·Kernel → Carrier/Damageable → 실제 공통 장부의 순차 합성.
- 비최초 비권한 확인 뒤 기존 회복 유지, 명시적 form 삭제, 일반 Pop/Activate의 사제 거부 유지.

회복 이벤트 처리기·가상 +0xcc·지면 공간 순회는 이번 C++ 통합에서도 전체 원본 몸체를 구현한 것이 아니다. GUI 창이나 사제 낙하/보호막을 실행하지 않았다.

Release **경고/오류 0**, CTest 내부 **284개·실패 0**(기존 278 + 새 6, 58.19초). 관련 감사 **6종 통과**(carrierpostpop/priestpostpop/priestowner/postpop/process/owner). 새 7,344개를 더해 누적 독립 x86 **159,526개**다. 로그는 `extracted/carrierpostpop-export.log`, `carrierpostpop-generate.log`, `build-carrierpostpop.log`, `ctest-carrierpostpop.log`, `audit-carrierpostpop.json`에 있다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name carrierpostpop
python -X utf8 tools/decomp_carrierpostpop_oracle.py
python -X utf8 tools/decomp_carrierpostpop_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

## 후속 복원과 분석 정정

다음 작은 범위는 사제의 HP/지면에 따른 이동 불가 상태 조회 **`00427030`→`00492090` / CD `004e5150`→`0040d7b0`** 또는 이벤트 처리기 **`00494580`/CD `0040ccd0`**의 회복 `0x25a` 분기다. 이벤트 주소는 실제 사제 가상 표의 +0x5c에서 확인했다. 이 단계에서는 현재 HP의 패치 DWORD/CD signed WORD 차이와 1/2 최대 HP 경계, 지면 spot/extra 비트 및 반환값을 대조해야 한다.

`00493d30`/CD `0040bfb0`은 **경로 무효화 함수가 아니라 보호막 생성 및 사운드 처리**다. 같은 소유자의 보호막을 찾는 `004918e0`/CD `0040bf00`이 없을 때 타입 `DAT_005412f4`/CD `DAT_0051cbe0`을 생성하고, Owner/Pop 및 `priestForceField.wav`/`ourPriestImmobile.wav` 처리를 한다. 이 판단은 현재 PC의 정밀 디컴파일 정적 분석이며 해당 몸체의 독립 실행 대조/구현은 아직 하지 않았다.

낙하 `004941f0`/CD `0040c880`은 보호막 처리 뒤 SharedRegular 이벤트 **`0x25b`**, payload **`0x3c23d70a`**를 요청하고 권한 상태에서 Unpop(0)→Repop(0x800), `priestFall.wav`로 이어진다. 반대 분기 `00491980`/CD `0040c0d0`는 보호막 SID를 찾아 가상 삭제한다. 전체 postPop·preDestroy를 완성한 뒤 일반 Pop/비표면 raw GUI를 연결한다. PathProcess 진행/종료는 별도 후속이다.

지면 소유자 공간 순회·보호막/사운드·SharedRegular·HP 회복·preDestroy·보행 진행/종료·GUI 전환, 기존 약 40분 변이 검사와 최대 지도/SID 소진 검사는 인계한다. 이번 사용자 지시의 30분 범위는 앞 단계 인계의 15분 기준보다 우선하며, 장시간 검사 완료로 처리하지 않는다.
