# raw 표면 월드·GUI·고정 게임 시각 연결

2026-10-08, 호스트 **VM-W11-CODEX**. 1차 기준 10.78, CD 10.72의 실제 자산도 검사했다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-08(유지)**. 같은 PC의 기존 읽기 전용 내보내기와 복원 모듈을 사용했다. 새 디컴파일·독립 x86 입력·원본 게임/복사본 실행은 없었다. 이 단계는 새 GUI 호스트 통합이며 원본 GameWorld 전체 함수의 디컴파일 복원을 의미하지 않는다.

[RawSurfaceWorld](../../cpppj/src/client/RawSurfaceWorld.h)가 본섬 `isle`·`noIsland`·다리·받침·종유석·연결 조각을 실제 SID 풀로 소유한다. [GameWorld](../../cpppj/src/client/GameWorld.cpp)는 이 슬롯의 현재 프레임/좌표/소유자로 스프라이트를 제출한다. 이전 임시 목록의 다리와 정적 받침 스프라이트는 표시하지 않는다. 원본 타입/SHP·생성자·소유자·Pop/Unpop·파생 postPop·프레임 지정·Graph·가상 삭제·ProcessForm/Regular가 같은 풀과 공간 지도를 공유한다.

## 초기 등록

1. 기존 저장 미션/지형 어댑터가 실제 타입 번호와 프레임·정수 좌표·소유자를 공급한다. 전체 입력의 유한 좌표·지원 타입·프레임·소유자를 변경 전에 확인한다.
2. 본섬 → 행 우선 noIsland → 다리 순서로 실제 생성/소유자/Pop을 실행한다. noIsland는 복원된 F/H 몸체로 받침·종유석·9칸을 구성한다. 저장 입력 순서가 뒤섞여도 서/북 이웃을 먼저 등록한다.
3. 초기 대량 등록은 Graph 갱신을 끈 상태에서 수행하고, 완료 후 `RawGraph::Rebuild(true)` 한 번으로 0번 예약과 전체 연결 성분을 만든다. 이후 graphsEnabled를 켜고 요새 적재 카운터를 0으로 되돌린다. 초기 Graph 표면 수와 실제 살아 있는 표면 슬롯 수가 같다.
4. 저장 건물의 createsIsland/geyser 소유자는 기존 초기 월드 어댑터에서 `SetSupportOwner`로 전달한다. 건물 생성 전체 몸체가 복원된 것은 아니다. 이 초기 어댑터의 색 모드는 battle=true·mission=false이며 플레이어 색 표를 사용한다. 원본 미션 전역의 전환 순서 전체는 후속이다.

현재 초기 입력은 1..255의 정수 좌표와 isle/noIsland/bridge로 제한한다. 원본 signed SID 한계인 32768슬롯을 사용하며 원본 서버/클라이언트 할당 정책을 유지한다. 임의 사용자 맵의 불완전 받침·SID 소진·최대 크기 지도는 이번 완료 범위에 포함하지 않는다. 입력 필드 사전 검사 뒤 Pop/파생 효과에서 실패한 상태의 재시도/롤백은 아직 제공하지 않는다.

## 게임 루프와 수명

`Client::Time()`의 한 프레임에 고정된 `game/wall/delta/number`를 `GameWorld::RunFrame()`이 전달한다. Regular의 now는 동일한 절대 게임 시각이다. 정지 중에는 raw Kernel을 실행하지 않는다. 예약 시간이 이미 도달한 지연 낙하도 정지 중에 처리되지 않으며 재개한 프레임에 처리된다. 콘솔 `Step()`도 누적 절대 시각으로 같은 경로를 사용한다.

클라이언트의 전역 Kernel은 GameWorld를 소유하고, RawSurfaceWorld는 **미션 수명의 내부 Kernel**을 소유한다. 이는 이번 호스트 구성이다. 원본 전체 프로세스가 하나의 전역 Kernel에 등록되던 구조를 완전히 재현했다고 주장하지 않는다. 내부 Kernel은 실제 복원된 슬롯 순회/삭제/새 등록 순서를 사용한다. 월드를 해제할 때 Kernel과 예약 Regular를 host/raw 모듈보다 먼저 해제하여 다음 미션의 SID/프로세스/게임 시각과 섞이지 않게 한다.

표시 콜백은 호출할 때마다 현재 Renderer를 조회한다. 해상도 변경으로 장치가 교체되어도 이전 포인터를 저장하지 않는다. 카메라/viewport는 현재 월드 화면을 반영하며 전체 갱신으로 표시가 억제돼도 논리 changed를 보존한다. 기존 보행용 GroundGrid의 다리/받침 지면은 현재 raw 슬롯에서 다시 공급하므로 삭제한 다리가 옛 저장 목록 때문에 보행 가능 상태로 남지 않는다.

## 관찰 명령

기존 `--dump-world`와 `--ui-report`의 저장/임시 `object` 행은 유지한다. 현재 표면은 별도 행으로 기록한다.

| 행 | 필드 순서 |
|---|---|
| raw_world | game, frame number, 예약 프로세스 수, 표면 수, Graph 수, 비용, postPop 깊이, preDestroy 깊이, postDestroy 깊이, 다리 효과 요청 수, 공통 삭제 효과 요청 수 |
| raw_object | SID, 타입, 소유자, 프레임, x, y |

