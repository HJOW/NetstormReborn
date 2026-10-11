# 건설 프로세스 시작·진행·완료 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS를 읽고 [건설용 객체 생성·이동](cpp-construction-builder-reconstruction.md)의 다음 항목인 건설 시작/생성자·진행·완료를 진행했다. 시작 트리는 깨끗했고 현재/마지막 디컴파일 PC가 같았다. `constructionprocess` **12/14/14개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본 게임·analyzeManager·클론 창 실행은 없었다.

## 주소와 복원 범위

| 흐름/자료 | 10.78 | CD/10.37 |
|---|---|---|
| 건설 시작(new 0x38 → 생성자) | `00443fe0` | `004d06a0` |
| 생성자 | `00443c40` | `004d0720`(기본 시간 helper `004d08a0`) |
| 진행(프로세스 vtable +0x18) | `004428c0` | `004d0ba0` |
| 완료 | `00443e20` | `004d09e0` |
| form 삭제 통지(vtable +0xc) | `00444030` | `004d08e0` |
| 프로세스 가상 표(객체 +0x28) | `00506ffc` | `00506b88` |
| 건설 표시 효과(지속 0 / 그 밖) | `00423840` / `00423900` | `004eab60` / `004eac30` |
| 부착 / Kill | `0041bd20` / `0041bf10` | `0048f590` / `0048f7b0` |
| 소리 조회 / 소리 프로세스 생성 | `004a8ee0` / `004ab240` | `00437d20` / `00453800` |
| 최대 HP / HP 쓰기 / 부모 조회 | `004adcb0` / `004adca0` / `0041bfb0` | `004aed30` / `004aed20` / `0048f870` |
| contained 삭제 / 갱신 요청 | `004b1a90` / `004da650` | `004eb310` / `0041eac0` |
| 위치 소리 / 전역 소리 / 밀어내기 | `004a9d70` / `004a9550` / `00443800` | `00438c00` / `004383f0` / `004d2450` |
| 프로세스 타입 / 소리 프로세스 타입 전역 | `005410e8`(30) / `0054112c`(47) | `0051c9d8` / `0051ca1c` |
| 로컬 플레이어 / 권한 / 밀어내기 genus | `00540c70` / `00540bc4` / `005325b4` | `0050f6c8` / `00540a2c` / `0053fc28` |
| dais 타입 / quickbuild / quickvortex | `005412ec`(165) / `0055a49c` / `0055a498` | `0051cbd8` / `00537eb4` / `00537eb0` |

[`RawConstructionProcess`](../../cpppj/src/o/RawConstructionProcess.h)가 다섯 몸체를 복원한다. 원본 객체는 0x38바이트이며 +0에 전송용 상태(가상 표, +8 변경 비트, +0x10 시작 시각, +0x18 끝 시각, +0x20 추가 바이트), +0x28에 프로세스(가상 표, form SID, 부모 SID), +0x34에 진행음 항목이 있다. C++은 건설 흐름이 읽고 쓰는 +8/+0x10/+0x18/+0x34만 `ConstructionProcessFields`로 옮겼다. 프로세스 타입 전역의 초기값은 두 판본 모두 **30**이다.

`quickBuild`·`quickVortex`는 콘솔 명령 `quickbuild`(`qb`)·`quickvortex`(`qv`)가 쓰는 전역이다(패치 `Netstorm.c`의 명령 분기에서 확인).

## 원본 순서

**시작/생성자.** new(0x38)가 실패하면 아무 효과도 없다. 성공하면

1. 시작/끝 0.0, 변경 비트 0xff, form/부모 0으로 초기화한다.
2. 지속 시간이 0(또는 -0)이면 `00423840`, 아니면 `00423900`으로 건설 표시 효과를 만든다(인자는 부모 SID).
3. **효과 뒤의 현재 게임 시각**을 시작 시각으로 쓴다. 지속 시간이 0이면 10초로 바꾼다. 끝 = 지속 시간 + 현재 시각이다.
4. 현재 프로세스 타입 전역으로 부모에 부착한다(form SID 0, flags `0x10`).
5. 지속 시간이 1초보다 길면 `jimBuild.wav`를 조회해 +0x34에 쓰고, new(0x28)가 성공하면 **그 뒤의 현재 +0x34**로 소리 프로세스를 만든다(대체 소리 0, flags 1 = 부모 위치 반복 재생). new가 실패해도 소리 항목은 남는다. 1초 이하면 +0x34는 0이다.
6. 화면 갱신을 요청한다.

**진행.** 최대 HP를 조회하고 그 뒤의 현재 시각으로 `절삭((현재-시작)/(끝-시작)×최대)`를 계산한다. `quickBuild`이거나 `quickVortex`이고 부모 genus에 `0x200`이 있으면 최대로 바꾼다. `min(값, 최대)`가 음수면 0, 아니면 그 값으로 제한해 부모 HP에 쓰고 가상 표시(+0x88)를 부른다. 그 뒤의 abstract 비트가 꺼져 있으면 Kill(0), 값이 최대와 같고 권한이 있으면 Kill(0)한다.

