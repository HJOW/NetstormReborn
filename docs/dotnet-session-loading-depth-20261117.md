# dotnetpj 0.604 — 적재 깊이 흐름 연결(Prefix 프레임 선택 발화)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-2 적재 중 postPop의 네 번째 단계로 적재 깊이 흐름을 연결했다. 로더의 동일 깊이가 섬 후처리의 프레임 선택 관문까지 같은 값을 읽는다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

세션 섬 후처리 상태를 `HostSession` 소유 필드에 보관하고, `SharedLoadingDepth`에 세션 적재 깊이 상자를 빌렸다. `SetWorldLoadingDepth`가 바뀌면 상자 값이 함께 바뀌어, 적재 중 표면 Pop이 서·북 칸으로 프레임을 고르는 원문 블록이 발화한다. 깊이가 0이면 블록을 건너뛰어 요새 읽기 프레임이 그대로 남는다. 근거는 `o/Construction.cpp`의 적재 중 방향 선택과 `o/RawIslandPostPop`·`RawSurfaceStorage.State`의 깊이 공유 계약이다. 월드 적재 내부 깊이 통로(`BuildWorldLoad`의 읽기·쓰기 쌍, CD `ReadTerritory` 전용)는 이번 단위에서 건드리지 않았다.

## 검증

최종 **신규 2검사**(`HostSessionLoadingDepthTests`) 통과다. 깊이 1에서 서 C·북 C 이웃이 G 글자 프레임을 기록하고 같은 호출이 분배 Pop으로 남으며 F·H 분기를 타지 않는지, 깊이 0에서 공장 초기 프레임이 그대로 남으면서 같은 호출이 기록되는지를 확인한다. 프레임 표는 실파일 값을 복사해 157번 받침 코드만 고정했으므로 이웃 탐색이 읽는 다른 타입 표는 실파일 값 그대로다. 기존 실제 지형 생성 결정성 검사(0.602·0.603)와 함께 깊이 0 기본 흐름의 풀 바이트 중립을 유지한다.

집중 Client 전체 1259/1259·Core(Fence|CppPortMap|Surface|Dais) 470/470·Assets 366/366 통과, Release 빌드 0경고·0오류다. L2는 `일치 0/34`, 데모 dn 3 미연결 유지다. M1 판정은 R4-2 잔여(농지 파이프라인·그래프 번호·장부 대조·AI·첫 지역)와 함께 일괄 대조한다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs`·`HostSession.PriestWorld.cs` 범위)의 결선만 더해 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 농지 파이프라인과 그래프 번호·장부 대조다.
