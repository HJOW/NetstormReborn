# noIsland 최초 등록과 받침 소유자 전파 복원

> 기준 10.78. 마지막 디컴파일 수행 PC: **VM-W11-CODEX**, 2026-10-08. 같은 PC의 읽기 전용 Ghidra 프로젝트에서 [함수 목록](../../tools/ghidra/islandpostpop-functions.json)을 내보냈다. 원본 게임·복사본·업데이터를 실행하지 않았다.

구현은 [RawIslandPostPop](../../cpppj/src/o/RawIslandPostPop.h)과 `SquidPostPop::SetSurfacePrefix`다. `SquidPop`은 실제 noIsland 가상 표와 연결된 접두를 확인한 뒤 배치를 허용한다. 접두의 받침 생성/중첩 Pop을 마친 뒤 자기 공통 postPop을 같은 flags로 호출한다.

| 동작 | 10.78 | CD/별도 10.37 |
|---|---|---|
| noIsland postPop | `004423b0` | `004d2790` |
| 서/북 noIsland 글자 조회 | `00442350` | `004d03a0` |
| 3×3 받침 소유자 전파 | `0044bd20` | `00461f00` |
| 좌표의 표면 머리 조회 | `0040eaf0` | 호출자 안에 인라인 |
| 첫 프레임 검색(assert 포함) | `0049a940` | `004442f0` |
| noIsland 가상 표 | `00506f08` | `00506ad0` |

`flags & 1`이 없으면 파생 효과를 수행하지 않는다. 최초 등록에서는 다음 순서다.

1. 요새 읽기 중첩 값(`005c89b8` / CD `005178d4`)이 0이 아니면 서쪽 `(x−1,y)`와 북쪽 `(x,y−1)` 칸의 글자를 읽는다. 조회는 지도 머리 하나만 읽으며 해당 타입이 noIsland일 때만 프레임 코드의 방향 글자를 반환한다. 이 경로는 좌표에 보정값을 더하지 않는다.
2. 서쪽 C→G, F→C, A→D, B→A, E→H, I→E다. 다른 글자/없는 칸이면 북쪽 F→B, B→I, 그 밖에는 F다. `P` 변형의 번호 1 프레임을 검색해 필드를 직접 쓴다. `SquidFrame::Set`이나 표시 갱신을 호출하지 않는다. 행 우선 배치에서 `F C G / B A D / I E H`가 된다.
3. 현재 글자가 F면 섬 받침을 flags 2로 생성한다. 자기 소유자를 가상 지정하고 받침 타입의 발자국 크기를 읽어 `(x+폭−1,y+높이−1)`에 flags 0으로 가상 Pop한다. 받침의 파생 postPop이 종유석도 생성한다.
4. 소유자 전파 억제 값(`005c85d4` / CD `0051894c`)이 0이고 현재 글자가 H면 왼쪽 위 표면 객체의 소유자로 받침 묶음을 갱신한다. 패치는 이 표면 칸이 없으면 assert에 도달하고 CD는 갱신을 생략한다.
5. 연결 순회(`RawBridgeConnect::Connect`)를 호출한다. 이후 공통 postPop은 호출자가 수행한다.

`SetSupportOwner(x,y,owner)`는 먼저 좌표의 표면 머리가 noIsland인지 확인한다. 맞으면 **x 바깥/y 안쪽 반복 순서**로 오른쪽 아래 기준점의 3×3 머리 객체를 순회한다. 객체 타입에 제한을 두지 않고 가상 소유자 지정 → 가상 표시 갱신(+0x88)을 부른다. 이어 같은 위치의 island → islandStalag 순서로 각각 같은 처리를 한다. 없는 칸/받침/종유석이 있으면 정상 객체의 효과를 마친 뒤 진단 경계를 부른다. 원본 진단 문자열 생성과 창 표시는 콜백 경계로 남겼다.

소수 좌표 규칙은 디컴파일 C만으로는 보이지 않아 실제 명령에서 확인했다.