**완료.** 최대 HP → 현재 HP 순서로 읽는다. 부모에서 현재 소리 프로세스 타입의 contained를 지우고(인자 0, 1) 갱신을 요청한다. 부모 소유자 BYTE가 로컬 플레이어 **DWORD**와 같고 번호가 예측 영역(5 이상, 서버 영역 시작 미만) 밖이면 부모 좌표에서 `buildDone.wav`를 내고, 타입 +0x90의 소리 항목이 0이 아니면 전역으로 낸다. 권한이 있고 예측 번호가 아니며 HP가 최대와 같고 abstract이면 Unpop(0) → abstract 비트만 해제 → 재등록(flags 9) → 현재 타입 genus가 현재 밀어내기 마스크와 겹치면 유닛 밀어내기 순서다.

**form 삭제 통지.** 완료를 실행한 뒤, 부모 상태의 하위 두 비트(free/dead)가 0이고 타입이 dais가 아니면 최대 HP를 다시 조회해 HP에 쓴다. flags 인자는 읽지 않는다.

## 판본 차이

| 항목 | 10.78 | CD/10.37 |
|---|---|---|
| 진행음 조건 | 실수 비교(비교 불가는 소리 없음) | 지속 시간 **비트의 signed 비교**(양의 NaN은 소리 있음) |
| 지속 시간 0 판정 | 실수 비교 | `비트 & 0x7fffffff`(결과 같음) |
| HP 폭 | DWORD 읽기/쓰기 | WORD 쓰기, **부호 확장한 WORD** 읽기 |
| 예측/서버 경계 | 15000 | 6000 |
| 밀어내기 인자 | x, y, 타입, 소유자 | x, y, 타입 |
| 진행의 debug assert | `005e4794`가 켜져 있으면 form 유효성 보고 | 없음 |

## 연결과 호스트 계약

`RawConstructionProcess`에 `SquidProcessHost`를 주면 `Start`가 실제 `ConstructionProcess`를 만들어 Kernel에 붙인다. host 없이 `attach` 경계만 주면 호출자가 필드를 보관한다(원본 관찰 재생에 쓴다). `MakeConstructionProcessHooks`는 최대 HP를 실제 `SquidReward`로, 밀어내기를 실제 `RawConstructionEffects::Displace`로 잇는다. `MakeConstructionProcessNoticeHooks`는 [건설 배치 통지](cpp-construction-notice-reconstruction.md)의 건설 시작 경계에 `Start`를 연결한다(통지는 지속 시간 비트 0을 넘긴다). 클라이언트의 [`MakeConstructionProcessSoundHooks`](../../cpppj/src/client/SoundProcess.h)는 진행음 조회/소리 프로세스 부착과 완료음·준비 소리 재생을 실제 이름 표·소리 프로세스·재생 계층으로 잇는다.

원본과 다르게 둔 것:

- **둘째 Kill.** 원본 진행은 첫 Kill이 프로세스 객체를 해제한 뒤에도 같은 포인터로 둘째 Kill을 부를 수 있다(해제 뒤 읽기). C++은 첫 Kill 뒤 프로세스가 Kernel에 남아 있을 때만 둘째 Kill을 한다. `SquidProcessHost::IsRegistered`와 `SquidProcess::Pid`를 이 확인을 위해 더했다.
- **부착 실패.** 패치판은 부모가 free/dead이면 붙이지 않고 객체를 해제하지도 않은 채 남은 쓰기를 계속한다. C++은 붙지 않은 프로세스를 버리고 남은 효과(진행음·갱신)만 실행한다. CD판의 그 경로는 `SquidProcessHost::Attach`가 지원하지 않는다.
- **assert.** 완료의 abstract 해제는 void 상태가 아니면 예외로 거부한다. 패치판 진행의 debug assert 구간과 부모 조회의 종속 체인 assert는 옮기지 않았다.
- **x87.** 진행 비율은 double 연산으로 계산한다. 모든 입력에서 x87 확장 정밀도와 같다고 증명한 것은 아니다.

전송용 상태의 쓰기/읽기(`004426d0`/`00442760`), 메시지 수신(`004427b0`), 끝 시각 재설정(`00443dc0`/CD `004d0930`·`004d0970`)은 이번 범위가 아니다. 건설 표시 효과·contained 삭제(`004b1a90`)·가상 표시/Unpop/재등록·타입 준비 소리 항목은 경계로 남았다.

[`ConstructionProcessTests.cpp`](../../cpppj/tests/ConstructionProcessTests.cpp)의 새 검사 **12개**는 세 PE 관찰 재생, 경계 누락/다른 풀 거부, 두 확보 실패, 지속 시간별 기본 시간/진행음과 판본 차이, void 요구, 통지 어댑터를 확인한다. 실제 host·Kernel·공통 삭제·보상 모듈 위에서 시작 → 프레임별 HP 증가 → Kill → form 삭제 통지의 완료까지 두 판본으로 실행하고, 둘째 Kill 생략·권한 없는 쪽의 유지·quickbuild·dead 부모도 검사한다. [`SoundPlayTests.cpp`](../../cpppj/tests/SoundPlayTests.cpp)의 새 검사 **1개**는 실제 이름 표/소리 프로세스/재생 계층에서 진행음 반복 시작 → 완성 때 정지 → 완료음 → 준비 소리의 장치 호출 순서를 확인한다. 이 검사의 contained 삭제는 검사 안의 경계 구현이며, 전체 GUI에서 건설한 관찰이 아니다.

