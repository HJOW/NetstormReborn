# 건설 유닛 밀어내기와 섬 실패 정리 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 [생산 칸 반환](cpp-cursor-return-piece-reconstruction.md)의 다음 건설 항목을 진행했다. 시작 작업 트리는 깨끗했고 현재 PC가 마지막 디컴파일 PC와 같았다. `constructioneffects` 목록 **30/26/26개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본 게임·analyzeManager·클론 창은 실행하지 않았다.

## 복원한 몸체와 연결

[`RawConstructionEffects`](../../cpppj/src/o/RawConstructionEffects.h)는 [서버 건설 요청](cpp-construction-request-reconstruction.md)이 호출하던 두 효과의 몸체를 복원한다.

| 기능 | 10.78 | CD/10.37 | 차이 |
|---|---|---|---|
| 기존 유닛 밀어내기 | `00443800` | `004d2450` | 10.78만 요청 플레이어와 성공 반환값이 있다. |
| 섬 정리/확정 실패 정리 | `0046e6c0` | `0048f960` | 같은 삭제 순서·전파/표시 억제·9/1/1 조각 수 조건이다. |
| float 발자국 | `0049ae80` | `004449d0` | 자른 반대 모서리와 유효한 시작점을 잇는다. |
| 사각형 폭 | `0041d3e0` | `00440210` | 오른쪽−왼쪽, 높이는 축소 조건에 쓰지 않는다. |
| 현재 genus | `004ac200` | `004abae0` | 현재 raw 타입 BYTE의 `flags2`를 읽는다. |
| 일반 finder | `004b16d0`/`004b1810` | `004eae20`/`004eafe0` | 실제 4단계 해시와 다음 링크 저장 순서를 재사용한다. |

밀어내기의 디컴파일 대응은 신뢰도 A, `callseq/strong` 0.704이고 섬 정리는 신뢰도 A, `string/strong` 0.687이다. 디컴파일에 빠진 좌표와 메시지 배율은 실제 명령/관찰로 확인했다.

`MakeConstructionEffectsRequestHooks`는 같은 풀의 요청 `displace`·`displaceCd`·`cancel`을 이 몸체에 연결한다. 기존 `MakeConstructionRequestHooks`의 전체 MayPlace/실제 확정 어댑터와 합쳐 쓸 수 있다. CD는 플레이어를 전달하지 않고 반환값도 사용하지 않는다. 두 모듈의 전역/타입/지도와 수명은 호출자가 일치시킨다.

## 밀어내기 순서

1. 요청 타입의 float 발자국을 구한다. 사각형 폭이 **3 이상**이면 네 모서리를 각각 1만큼 안으로 옮긴다. 높이가 좁아도 같은 축소를 수행한다.
2. 작은 모서리는 절삭하고 큰 모서리는 **+0.99999 뒤 절삭**하여 일반 finder를 시작한다. 원본 float 절삭 helper는 1~255로 자르지 않는다. 지도 범위 보정은 finder가 수행한다.
3. 각 후보의 현재 genus가 10.78 **`0x218000`**, CD **`0x210000`**과 겹칠 때 이동 자리를 찾는다. 10.78만 `0x8000` 후보를 추가로 검사한다.
4. genus `0x200000` 후보는 `(절삭(x−정수((폭−1)/2)), 절삭(y+1))`을 먼저 본다.
5. 못 찾으면 아래쪽 줄을 가까운 순서로 탐색한다. 각 줄의 x는 `절삭(x)`부터 `절삭(x)−폭`까지 역순이며 **폭+1칸**을 포함한다. 10.78은 아래 **세 줄**, CD는 **두 줄**이다.
6. 그래도 못 찾으면 오른쪽 열을 가까운 순서로 탐색한다. 각 열의 y는 `절삭(y)`부터 `절삭(y)−높이`까지 역순이며 **높이+1칸**을 포함한다. 10.78은 오른쪽 **세 열**, CD는 **두 열**이다.
7. spot에서 **`0x10`이 없고 `0x02`/`0x04` 중 하나가 있으면** 첫 이동 자리로 선택한다. 없으면 후보를 건너뛰고 계속한다.
8. 10.78은 자리를 찾은 뒤 소유자를 검사한다. 요청 플레이어와 다르고 `alliances[요청*9+소유자]`가 0이며 현재 genus에 **`0x8000`이 없으면 즉시 false**다. CD에는 이 검사 자체가 없다. 따라서 자리 없는 적대 후보는 10.78에서도 요청을 거부하지 않는다.
9. 현재 유닛 타입의 경로 contained 타입을 조회하고 `(SID,경로 타입,0,1)`로 첫 살아 있는 경로 삭제를 요청한다. **Unpop(0) → 현재 genus 재조회 → Pop(찾은 x,y,0)**을 호출한다.

