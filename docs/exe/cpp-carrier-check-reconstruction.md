# Carrier 지면 조회·사제 가상 낙하 연결 복원

2026-10-10. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** AGENTS.md·두 인계·cpppj 내부 계획/주석 규칙을 다시 읽었다. 현재 PC와 마지막 디컴파일 PC가 같으며 새 `carriercheck` 목록 **9/10/10개**를 같은 PC에서 읽기 전용으로 내보냈다. 10.78 싱글플레이가 우선이고 outpost/LAN은 3차, 한국어·요구사항/MCP는 4차다. 원본 게임/복사본·클론 창 실행, 보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.

[`RawCarrierCheck`](../../cpppj/src/o/RawCarrierCheck.h)는 [Carrier postPop](cpp-carrier-postpop-reconstruction.md)에 남아 있던 가상 `+0xcc`를 복원한다. [`RawPriestFall::TryFall`](../../cpppj/src/o/RawPriestFall.h)은 실제 사제 가상 `+0xc8`를 [기존 낙하 요청/SharedRegular](cpp-priest-fall-reconstruction.md)에 연결한다. 일반 사제 Pop 지원은 확대하지 않았다.

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| Carrier 가상 +0xcc | `00426fc0` | `004e50d0` |
| 사제 가상 +0xc8 | `00494e80` | `0040d770` |
| signed 프레임 방향 | `004ad710` | `004ae4b0` |
| CD의 방향 J bool helper | 별도 함수 없음 | `0040c5b0` |
| 낙하 요청 | `004941f0` | `0040c880` |
| Carrier spot 보정 상수 | `00500edc` | `00506d78` |

## Carrier 조회의 정확한 역할

`Check(sid)`는 다음 순서로 실행한다.

1. **현재 타입 genus에 `0x320000` 중 하나라도 있으면 거짓**을 반환한다. 좌표·spot·프레임·가상 낙하를 읽지 않는다.
2. 현재 x/y에 `0.9999f`를 더하고 0방향 절삭한 칸의 spot을 읽는다. `spot & 6`이면 거짓을 반환한다.
3. 그 외에는 가상 `+0xc8`을 호출하고 그 반환값이 0이 아닌지를 반환한다. 가상 낙하는 실제 효과를 일으킬 수 있다.

사제의 실제 genus는 `0x210000`이므로 **사제 +0xcc는 첫 조건에서 거짓**이다. 사제 HP/이동 불가 조회나 낙하 요청을 대신하는 함수가 아니다. 비최초·비권한 Carrier postPop과 생성의 Carrier genus 래퍼가 이 반환값을 버리는 순서도 보존했다. 이 경계가 임시 callback일 때 허용했던 임의 상태 변경을 실제 사제 +0xcc의 효과로 해석하지 않는다.

두 판본 보정 상수는 주소가 다르지만 비트 **`3f7ff972`**로 같다. x87의 float 입력 합은 `double(float) + double(0.9999f)`로 유지하고 float 중간 저장을 하지 않는다. 원본은 지면 읽기에 별도 지도 범위 검사를 하지 않으므로 C++은 정상 지도 안 좌표를 받으며 범위 밖/비유한 읽기를 예외로 진단한다. 타입 제외 때문에 읽지 않는 잘못된 좌표는 허용한다.

원본 +0xcc는 raw state/extra/HP를 독립 생략 조건으로 검사하지 않는다. 타입과 spot 표는 복사하지 않고 참조하여 매 호출 현재 값을 읽는다. 실제 C++ 연결의 자산 상태/장부 검사는 기존 postPop/생성 호출자가 유지한다.

`MakeCarrierCheckPostPopHooks`와 `MakeCarrierCheckSpawnHooks`는 같은 풀인지 확인하고 +0xcc 경계만 바꾼다. 지면 소유·공통 장부·생성/소유자/Pop 등 다른 경계의 계약은 그대로다.

## 사제 +0xc8와 표면 삭제

`TryFall(sid)`는 현재 프레임의 **signed side − 'A'가 9(J)이 아니면 `Begin(sid)`를 호출**하고, 효과를 실행했는지와 관계없이 항상 참을 반환한다. 기존 낙하 예약을 검색하지 않는다. 따라서 첫 이벤트가 J 프레임을 쓰기 전에는 요청이 반복될 수 있지만 J 상태에서는 재시작하지 않는다.

