# dotnetpj 0.605 — 그래프 per-Pop 결선과 장부 누적 입증

2026-11-17, VM-W11-Codex. cpppj 제품1314 기준에서 R4-2 적재 중 postPop의 다섯 번째 단계로 그래프 per-Pop 결선을 연결했다. 지형 Pop 호출이 같은 호출에서 영역 무효화·표면 추가와 ResetGraph까지 실행한다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`HostSession.WireTerrainPostPopGraph()`를 추가했다. 그래프 장치 뒤에 호출해야 하며, 월드 그래프 엔진에 현재 타입·진단을 빌린 뒤 지형 공용 후처리 분배기와 분배 Pop을 그래프 엔진과 함께 다시 묶는다. 섬·표면 접두와 다리 연결은 같은 세션 장치를 그대로 잇고, 건물 옆면은 요새 장치가 있을 때만 잇는다. 결선과 함께 공용 후처리의 그래프 관문(`TerrainBookkeeping.GraphsEnabled`)을 연다. 건물 옆면이 필수인 흐름은 인자로 거부할 수 있게 두었다. 근거는 `cpppj/src/client/RawCampaignWorldRuntime.cpp` 65·83행의 core.graph 공유 엔진과 `o/SquidPostPopPatch.cpp`의 per-Pop 그래프 관문이다. 시작 흐름(`ClientHostGame`)은 그래프 장치 직후에 결선한다.

## 검증

최종 **신규 4검사**(`HostSessionTerrainGraphTests`) 통과다. 섬·그래프 장치 순서를 어기면 거부하는지, 결선 뒤 표면 Pop이 그래프 번호 바이트(254)를 기록하고 같은 호출이 분배 Pop으로 남는지, 결선 없이 실행하면 번호 바이트가 그대로 남는지, 실제 지형 생성(`thewarbegins`)의 호출열·풀 바이트가 결선 뒤에도 결정적인지를 확인한다. 프레임 표는 실파일 값을 복사해 157번 받침 코드만 고정했으므로 이웃 탐색·그래프 엔진이 읽는 다른 타입 표는 실파일 값 그대로다. 사제·화신 경로 93건과 Client 전체 1263건이 함께 통과하므로, 공유 장부의 그래프 관문이 사제·화신 Pop에 영향을 주지 않음이 입증된다.

집중 Client 전체 1263/1263·Core(Fence|CppPortMap|Surface|Dais) 470/470·Assets 366/366 통과, Release 빌드 0경고·0오류다. L2는 `일치 0/34`, 데모 dn 3 미연결 유지다. 장부는 0.603의 분배 발화부터 누적되며, 덤프 대조(M1 판정)는 R4-2 잔여(농지 파이프라인·첫 지역·AI·정기 처리)와 함께 일괄 대조한다. 사용자 제외 플레이/MP 실행은 없다.

## 잔여 — 농지 파이프라인이 R6 캐논 파이프라인에 막힌 이유

R4-2 ②(농지 배치 파이프라인, 타입 121)의 실제 결선은 `MakeNormalTerrainFarmWorldHooks` → `MakeFortRandomPlacementHooks(pool, pipeline, factory, hp)`이며, 여기서 `pipeline`은 `RawCanonPlacementPipeline`(어셈블리: 앵커·허용·주변·관계·지형·기하·모양·충돌·미리보기·배치 10모듈)이다. 파이프라인 조립 단위(`o/RawCanonPlacementPipeline` 117줄, R6 I영역)는 미이식이라 세션 농지의 `MayPlace`에 올바른 값을 줄 수 없다. 스텁(항상 허용·항상 거부)은 오배치·미배치로 어긋나므로 두지 않았다. 파이프라인이 R6에서 이식되면 세션 결선(`RawFarmCluster`+`RawFortRandomPlacement`, 세션 공장·HP·owner·Unpop/Repop/Pop, 청크 지역·섬 owner)은 같은 모양으로 잇는다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs`·`ClientHostGame.cs` 범위)의 결선만 더해 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 L2 일괄 재측정과 격차별 R4-2 잔여 처리다.
