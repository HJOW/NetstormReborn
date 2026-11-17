# dotnetpj 0.603 — 섬 후처리 Prefix 실측 발화(R4-2 접두·연결 결선)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-2 적재 중 postPop의 세 번째 단계로 섬 후처리 Prefix 실측 발화를 연결했다. 지형 Pop 호출이 공간 등록 뒤 같은 호출에서 섬·표면 접두와 장부 집계를 실행한다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`HostSession.BuildIslandPostPopDevices`가 세션 소유 이웃 탐색기·다리 연결 스택(`RawSquidNeighbors`·`RawBridgeConnect`)과 지형 공용 후처리 분배기(`RawPostPop`)를 함께 연결한다. 이웃 탐색기는 같은 풀·해시·점유지·타입·프레임을 공유하고, 다리 연결은 세션 공장·소유자·지형 Pop·표면 통지를 같은 저장소에 잇는다. 프레임 지정은 R6 표시 경로에서 잇기 전까지 명시 거부로 두었고, 표면 통지는 요새 장치 미연결 때 같은 위치에서 명시 거부한다. 지형 Pop 몸체는 분배 Pop(표시·후처리 포함)으로 교체해 공간 등록·접두·장부 집계를 같은 호출에서 실행하며, 진단·검사·추적 장치를 빌리고 건물 옆면은 요새 장치가 있을 때만 잇는다. 근거는 `cpppj/src/client/RawCampaignWorldRuntime.cpp` 46행의 `world.Pop` 결선과 `o/SquidPostPop.cpp`·`o/Construction.cpp`·`o/Bridge.cpp`의 접두·연결 순서다.

## 검증

최종 **신규 3검사**(`HostSessionTerrainPostPopFireTests`) 통과다. 받침 표면 Pop의 H 분기가 3×3 머리·표면에 소유자 5를 찍고 연결 순회를 실행하며 섬·종유석 누락 상세를 남기는지, F 분기가 섬(94)을 만들어 등록하고 중첩 Pop을 순서대로 기록하는지, 실제 지형 생성(`thewarbegins` `BuildLiveTerrainFirst`)의 호출열·풀 바이트가 결정적인지를 확인한다. 프레임 표는 실파일 값을 복사해 157번 받침의 방향 글자만 고정했으므로 이웃 탐색이 읽는 다른 타입 표는 실파일 값 그대로다.

집중 Client 전체 1257/1257·Core(Fence|CppPortMap|Surface|Dais) 470/470·Assets 366/366 통과, Release 빌드 0경고·0오류다. L2는 `일치 0/34`, 데모 dn 3 미연결 유지다. 분배 발화로 슬롯 바이트가 바뀔 수 있으나 M1 판정은 R4-2 잔여(깊이 흐름·농지 파이프라인·그래프 번호·장부 대조·AI·첫 지역)와 함께 일괄 대조한다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs` 범위)의 장치만 더해 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 적재 깊이 흐름 연결(Prefix 프레임 선택 발화)과 농지 파이프라인·그래프 번호·장부 대조다.
