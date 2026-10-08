# Graph 활성 섬 생성·다리 삭제·끝 칸 재등록 통합

2026-10-08, 호스트 **VM-W11-CODEX**. 1차 기준은 10.78이며 CD 10.72도 비교했다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08(유지)**. 같은 PC의 기존 읽기 전용 내보내기를 사용했고 새 디컴파일이나 원본 게임/복사본·클론 창 실행은 없었다.

다른 AI가 LEFT_JOBS.md에 추가한 다리 삭제 재정의 누락을 확인하고, [RawSquidDestroyDispatch](../../cpppj/src/o/RawSquidDestroyDispatch.h)를 추가했다. [SurfaceLifecycleTests](../../cpppj/tests/SurfaceLifecycleTests.cpp)는 기존 섬 생성→지연 낙하→끝 칸 변환 흐름을 Graph와 실제 표시 연결까지 확장한다. GUI GameWorld는 아직 이 raw 흐름을 사용하지 않는다.

## 가상 삭제와 공통 몸체

기존 `extracted/bridgeevent`, `bridgeeffects`, `bridgedecay`의 두 판본 내보내기에서 아래 호출을 확인했다. 원본 주소를 호스트 함수 포인터로 실행하지 않는다.

| 호출 지점 | 10.78 | CD | 호출 방식 |
|---|---|---|---|
| 자기 삭제 이벤트 `0x2691` | `00422740` | `00449a20` | vtable +0x10 |
| 끝 칸 변환: 이웃 없음/옛 다리 삭제 | `004215d0` | `00449a20`에 인라인 | vtable +0x10 |
| dead 참조를 가진 연결 객체 정리 | `00422290` | `004498e0`에 인라인 | 연결 객체의 vtable +0x10 |
| 다리 삭제 재정의의 마지막 호출 | `004220f0` | `00449820` | 공통 몸체 `004af780` / `004ab7e0` 직접 호출 |

`RawSquidDestroy`는 공통 몸체로 유지한다. 가상 호출 지점은 `RawSquidDestroyDispatch::Destroy(sid,flags,host.Hooks())`로 연결한다. 다리의 실제 가상 표이면 다음 조건을 순서대로 평가한다.

1. 편집기 아님·권한 있음·extra의 abstract/buried 비트 없음.
2. 현재 raw graph byte의 signed WORD 표면 수가 5 이상.
3. 현재 raw 프레임 코드의 hard 비트 `0x40` 또는 debugKeep.

모두 통과하면 삭제를 거부한다. 거부 시 dead 비트·선택·장부·표시·프로세스·공간·SID를 바꾸지 않는다. 모드/프레임/그래프를 매번 다시 읽으며 앞 조건이 실패하면 뒤 조회를 생략한다. Graph 미연결은 비활성 표면 수 0으로 처리한다. 254의 원본 NULL 역참조와 255의 표 밖 접근은 호스트에서 변경 전 예외로 남긴다. 다른 지원 자산은 공통 몸체로 전달하고, 명시적인 공통 몸체 호출은 다리 거부를 우회한다.

## Graph와 표시 연결

빈 월드에서 `RawGraph::Rebuild(true)`로 원본 전체 초기화를 먼저 수행한다. 원본은 **0번을 예약**하고 reserved의 low byte를 1로 쓴다. 이 초기화를 생략한 첫 검사는 첫 객체의 기본 graph byte 0과 첫 할당 번호 0이 같아 Flood가 바로 반환했고, 표면 수가 실제보다 1 작았다. 정상 초기화 뒤 일반 레코드와 raw 슬롯 수가 일치한다. 첫 객체가 예약 0번에서 나올 때 원본 Flood의 감소가 남기는 **0번 surfaces=-1, inUse=1, reserved=1** 흔적도 별도로 확인한다.

같은 Graph를 `SquidPostPop`과 `SquidDestroyLifecycle`에 공급하고 graphsEnabled를 켠다. 표시에는 `MakeSquidFrameHooks`, `SquidDisplay`, 실제 Unpop/Pop, Renderer 변경 표를 연결한다. 9칸 noIsland의 F/H 처리로 받침·종유석·소유자 전파를 실행한 뒤 다음 네 흐름을 검사한다.

| 흐름 | 확인한 결과 |
|---|---|
| 보통 다리+오른쪽 이웃 | O 프레임 56의 끝 칸 생성, 옛 다리/연결/Regular 반납, 같은 위치 Pop, 표면 11개 |
| 고립 다리 | 새 끝 칸 없이 다리/연결/Regular 제거, 표면 9개 |
| hard 다리 | 낙하 이벤트 뒤 수명 4·프레임 46·연결/Regular 유지, 자기 삭제 이벤트도 거부; editor 전환 후 정리, 표면 10개 |
| 받침 삭제 flags `0x1000` | 낙하 예약 생략, 후속 가상 다리 삭제로 연결 정리, 표면 10개 |

일반 그래프 레코드의 surfaces/inUse를 전체 살아 있는 raw 표면 번호와 대조하고, 공통 pre/post·postPop 깊이 0, 타입 수/비용·해시/spot·풀 여유·소유자/수명/부모 단어를 함께 확인한다. 두 판본의 실제 SHP를 공급한 선택 검사도 같은 네 흐름을 사용한다. 최종 삭제 구간의 표시 통지는 각 판본에서 교체 4회, 나머지 각 3회이며 부분 Draw/Present로 이어진다. 픽셀 버퍼 갱신 검사로 실제 스프라이트 화면 검사를 대신했다고 주장하지 않는다.

## 재현과 검증

일반 CTest는 원본 자산 없이 합성 SHP와 저장된 실제 다리 코드를 사용한다. 원본 파일을 읽는 선택 검사는 테스트 실행 파일에서 별도로 실행한다.

```powershell
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
cpppj/build/bin/Release/netstorm_tests.exe --inspect-surface-graph originals
cpppj/build/bin/Release/netstorm_tests.exe --inspect-surface-graph originalCD --cd
```

Release **경고/오류 0**, CTest 내부 **257개·실패 0**(62.12초)(기존 251 + 가상 삭제 5 + Graph 통합 1). 실제 자산 **두 판본 × 네 흐름 = 8개 모두 통과**. 기존 근거 감사 bridgedecay/bridgeevent/bridgeeffects/graphrebuild/destroygraph/rawgraph/postpop/islandpostpop/setframe/display **10종 모두 통과**했다. 새 독립 x86 기대값은 생성하지 않았으며 누적 제한 x86 **131,542개 유지**다. 이번 검사는 이미 복원된 몸체들의 호스트 통합이고 전체 원본 실행의 독립 대조가 아니다.

로그는 Git 제외 `extracted/build-surface-graph.log`, `ctest-surface-graph.log`, `surface-graph-originals.log`, `surface-graph-cd.log`, `audit-surface-graph.json`이다. 초기 검사에서 발견한 Graph 초기화 누락·해시 단계 필드 위치·Transmit 억제에 따른 훅 수 기대값은 수정했다.

다음은 raw GameWorld/GUI와 Kernel 프레임·게임 시각 연결이다. 애니메이션 프로세스·실제 walker 낙하·파편·소리·건설·경제·전투·승패와 장시간 변이 검사는 남았다. 원본 파일·AGENTS.md·dotnetpj·dotnet 인계를 변경하지 않았고 커밋/푸시하지 않았다.