- 진입 조건의 표면 조회는 **float 0.9998999834060669(비트 `3f7ff972`)를 x87에서 더한 뒤 절삭**한다. 실제 3×3 순회 기준점은 보정하지 않은 x/y 절삭값이다. 두 조회 칸은 달라질 수 있다.
- H의 왼쪽 위 조회: 패치는 `(float)(x−2)`에 위 보정값을 더해 절삭한다. CD는 x87에서 float 1.000100016593933(비트 `3f800347`)을 한 번 빼고 절삭한다. y도 같다.
- 서/북 조회와 소유자 순회는 고정 지도(`005c84bc` / CD `0052d590`)를 읽는다. H 왼쪽 위와 전파 진입 조건은 현재 해시 지도(`00542514` / CD `005670cc`)를 읽는다. C++은 두 지도 span을 따로 받을 수 있다. 기본 구성은 같은 0단계 지도다.

독립 기대값 생성은 [decomp_islandpostpop_oracle.py](../../tools/decomp_islandpostpop_oracle.py)다. 탐색·연결·프레임 선택·지도 조회·3×3 순회는 실제 PE 명령으로 실행한다. 생성/가상 소유자/Pop/표시 갱신/공통 postPop은 인자와 순서를 기록하는 대체다. 진단 문자열 작성/표시/assert도 대체하며 효과 이후 진단 발생을 비교한다. 예상하지 못한 assert와 허용 범위 밖 명령/쓰기는 거부한다. 세 PE와 x87 53/64비트 관찰이 같아야 저장한다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name islandpostpop
python -X utf8 tools/decomp_islandpostpop_oracle.py
python -X utf8 tools/decomp_islandpostpop_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

검증: 세 실제 PE 각각 **552개 = 접두 402 + 소유자 전파 150**, 합계 **1,656개**, 공유 장면 **332개**가 C++과 전부 일치한다. 입력은 서/북의 없음·다른 타입·9개 글자, flag 비트 없음, 중첩/억제, 소수/가장자리 좌표, 프레임 유지, 연결 생성, 3×3의 다른 타입/누락/받침 누락을 포함한다. 두 x87 정밀도를 각각 실행하며 판본별 진단 130회와 그 뒤 예상 assert 130회(입력 재실행 두 배 포함)를 확인했다. 예상 밖 assert는 0이다.

두 판본의 콘솔 통합에서 `SquidFactory` → noIsland 9칸의 실제 `SquidPop` → 첫 F의 받침 생성 → 받침 postPop의 종유석 생성 → 마지막 H의 소유자/색 전파가 통과했다. 표시 갱신 11회, noIsland/island/islandStalag 통계 9/1/1, 중첩 깊이 0, 접두 누락 시 공간 쓰기 전 거부도 확인했다. CD의 별도 고정 지도 선택을 따로 검사했다. **Release 경고/오류 0·CTest 내부 240개·실패 0**(CTest 전체 100.04초), 누적 제한 x86 **131,542개**, SHA/경계 감사 **31개 모두 통과**다. 같은 PC에서 `regiongraph`도 재생성해 기존 1,536개 fixture가 바이트 단위로 같음을 확인했다.

같은 PC에서 클론 창 회귀 window·renderer·menu·world **4종도 통과**했다. 이 검사는 기존 임시 GUI 월드의 회귀이며 새 받침 처리를 GUI에서 확인한 것은 아니다. 원본 설정 파일은 스모크가 기존 상태로 복구했다.

GUI raw GameWorld, Graph 활성 상태, 실제 표시·진단 출력·커널/게임 루프 연결은 후속이다. 원본의 잘못된 프레임/누락 H 칸에 대한 assert 경로는 C++에서 예외로 거부한다. 예외 뒤 원본처럼 계속 실행하는 동작까지 복원한 것은 아니다. 새 모듈의 변이 검사는 아직 실행하지 않았다.