`--ui-script`의 `surface-delete`는 검사 전용이다. 인자는 `island:135,138`처럼 타입 이름과 좌표이며 bridge/island/noIsland만 허용한다. 실제 월드의 가상 삭제를 호출하므로 hard 거부·Graph·pre/post·예약·연결 정리가 모두 같은 경로를 사용한다. 일반 건설/전투 UI에는 노출하지 않는다.

## 검증

Release 빌드는 경고/오류 없이 통과했다. 최종 일반 CTest는 **261개·실패 0·전체 61.52초**(기존 257 + 새 월드 검사 4개)이다. [RawSurfaceWorldTests](../../cpppj/tests/RawSurfaceWorldTests.cpp)는 원본 파일 없이 두 판본에서 뒤섞인 초기 입력, GameClock 정지/재개, 정상 끝 칸 교체, hard 삭제 거부, 예약을 가진 미션 해제, 깊이/Graph/비용, 전체 표시 억제/장치 교체, 잘못된 입력과 중복 Load를 검사한다. 실제 다리 코드 표는 저장된 원본 fixture를 사용하고 SHP는 합성 입력이다.

[cpp_surface_world_smoke.py](../../tools/cpp_surface_world_smoke.py)는 실제 타입/코드/SHP로 아래 값을 확인했다.

| 판본/미션 | raw 객체 | 표면 | Graph | 연결 조각 |
|---|---:|---:|---:|---:|
| 10.78 The War Begins | 1665 | 1637 | 14 | 2 |
| 10.78 Save the Island | 2496 | 2413 | 8 | 31 |
| 10.78 TEST01 | 6729 | 6546 | 2 | 77 |
| CD The War Begins | 1665 | 1637 | 14 | 2 |
| CD Save the Island | 2496 | 2413 | 8 | 31 |

1-1의 클론 창에서는 화면을 이동해 (135,138) 받침을 표시하고, ESC 정지 중 검사 경계로 받침을 삭제했다. 20프레임 동안 다리/연결/예약 1개와 게임 시각이 보존되었다. 재개 후 (133,135)의 옛 다리 SID가 새 끝 칸으로 교체되고 (133,136) 연결 조각이 제거되며 예약은 0이 된다. 같은 화면 좌표의 **140×150픽셀 영역**이 실제로 바뀌었다. 메뉴 복귀/미션 재진입 때 받침·연결과 초기 개수가 복구된다. 캡처도 직접 확인했다.

기존 `cpp_world_smoke.py`의 10.78 네 초기 미션·CD 두 미션 대조와 1-1/TEST01 창의 선택·이동·정지·카메라·재진입도 통과했다. `cpp_window_smoke.py`·`cpp_renderer_smoke.py`·`cpp_menu_smoke.py`가 통과했고, 관련 `--verify` 감사 **15종**(bridgedecay, bridgeevent, bridgeeffects, bridgeconnect, islandpostpop, islandlifecycle, process, setframe, display, rawgraph, graphrebuild, destroygraph, postpop, pop, unpop)도 통과했다. 새 독립 x86 입력이 없어 누적 **131,542개**는 유지한다.

```powershell
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_world_smoke.py
python -X utf8 tools/cpp_surface_world_smoke.py
```

로그/결과는 Git 제외 `extracted/build-raw-world.log`·`ctest-raw-world.log`·`gui-raw-world-regression.log`·`gui-raw-cpp_*-smoke.log`·`audit-raw-world-*.log`·`cpp-surface-world-smoke/report.json`·`states.tsv`·`before.bmp`·`after.bmp`에 있다. 창 스모크는 순차 실행하며 허용된 options.cfg/fullscreenStateFile.dat를 복구한다. 두 원본 디렉터리의 전체 파일 해시와 존재 상태가 유지됐다. AGENTS.md·dotnetpj·dotnet 인계는 수정하지 않았으며 커밋/푸시하지 않았다.

## 남은 범위

표면은 현재 raw 상태로 표시하지만 건물·walker의 선택/이동은 기존 어댑터다. fringe 절벽과 본섬 생성/변형 선택도 기존 지형 어댑터다. 현재 화면의 깊이 표·그림자·애니메이션 전체가 원본과 일치한다는 검증은 하지 않았다. 저장 `object` 행은 삭제된 raw 다리의 옛 입력을 계속 포함할 수 있으므로 현재 표면 판단에는 raw_object를 사용한다.

다리 파편/소리와 공통 삭제 UI/보상 요청은 진단 카운터 경계다. walker 가상 낙하는 경계만 연결했으며 임시 walker가 raw 풀에 없어 실제 유닛 낙하를 재생하지 않는다. 네트워크 Transmit은 실제 전송하지 않는다. 받침 삭제 뒤 남는 종유석/noIsland와 건물의 전체 붕괴/낙하 애니메이션도 복원 완료가 아니다. 이번 창 검증은 삭제·끝 칸 교체의 정적 픽셀 변화이며 원본 게임의 전체 화면 픽셀 대조나 낙하 애니메이션 검증이 아니다.

다음은 비표면 객체의 raw 전환과 애니메이션 프로세스·walker 낙하·파편/소리 출력이다. 이어서 다리 배치/Construction·건설·경제·전투·AI·승패를 복원한다. 앞 단계 장시간 변이 검사와 미실행 변이 검사는 계속 인계하며 미션 완주는 아직 불가능하다.
