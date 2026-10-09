# 사제 낙하 요청·공유 Regular·0x25b 처리 복원

후속 완료(2026-10-10): [Carrier 지면 조회·사제 가상 낙하](cpp-carrier-check-reconstruction.md)의 사제 +0xc8/J 재시작 생략을 `TryFall`로 복원하고 표면 삭제의 실제 finder→공유 Regular에 연결했다. 일반 사제 Pop·프레임/공간/소리 수명은 후속이다.

2026-10-10. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** AGENTS.md·두 인계와 cpppj 내부 계획/주석 규칙을 다시 읽고 현재 호스트 일치를 확인했다. 새 `priestfall` 목록 **15/15/15개**를 같은 PC에서 읽기 전용으로 내보냈다. 10.78 싱글플레이 복원이 현재 우선이며 outpost/LAN은 3차, 한국어·요구사항/MCP는 4차다. 게임·클론 창 실행과 보호 파일/dotnetpj 변경·커밋/푸시 없음.

[`RawPriestFall`](../../cpppj/src/o/RawPriestFall.h)은 [postPop 후반](cpp-priest-postpop-tail-reconstruction.md)에 남아 있던 낙하 요청과 이벤트 `0x25b`의 **분기·호출 순서·예약/종료 반환**을 복원한다. 실제 프로세스와 보호막 생성/조회는 기존 모듈을 재사용한다. 프레임/공간/오디오 하위 효과는 명시적인 외부 경계다.

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| 낙하 요청 | `004941f0` | `0040c880` |
| 사제 이벤트 분배의 0x25b | `00494580` | `0040ccd0` |
| 낙하 이벤트 전체 몸체 | `00493e60` | `0040c430` |
| SharedRegular 생성자 | `00496f00` | `0048f1d0` |
| 일반/공유 Regular RunFrame | `00496cb0` | `0048f0e0` |
| 프레임 설정 경계 | `004acee0` | `004acbc0` |
| 프레임 진행 경계 | `004afc90` | `004acce0` |
| 위치 소리 경계 | `004a9d70` | `00438c00` |

## 요청과 예약

`Begin(sid)`는 보호막 생성을 먼저 요청한다. 이어 `new(0x28)` 성공 때 **SharedRegular, 타입 61**을 부모에 부착한다. 이벤트는 `0x25b`, 초기 payload는 비트 `3c23d70a`인 `0.01f`, 생성 flags는 `0x50`이다. 기존 예약은 조회하지 않는다.

생성 효과 다음의 현재 권한이 참이면 `Unpop(0)`→`Repop(0x800)`을 요청한다. 확보 실패여도 이 전환과 마지막 소리는 실행한다. 마지막 `priestFall.wav` 요청은 **Repop 효과 다음의 현재 좌표**와 flags 0을 사용한다. 보호막/new 효과의 권한 변경과 Repop 뒤 좌표 변경을 독립 관찰로 대조했다.

[`SquidProcessHost::AddSharedRegular`](../../cpppj/src/o/SquidProcess.h)은 기존 `RegularProcess`를 타입 61로 부착한다. 일반/공유 타입은 동일한 RunFrame을 사용하므로 다른 타이머를 추가하지 않았다. 현재 시각은 부착 다음에 읽고 count는 0이다. `FindEvent`·`FindEventMasked`는 기존 타입 46/61 검색을 재사용한다. **SharedRegular의 네트워크 저장/복원 가상 함수는 이번 범위가 아니다.**

## 0x25b의 반복과 종료

1. 실제 [`RawPriestState::Immobile`](../../cpppj/src/o/RawPriestState.h)이 참이면 보호막을 요청한다.
2. 보호막 효과 다음의 현재 프레임을 조회한다. signed side − `'A'`가 9(J)이면 프레임 진행 `(1,0)`, 그 외에는 현재 타입 `+0x154`의 낙하 시작 프레임을 flags 0으로 설정한다.
3. 프레임 효과 다음의 현재 좌표를 `double(float 좌표) + double(0.9999f)`로 계산하고 0방향 절삭한다. 단순 절삭이나 중간 float 재저장은 사용하지 않는다. 지도 밖은 지면 없음이다.
4. 해당 지도의 **WORD가 0이 아니면** 현재 타입 `+0x140`의 도착 프레임을 설정한다. 지도 번호가 유효 SID인지 추가 검사하지 않는다. 그 다음 이동 불가를 다시 조회하여 거짓이면 보호막 삭제를 요청하고 `0.0f`를 반환한다.
5. 지면이 없으면 `0.1f`(비트 `3dcccccd`)를 반환한다.

