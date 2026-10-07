# cpppj 상위 Add·Detach·Pop의 Graph 소진 복구 연결

2026-10-07. **10.78 기준**으로 [전역 재구성 API](cpp-graphrebuild-reconstruction.md)를 Add·삭제 준비·공통 postPop·Pop의 내부 할당에 연결했다. `DESKTOP-HJOW`에서는 게임/복사본·업데이터/설치 도구·클론 창을 실행하지 않았다. Ghidra 읽기 전용 헤드리스, 제한된 Unicorn 함수 실행, 콘솔 Release/CTest만 사용했다.

## 전체 풀 계약

[GraphRecovery](../../cpppj/src/o/Graph.h)의 `FullPool`을 선택한 [RawGraph](../../cpppj/src/o/RawGraph.cpp)는 매 호출 모든 할당 자산 슬롯에서 표면을 수집한다. 지역 입력의 기본 `Reject` 정책은 유지하며 기존 소진 보호 검사가 그대로 통과해야 한다.

```cpp
// 모든 할당 자산 표면을 읽을 수 있는 raw 풀에서만 자동 전역 재구성을 선택한다.
netstorm::o::RawGraph graph(pool, hash, spots, types, frames, records, stack,
    netstorm::o::GraphRecovery::FullPool);
// 기존 SquidPostPop/SquidPop 연결을 통해 같은 정책으로 Pop 전 검사와 최종 반영을 수행한다.
```

자동 복구 경로는 hash에 나타나지 않는 다른 void/contained 표면까지 포함한다. 전체 풀의 타입/프레임·정수 좌표·번호·0단계 hash 상태를 변경 전에 확인한다. 자산 타입 74 이상·SID 5..32767 범위이며 미복원 form/process 타입은 거부한다. 실제 raw GameWorld를 연결할 때 이 입력 계약을 충족하거나 내부 타입 처리를 복원해야 한다.

Pop 사전 검사는 원천의 새 좌표·void 해제·hash 머리/spot 등록을 예측한 전체 스냅샷을 사용한다. 다른 void 표면의 프레임 손상이나 내부 타입이 있으면 풀/공간/Graph/비용/깊이를 변경하지 않는다. 등록 뒤 공통 postPop도 같은 정책으로 계산하므로 그래프 번호가 소진됐을 때 재구성→새 번호 확보→원래 flood 순서를 유지한다.

## 재구성 후 상위 함수의 순서

Add는 먼저 이웃 번호를 읽는다. 유효 연결이 없으면 번호 확보를 요청하고, 소진된 경우 전체 재구성 뒤 확보한 번호로 원천을 다시 flood한다. 재구성이 이미 원천에 배정한 레코드의 크기가 0이 되더라도 **inUse를 임의로 지우지 않는다**. 이는 native flood가 크기만 감소시키는 동작이다.

Detach는 **처음 위치 조회로 선택한 레코드와 원래 연결 목록**을 보존한다. 첫 번호 확보가 전역 재구성을 일으킨 뒤에는 매 연결의 현재 번호를 다시 읽지만 최초 선택한 번호 자체를 새 원천 번호로 바꾸지 않는다. 마지막 1/특수 9 감소와 정상/rebuild 분할의 레코드 해제도 원래 선택한 레코드에 수행한다. 따라서 예약 0번이 감소하거나 해제되는 입력도 정리하지 않고 원본 결과를 유지한다. 전역 재구성은 dead 원천의 graph byte도 바꿀 수 있으며 state/type/frame/좌표/payload는 보존한다.

내부 할당은 같은 복사본에서 재구성하여 상위 함수가 보관한 레코드/객체 참조를 무효화하지 않는다. 성공 후에만 Graph의 번호/표/스택과 raw graph byte를 함께 반영한다. `Rebuild(false)`의 중첩 소진과 전체 재구성 중 재소진은 기존 변경 전 거부 계약을 유지한다.

## 디컴파일·검증

세 읽기 전용 프로젝트에서 각각 다음 여섯 함수를 새 `extracted/graphrecovery/<판본>/`에 내보냈다. 각 로그의 완료 마커는 **6개**다.

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| Add | `004633c0` | `0045b7d0` |
| Detach | `004637b0` | `0045bfd0` |
| Allocate | `00463330` | `0045c390` |
| Rebuild | `00463110` | `0045bd60` |
| Pop | `004b02d0` | `004ad490` |
| postPop | `004b0d30` | `004ae180` |

[독립 도구](../../tools/decomp_graphrecovery_oracle.py), [fixture](../../cpppj/tests/fixtures/graphrecovery-x86.tsv), [SHA·호출 기록](../../cpppj/recovery-graphrecovery-evidence.json). 세 실제 PE × 두 x87 정밀도 × 8입력 × 네 상위 경로 = **192회**다. 각 공개 호출에서 실제 **32768슬롯 전체 재구성을 한 번씩** 수행했음을 관찰한다. 준비 Reset/Create와 내부 Allocate/Flood/postPop 도달은 공개 호출 수에 더하지 않는다. CD/추가 10.37의 코드 배치는 같으며 새 시대 규칙으로 세지 않는다.

7개 raw 슬롯·255개 Graph 레코드·dirty 항목 전체는 직접 비교한다. 전체 풀/네 단계 hash/spot/4096 DWORD 스택/비용·깊이·통계를 기존 독립 관찰기로 대조한다. 검토한 Ghidra 몸체 밖 실행과 쓰기를 거부하고 OS/대체 함수는 사용하지 않는다. 생성 준비에서 전 시퀀스의 특수 9 감소 타입이 생성 금지 타입으로 남은 문제는 다음 실제 Create 전에 합성 입력을 초기화하여 해결했다. 원본 assert 몸체는 실행하지 않는다.

재현 명령:

```powershell
& tools/ghidra/run_script.ps1 -Edition originals -Script ExportCreation.java -ScriptArgs @('extracted/graphrecovery/originals','004633c0','004637b0','00463330','00463110','004b02d0','004b0d30')
& tools/ghidra/run_script.ps1 -Edition originalCD -Script ExportCreation.java -ScriptArgs @('extracted/graphrecovery/originalCD','0045b7d0','0045bfd0','0045c390','0045bd60','004ad490','004ae180')
& tools/ghidra/run_script.ps1 -Edition original1037 -Script ExportCreation.java -ScriptArgs @('extracted/graphrecovery/original1037','0045b7d0','0045bfd0','0045c390','0045bd60','004ad490','004ae180')
python -X utf8 tools/decomp_graphrecovery_oracle.py
python -X utf8 tools/decomp_graphrecovery_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

재생성에는 기존 SID/생성·graphremove·graphrebuild 내보내기도 필요하다. 일반 CTest는 고정 fixture만 사용하므로 원본 실행 파일/Python/Ghidra가 필요 없다. 최종 Release/CTest 결과는 [LEFT_JOBS](../../LEFT_JOBS.md)의 최신 절에 기록한다.

합성 자산 타입/FrameCode/SHP·정수·동결 시계·비전투 null 큐 범위다. 건물 부착/전투 알림·특수 타입 위치 조회·SID 소진의 파생 삭제/통지·form/process·raw GameWorld는 남았다. **미션 완주는 불가능하며 GUI 회귀는 `DESKTOP-HJOW` 이외의 PC에 인계한다.** 사용자 추가 지침대로 cpppj/디컴파일 기록은 LEFT_JOBS.md에, 별도 dotnetpj 작업 기록은 LEFT_JOBS.dotnetpj.md에 둔다.
