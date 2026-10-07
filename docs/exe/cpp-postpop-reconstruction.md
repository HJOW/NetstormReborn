# cpppj raw 공통 postPop 일부 효과 복원

2026-10-06. 공통 postPop의 비용 집계·공급 목록·소유자별/전역 작업장 목록·타입 통계와 noGraph 리셋을 raw SID에 연결했다. 그래프 생성/영역 통지·AI 통지·배치 선택 효과는 미복원이며 요구하는 입력을 공간 변경 전에 거부한다. 실제 GameWorld와 raw SID는 아직 별개다.

## 원본 근거

| 경로 | 패치 10.78 | CD 10.72 |
|---|---|---|
| 공통 postPop | `004b0d30` | `004ae180` |
| 로컬 조건의 두 통계 표 증가 | `004c25e0` | `0040bc50` |
| 전역 통계 표 증가 | `004c25b0` | `0040bc20` |
| 공급 SID 추가 | `00472f70` | `0048a360` |
| 전역 작업장 SID 추가 | `0044f770` | `0045ec90` |
| 소유자별 작업장 SID 추가 | `00490050` | `004071f0` |
| 그래프 번호 리셋 | `004ad3e0` | `004adbd0` |
| AI 통지 입구 | `004166e0` | `004d8230` |
| 배치 선택 재검사 | `004da650` | `0041eac0` |
| 비용 표 인코딩/float 조회 | `0049c2e0`의 일부 | `004147a0` |

읽기 전용 헤드리스 Ghidra `ExportCreation.java`로 패치 10개/CD 13개 함수를 새로 내보냈다. 결과는 `extracted/postpop/<판본>/creation.c`, `functions.tsv`, 완료 로그는 `extracted/postpop-originals-ghidra.log`, `extracted/postpop-cd-ghidra.log`다. 기존 대응 2,496쌍·검토 앵커 24쌍은 유지한다. 원본 게임 프로세스나 OS API를 실행하지 않았다.

디컴파일은 비용 누적의 x87 피연산자를 생략한다. 실제 기계어에서 CD는 타입의 `+0xc4` float 비용에 이전 signed DWORD 집계를 더한 뒤 `ftol`로 절삭한다. 패치는 로드 시 `trunc(cost+type*23)`을 DWORD 표에 저장하고, postPop에서 `type*23`을 빼서 누적한다. 음수 소수 비용에서는 두 판본 결과가 다를 수 있다. 예를 들어 타입 74의 비용 -0.5·기존 집계 0은 패치 -1/CD 0이 된다. 정상 누적의 signed DWORD 경계 감김도 low DWORD로 유지했다. 이 집계는 Player의 현재 SP 차감이 아니다.

공급 목록은 활성 항목에서 같은 SID를 찾으면 추가하지 않는다. 작업장 목록은 중복을 검사하지 않는다. 모두 용량이 가득 차면 추가를 생략하고 배열의 미사용 DWORD를 보존한다. 공급/전역 작업장 함수는 중복 또는 가득 찬 목록에서도 생산 재계산 카운터를 증가시킨다. 소유자 0은 소유자별 작업장 추가를 생략하지만 전역 목록/통계에는 참여한다. 로컬 소유자 번호가 0이면 그 비교도 원본대로 수행한다. 세 통계 표는 모두 타입 번호로 접근하며, 두 로컬 표의 추가 의미는 이번 복원에서 확정하지 않았다.

효과는 flags bit 1이 있고 extra의 abstract/buried 마스크 9가 없을 때 적용한다. 최초 Pop의 firstPop은 이 플래그를 자동으로 추가하며, 이후 재등록에서는 다시 추가하지 않는다. 직접 flags 1로 postPop을 호출하면 통계/비용은 다시 증가한다. suppression 분기는 효과 없이 깊이만 감소한다. graphsEnabled·surface·flags 1·noGraph 조건의 리셋 바이트는 **패치 +30/CD +28**, invalidGraphNum 초기값은 두 판본 모두 254다. 그래프를 만드는 flood 경로는 구현하지 않았다.

## C++ 연결

- `o::SquidPostPopState`는 기존 용량을 유지하는 목록·세 통계 표·전역 집계·알림·깊이를 보관한다. `o::SquidPostPop`은 타입 표를 복사하고 같은 SidPool의 슬롯을 읽는다.
- `SquidPop`의 선택적인 다섯 번째 인자에 연결한다. 공간 쓰기 전에 firstPop/비전투 Activate 플래그를 정규화해 검사하고, 기존 표시→firstPop→void 해제 뒤 Activate/postPop을 실행한다. 포인터를 생략하면 이전 억제 경로를 보존한다.
- `.type`의 숫자 `cost`를 raw float 속성으로 읽는다. 누락은 0, 중복은 뒤의 값이 남으며 문자열 비용은 거부한다.
- 영역/표면 그래프 생성, AI가 붙은 소유자의 생성 통지, 활성 배치 선택, 파생 postPop, 손상 목록·소유자·비용 입력을 거부한다. 그래프/AI 효과를 완료한 것으로 처리하지 않는다.