count와 들어온 payload는 이 분기에서 읽지 않는다. 기존 Regular 실행기가 양수 반환을 다음 payload/시각으로 저장하고 count를 늘리며, 0 반환이면 실제 form을 삭제하여 Kernel에서 프로세스를 제거한다.

`PriestFallFrames`는 실제 타입의 두 프레임 번호를 연결하는 입력이다. 프레임 변경 전체 몸체와 SHP/공간 재등록을 대신하지 않는다. CD 프레임은 raw **+0x22 BYTE**, 패치 프레임은 **+0x24 DWORD**다.

`MakePriestFallHandler`는 실제 사제 vtable과 `0x25b`만 선택한다. 기존 회복 분배기의 fallback으로 합성할 수 있다. 다른 사제 이벤트는 명시 fallback이 없으면 미복원 예외이며 일반 Pop/Activate 지원을 확대하지 않았다.

`MakePriestFallHooks`는 같은 풀의 실제 SharedRegular·보호막 Ensure·일반 finder의 조회/삭제 요청을 연결한다. 가상 Unpop/Repop·프레임 설정/진행·위치 소리는 호출자가 공급한다. 실제 보호막 가상 Pop/destroy와 오디오 재생은 후속이다.

## 독립 원본과 C++ 검사

[`decomp_priestfall_oracle.py`](../../tools/decomp_priestfall_oracle.py)는 세 PE 각각 **456개**, 총 **1,368개** 입력을 `0x027f`/`0x037f` 두 x87 제어 워드로 실행한다. 판본마다 **912회 정상 반환**, 총 **2,736회 실행**이며 두 정밀도의 관찰이 같은 한 행을 저장한다.

요청·실제 SharedRegular 생성자·사제 분배/0x25b·현재 HP/이동 불가/방향·지면 WORD·실제 CRT는 원본 명령이다. `new`·BaseProcess 부착·보호막 Ensure/clear·Unpop/Repop·프레임 설정/진행·위치 소리·패치 보호막 finder/가상 삭제만 명시 대체한다. 함수 진입부터 정상 반환까지 실행하며 중간 지점에서 멈추지 않는다.

공유 생성자는 실제 메모리의 event/payload/count/시각을 검사한다. 부착 경계는 타입 61·부모 SID·formSid 0·flags 0x50을 검사한다. 전체 raw 슬롯/사건/반환 float 비트/최종 권한, ABI·보존 레지스터·SEH/x87 복구·실행/쓰기 허용 범위·assert/OS 0·SHA/입력 순서를 감사한다. 권한/확보 성공 실패, 강제 상태·HP·spot·signed side, 지도 WORD 0/1/65535, 효과 뒤 변경과 좌표 정밀도/경계를 포함한다. 기존 지면 상태 helper의 정상 좌표 계약을 유지하며 비유한 좌표/손상된 자산 전수는 범위 밖이다.

새 [C++ 검사](../../cpppj/tests/PriestFallTests.cpp)는 세 PE 관찰 재생과 **실제 factory/owner/보호막 조회→공유 form 타입 61 부착→Kernel 첫 실행/재예약→이른 시각 생략→지면 도착/보호막 삭제 요청→실제 form 삭제/Kernel 제거**를 두 raw 배치에서 검사한다. 실제 보호막 Pop/destroy와 프레임 효과 경계는 공간 등록/해제·현재 상태 변경 입력을 공급한다. 회복/낙하 분배 합성과 미복원 사건/잘못된 연결도 검사한다.

새 [fixture](../../cpppj/tests/fixtures/priestfall-x86.tsv)·[근거 JSON](../../cpppj/recovery-priestfall-evidence.json)은 UTF-8/LF다. 기존 fixture/감사 도구는 수정하지 않았다. 누적 인계 **354,106 + 1,368 = 355,474개**다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name priestfall
python -X utf8 tools/decomp_priestfall_oracle.py
python -X utf8 tools/decomp_priestfall_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

최종 Release **경고/오류 0**, CTest 내부 **446개·실패 0**(118.93초), 원본 근거 감사 **67종 모두 통과**. 최종 검사 로그는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 최신 단계에 기록한다. 다음은 **Carrier +0xcc의 실제 상태 검사·일반 사제 Pop·프레임/공간·보호막/소리 수명**, 이어 저장 맵/raw GUI·건설·경제·전투·승패다. 싱글플레이 완주와 플레이 가능한 게임은 아직 후속이다.