첫 후보를 옮긴 뒤 다음 적대 후보에서 실패하더라도 앞선 효과를 되돌리지 않는다. 경로 타입 조회/contained 삭제와 타입별 가상 공간 이동의 하위 몸체는 명시 경계다. 정수 탐색의 산술만 64비트로 넓혀 지도 밖 입력을 거부하는 과정의 C++ signed overflow를 피했다.

## 섬 정리와 실패 정리

`DropIsland(x,y,flags,player)`는 원본 Islanddropper 몸체다. 건설 요청은 본 확정이 실패한 `flags1 & 0x400` 타입에 **flags `0x1000`**으로 호출한다.

1. 차단 전역이 켜졌으면 전체를 생략한다.
2. 서버이며 `flags & 0x50`이 0이면 다른 플레이어에게 정리를 전파한다. 메시지 좌표는 **`__ftol(좌표×65536)`의 하위 DWORD**, 즉 **16.16 고정소수점**이다. `20.5`는 20이 아니라 **1,343,488**이다. 배율 주소는 10.78 `00501ad0`, CD `005046b8`이다. 초기화/실제 전송은 경계다.
3. 전파 뒤 **현재 섬 타입/발자국**을 읽고 `(x−(폭−1),y−(높이−1),x,y)`의 일반 finder를 시작한다. 시작점 1·폭 3이면 전달 범위의 왼쪽은 **−1**이며, 1로 미리 자르지 않는다.
4. 후보의 소유자 BYTE가 요청 플레이어 **DWORD 전체**와 같을 때만 처리한다. `3`과 `0x103`은 다르다.
5. noIsland 또는 islandStalag 후보는 `flags & 0x1000`이 없을 때 **추락 표시 → 현재 좌표의 `islandFall.wav`(loop 0)**를 호출한다. 이어 **`flags | 0x48`**로 삭제하고 각각의 수를 센다. 실패 정리 `0x1000`에서는 표시/소리를 생략한다.
6. islandStump는 **`flags | 0x48`**, island는 **flags 그대로** 삭제한다. island 수만 센다.
7. 삭제와 관계없이 **현재 raw 타입의 genus**를 다시 읽고 `0x10000` 후보에 가상 `+0xc8` 알림을 보낸다. 추락 표시가 좌표를 바꾼 경우 소리도 바뀐 좌표를 사용한다.
8. noIsland/island/islandStalag 수가 모두 0이면 보고하지 않는다. 하나라도 있으면 **9/1/1** 조건을 순서대로 검사하고 맞지 않으면 원본 조건식과 줄 **`0x7b`/`0x7c`/`0x7d`**를 보고한다. 보고 함수가 반환하면 다음 검사를 계속한다. 그루터기와 알림만 처리한 경우는 카운트를 만들지 않는다.

## 안전 계약과 남은 효과

C++는 짧은 지도·누락된 필수 효과·없는 타입·비유한 좌표·1~256 밖의 발자국을 거부한다. 실제 spot 조회가 지도 밖이거나 10.78의 관계 인덱스가 0~8 밖이면 해당 조회 전에 거부한다. 차단된 섬 정리는 잘못된 좌표라도 효과 없이 반환한다. 전파 경계가 바꾼 현재 섬 타입/발자국도 검사한다. 원본에 이 안전 검증이 있다는 뜻은 아니다.