## 독립 관찰과 검증

[`decomp_constructionprocess_oracle.py`](../../tools/decomp_constructionprocess_oracle.py)는 세 실제 PE의 시작(생성자 포함)·진행·완료·form 삭제 통지 전체와 부모 조회·HP 쓰기·genus/타입 조회·abstract 지정·CRT 정수 변환·CD 기본 시간 helper를 제한 x86으로 정상 반환까지 실행한다. operator_new·건설 효과·부착·소리 조회/소리 프로세스·갱신·최대 HP·가상 표시/Unpop/재등록·Kill·contained 삭제·완료 소리·밀어내기의 **몸체는 실행하지 않고 실제 인자만 관찰**한다.

지속 시간 0/-0/1.0 경계/음수/무한대/양·음 NaN, 두 할당 실패, 진행 비율(0·중간·최대 초과·음수·0으로 나누기)과 최대 HP의 부호/크기, 두 즉시 완성 전역과 genus, abstract 바이트의 다른 비트, 예측/서버 번호 경계, 로컬 플레이어 DWORD, HP 폭/부호, 밀어내기 마스크, dais 타입과 부모 상태를 교차했다. 경계 뒤에 현재 시각·프로세스 타입·소리 필드·HP·소유자·로컬 플레이어·권한·좌표·abstract·준비 소리가 바뀌는 입력도 포함한다. 각 PE **956개**(시작 146·진행 456·완료 280·form 삭제 74), 새 저장 **2,868개**, 두 x87 정밀도의 최상위 정상 반환 **5,736회**, 누적 저장 **483,330개**(480,462 + 2,868)다. form 삭제 통지 안의 완료 진입은 최상위 반환 수와 구분한다.

[fixture](../../cpppj/tests/fixtures/constructionprocess-x86.tsv)와 [근거 JSON](../../cpppj/recovery-constructionprocess-evidence.json)의 SHA·입력/출력 폭·진입/반환/경계 수를 감사한다. 두 x87 제어값 639/895에서 관찰이 같았고 SEH 머리·스택·보존 레지스터·x87 균형·허용 실행/쓰기를 확인했다. assert·게임·OS 호출은 0이다. 진행 시각은 2의 거듭제곱 분모의 합성 값이며 두 정밀도에서 절삭 결과가 갈리는 입력은 포함하지 않았다. 패치 debug 전역과 CD의 Pentium FDIV 우회 전역은 0으로 공급했다.

Release 경고/오류 **0**·CTest 내부 **635개·실패 0**(157.98초, 전체 158.07초), 근거 감사 **98종 모두 통과**. 새 변이 16개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과).

전체 CTest는 이 단계가 커밋된 `f468820`(1011 - 09)과 같은 내용의 저장소 밖 사본(`extracted/mutation-work`)을 다시 빌드해 실행했다. 그 시점의 작업 트리에는 다른 세션이 진행 중인 다음 단계 변경(끝 시각 재설정)이 있었고, 위 빌드·검사·감사·변이 결과는 그 변경을 포함하지 않는다.

## 다음 순서와 재실행

2026-10-11 후속: [건설 끝 시각 재설정](cpp-construction-time-reconstruction.md)과 실제 form→Kernel 연결을 완료했다. 현재 전체 641개·근거 감사 99종·새/기존 변이 22개 검출이다. 다음은 전송용 상태와 표시/contained/공간 경계의 실제 연결이다. 아래 순서는 이 절 작성 당시 기록이다.

다음은 **건설 프로세스의 남은 몸체**(끝 시각 재설정 `00443dc0`/CD `004d0930`·`004d0970`, 전송용 상태 쓰기/읽기/수신 `004426d0`·`00442760`·`004427b0`)와 **경계의 실제 연결**(건설 표시 효과 `00423840`/`00423900`, contained 삭제 `004b1a90`, 타입 준비 소리 항목)이다. 건설용 객체 생성·이동의 하위 경로/접근점·자원 선택/정렬·가상 Pop, 소유 사제의 실제 이동/contained, 타입별 공간/파생 삭제/추락/알림, 메시지 전파/수신·법적 위치/미리보기·건설 GUI도 남는다. **게임에서 건물을 지을 수는 아직 없다.**

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructionprocess
python -X utf8 tools/decomp_constructionprocess_oracle.py --verify
cmake --build cpppj/build --config Release
cpppj/build/bin/Release/netstorm_tests.exe --filter ConstructionProcess_
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_mutation_check.py --focused --only process-cd-hp-sign
```

로그는 Git 제외 `extracted/constructionprocess-*.log`다. 변이는 저장소 밖 사본에서만 실행한다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시는 없다.