`MakePriestFallWalker`는 실제 사제 vtable만 선택하여 이 몸체를 실행한다. 다른 walker는 명시 fallback을 공급해야 하며 기본적으로 미복원 예외다. 이 함수는 기존 [`RawIslandLifecycle::SurfacePostDestroy`](../../cpppj/src/o/RawIslandLifecycle.h)의 실제 일반 finder가 찾은 walker에 연결할 수 있다. 표면 삭제 caller가 반환값을 버려도 `TryFall`의 정수 반환 계약은 유지한다.

이번 단계는 **표면 삭제의 일반 finder→사제 +0xc8→낙하 요청→실제 SharedRegular form/Kernel**을 연결한다. 표면의 전체 가상 destroy와 프레임 설정/진행·보호막 Pop/destroy·오디오 하위 효과를 완성한 것은 아니다.

## 독립 원본과 C++ 검증

[`decomp_carriercheck_oracle.py`](../../tools/decomp_carriercheck_oracle.py)는 세 PE 각각 **676개**, 총 **2,028개** 입력을 두 x87 제어 워드 `0x027f`/`0x037f`로 실행한다. 각 PE **1,352회**, 총 **4,056회 정상 반환**이며 두 정밀도 관찰이 같은 한 행을 저장한다.

Carrier 전체 몸체·실제 genus/CRT·사제 +0xc8/방향·CD J helper·기존 낙하 요청·SharedRegular 생성자는 원본 명령이다. 합성 비사제 +0xc8 반환, new·BaseProcess 부착·보호막 생성 요청·Unpop/Repop·위치 소리만 명시 대체한다. **사제 +0xc8와 낙하 요청 자체는 대체하지 않는다.** 허용 몸체/쓰기·ABI/보존 레지스터·SEH/x87·전체 raw/사건/정수 반환/현재 권한·assert/OS 0·SHA/행/입력 순서를 검사한다.

각 PE 실제 +0xcc **776회**, 사제 +0xc8/방향 **636회**, 낙하 요청 **414회**, 실제 공유 생성 **222회**를 실행했다. 합성 가상 +0xc8는 60회이며 DWORD `0x80000000`의 0 아님 반환도 정규화한다. 제외 genus·지면 2/4 비트·단순 절삭/거의 올림의 서로 다른 칸·float 재저장 정밀도 경계·raw state/extra 변이·J 방향·할당 성공 실패·효과 뒤 권한/좌표·제외 조건의 NaN/무한 좌표 읽기 생략을 포함한다. 손상된 지도 밖 spot 읽기는 대조 범위 밖이다.

새 [C++ 검사](../../cpppj/tests/CarrierCheckTests.cpp) 5개는 세 PE 전체 관찰 재생과 두 raw 배치의 실제 표면 삭제/finder→가상 사제 낙하→공유 form 예약/첫 Kernel 실행→J 상태 재시작 생략을 검사한다. 이어 비권한 postPop→실제 조회→공통 장부/깊이와 실제 나선 생성의 genus 래퍼→조회, 표의 현재 값 재조회와 같은 풀/미복원 분배 계약을 검사한다. 생성/소유자/Pop 및 프레임/공간/보호막/소리 효과는 명시 입력 경계다.

새 [fixture](../../cpppj/tests/fixtures/carriercheck-x86.tsv)·[근거 JSON](../../cpppj/recovery-carriercheck-evidence.json)은 UTF-8/LF다. 기존 fixture/감사 도구는 수정하지 않았다. 누적 인계 **355,474 + 2,028 = 357,502개**다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name carriercheck
python -X utf8 tools/decomp_carriercheck_oracle.py
python -X utf8 tools/decomp_carriercheck_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

Release **경고/오류 0**, CTest 내부 **451개·실패 0**(115.64초), 원본 근거 감사 **68종 모두 통과**. 최종 검사 로그는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 최신 단계를 따른다. 다음은 **일반 사제 Pop·프레임 설정/진행의 공간 효과·보호막 Pop/destroy/소리 수명**이다. 이어 저장 맵/raw 비표면 객체·GUI 건설·경제·전투·승패를 연결한다. 싱글플레이 완주와 Windows 장시간/창 회귀는 후속이다.