이 단계는 **선택한 모듈의 몸체와 호출 순서**를 복원했다. 경로 contained 조회/삭제, 타입별 공간 Pop·파생 Unpop/삭제, 추락 표시/가상 알림, 메시지 머리/실제 수신·전송은 경계다. 일반 Form 부착/이벤트 0·최종 소멸, 사제 선택/이동/건설 진행, 실제 배치 판정/미리보기·건설 GUI는 후속이다. **게임에서 건물을 지어 미션을 완주할 수는 아직 없다.**

## 독립 원본 관찰과 C++ 검사

[`decomp_constructioneffects_oracle.py`](../../tools/decomp_constructioneffects_oracle.py)는 세 PE의 실제 밀어내기/정리 전체·float 발자국/폭·절삭/거의 올림·일반 finder·spot·현재 genus를 실행한다. 효과와 보고만 대체하며 탐색기 Begin의 인자는 대체 없이 관찰한다. 허용 몸체 밖 실행과 스택 밖 원본 쓰기를 거부하고 스택·보존 레지스터·x87 균형·지도 불변·SHA·사건/진입/정상 반환 수를 감사한다.

새 저장 입력은 **3,534개**(각 PE **1,178개**: 밀어내기 **770**, 섬 정리 **408**)다. 두 x87 정밀도의 루트 정상 반환 **7,068회**에서 같은 관찰을 얻었다. 누적 저장 입력 **474,501개**(470,967 + 3,534). 조각 수가 틀린 입력의 **보고 분기는 실제 실행**하고 예상 보고 경계 **504회**를 기록했으며, 원본 보고 함수 몸체/OS는 실행하지 않았다. 실제 게임/OS 호출은 0이다.

[`ConstructionEffectsTests.cpp`](../../cpppj/tests/ConstructionEffectsTests.cpp)의 새 검사 **6개**는 세 PE 전체 재생, 잘못된 자료/풀 연결 거부, 효과 뒤 현재 자료 재조회와 요청 통합을 확인한다. 새 통합 검사는 MayPlace와 본 확정 실패를 명시 경계로 둔다. 실제 factory로 만든 유닛/섬 조각을 같은 풀/해시에 놓고 **요청 → 실제 밀어내기 → 본 확정 실패 → 실제 섬 정리 → SquidUnpop/RawSquidDestroy/풀 반납**을 실행한다. 10.78 첫 적대 요청은 효과 없이 거부되고, 동맹을 켠 요청과 CD 요청은 유닛을 `(20,22)`로 옮기고 섬 조각 **11개**를 반납한다. 파생 pre/post 삭제의 내부 효과는 깊이 완료 경계로 제한했고 유닛/조각의 최초 배치와 Pop은 해시 입력 경계다. 기존 전체 MayPlace/확정/통지 통합 검사는 회귀로 유지했다.

Release 경고/오류 **0**·CTest 내부 **606개·실패 0**(158.44초, 전체 158.52초), 근거 감사 **95종 모두 통과**. 새 변이 12개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과).

## 다음 순서와 재실행

**2026-10-11 [확정의 실제 그래프 기준점 연결과 배치 통지의 소유 사제 조회](cpp-construction-selection-reconstruction.md)를 완료했다.** 기존 “확정의 실제 사제 선택”은 서로 다른 두 조회를 혼동한 표현이었다. 다음은 건설 사제 생성/이동 `00443000`/CD `004d1120`, 건설 시작 `00443fe0`/CD `004d06a0`와 진행/완료·접근점/contained다. 경로 contained와 타입별 공간/삭제·추락/알림·메시지 수신·건설 GUI도 남는다. 한국어 3차 → outpost/LAN 4차 → 요구사항/MCP 5차 순서를 유지한다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructioneffects
python -X utf8 tools/decomp_constructioneffects_oracle.py
python -X utf8 tools/decomp_constructioneffects_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기본 검사는 [fixture](../../cpppj/tests/fixtures/constructioneffects-x86.tsv)를 읽고 게임/Python을 실행하지 않는다. [근거 JSON](../../cpppj/constructioneffects.json)·문서·새 소스는 UTF-8이다. 내보내기는 Git 제외 `extracted/constructioneffects/`, 로그는 `extracted/constructioneffects-*.log`다. 보호 원본·AGENTS.md·dotnetpj 변경, 커밋/푸시는 없다.