## 검증과 재현

[독립 실행 기록](../../cpppj/recovery-postpop-evidence.json), [기대값](../../cpppj/tests/fixtures/postpop-x86.tsv), [실행 도구](../../tools/decomp_postpop_oracle.py), [C++ 검사](../../cpppj/tests/PostPopTests.cpp).

이전 [raw Pop](cpp-pop-reconstruction.md)·[공통 표시](cpp-display-reconstruction.md)의 내보내기와 새 postPop 함수 범위를 준비한 뒤 실행한다.

```powershell
python -X utf8 tools/decomp_postpop_oracle.py
python -X utf8 tools/decomp_postpop_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build --build-config Release --output-on-failure
python -X utf8 tools/cpp_world_smoke.py
```

두 판본×x87 53/64비트의 4시퀀스에서 **984회**를 대조했다: 직접 postPop 768, Pop 144, Unpop 72. 각 판본의 내부 postPop 456회·firstPop 32회·공급 함수 28회·소유자/전역 작업장 함수 각 36회는 상위 호출 수에 더하지 않는다. 준비 Reset/Create 각 4회와 패치 비용 인코딩 **192회 접두 구간**도 984회에 포함하지 않는다. 인코딩 구간은 `0049c338`부터 `0049c359` 직전까지이며 전체 타입 로더 실행이 아니다. 대체 함수/assert 도달은 0이다.

소수/음수/0 비용·signed DWORD 경계·가득 찬 목록·중복·소유자 0~8·abstract/buried·suppression·noGraph 리셋을 포함한다. 세 통계 표 전체와 목록의 활성/미사용 영역은 Adler-32, raw 슬롯 전체와 dirty 표의 모든 좌표/플래그는 직접 대조한다. Pop→Unpop→재등록에서는 공통 표시도 활성 상태다. `--verify`는 원본/도구/부모 도구/fixture SHA-256과 공개 호출 수를 확인한다.

x64 Release 경고/오류 0, CTest 내부 **124개·실패 0**. 최초 등록/재등록, 미복원 그래프·AI·배치 선택과 비유한 비용/손상 목록의 변경 전 거부, 실제 `.type` 문법의 숫자 비용 읽기도 검사했다. 새 기록을 포함한 누적 **53,468개**는 제한 x86 입력 수이며 게임 전체 완성도가 아니다. 앞선 공간 878개는 의존성 계약 대체, 수명 480개는 접두 구간, 해시 초기화 4개는 할당 없는 경로이고 생성자 359행/vtable 151행은 호출 수와 별도다.

이번 PC의 기존 창 허용으로 클론 월드 회귀를 다시 실행했다. 1-1/TEST01 선택·사제 이동·정지·카메라·복귀/재진입 **20개 상태**와 두 판본 초기 자료를 확인했다. 결과는 `extracted/cpp-world-smoke/report.json`, 로그는 `extracted/cpp-postpop-world-smoke.log`다. 설정을 복구하고 두 판본 원본 파일 해시가 같음을 확인했다. 이 GUI는 기존 임시 GameWorld 회귀이며 raw SID 월드 연결의 증명이 아니다.

## 남은 연결

후속에서 [정수 표면 Graph의 연결·flood·병합/표 관리](cpp-graph-reconstruction.md)를 복원하고 [raw graph byte·공통 postPop surface 분기·지원 Pop](cpp-rawgraph-reconstruction.md)에 선택 연결했다.

2026-10-08에는 실제 다리 vtable +0x20의 `00422150`/CD `00449890` 접두를 [별도로 복원했다](cpp-neighbor-reconstruction.md#다리-전용-postpop의-접두). extra 비트 갱신과 연결 요청이 공통 postPop보다 먼저 실행되며 공통 통계 억제도 이 접두를 막지 않는다. 실제 연결 객체 생성/소유자 전파는 명시적 콜백 경계다. 정확한 다리 가상 표와 콜백을 공급한 인스턴스만 추가로 지원하며 기존 공통 가상 표 목록과 다른 파생 함수의 거부는 유지한다.

주변 영역 통지·정상 다리/섬 Pop·그래프 삭제 분할/소진 복구·AI·배치 선택·생산 덱/자원 계산·SP 차감·dirty/grid·표면 부착 프레임·섬/다리/건물/파생 가상·삭제/의존 객체/참조 수명·form/process·SID 소진/Take 목록을 이어 복원한다. 실제 SID/표면/프레임을 GameWorld에 연결한 뒤 다리 배치/Construction·건설/경제/전투/AI/승패로 이어야 한다. 현재 미션 완주는 불가능하다.
