# dotnetpj 0.602 — 지형 Pop 호출 기록 연결(R4-2 호출 측정)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-2 적재 중 postPop의 두 번째 단계로 세션 지형 Pop 호출 기록을 연결했다. 도장·섬 후처리가 공유하는 `TerrainPopObject` 몸체가 호출될 때마다 호출 원문(번호·비트 단위 좌표·플래그·타입·결과·섬 후처리 연결 여부)을 세션 기록에 남긴다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`HostSession.TerrainPopCalls` 기록과 `FormatTerrainPopCall` 형식을 추가했다. 중립 호출(장치 미연결)은 `neutral`로, 실제 등록은 `RawPopResult` 이름(`Registered` 등)으로 남기며, 섬 후처리 장치 연결 여부는 `island:missing`·`island:ready`로 함께 둔다. 기록은 별도 목록에만 쓰고 풀 바이트·덤프에는 영향을 주지 않는다. 근거는 `cpppj/src/client/RawCampaignWorldRuntime.cpp` 46행의 `world.Pop` 결선이며, 스탬프 실측 발화(소유자 스탬프·농지 타입·그래프 번호·장부 집계)의 측정 바탕이다. 섬 후처리 `Prefix` 실측 발화와 연결 순회 결선은 다음 단위에서 잇는다.

## 검증

최종 **신규 4검사**(`HostSessionTerrainPopCallTests`) 통과다. 중립 호출 기록·실제 섬 Pop 등록 기록·섬 장치 연결 뒤 준비 상태·실제 지형 생성(`thewarbegins` `BuildLiveTerrainFirst`) 호출열의 결정성과 풀 바이트 동일을 확인한다. 공유 live 장면 조립에 기록 설치를 넣어 기존 풀 전환 검사와 함께 덤프 중립을 유지한다.

집중 Host 154/154 통과·Core(Fence|CppPortMap|Surface|Dais) 470/470 통과·Assets 366/366 통과·Client 전체 1254/1254 통과·Release 빌드 0경고·0오류다. L2는 `일치 0/34`, 데모 dn 3 미연결 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs` 범위)의 몸체만 더해 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 섬 후처리 `Prefix` 실측 발화(스탬프·연결 순회 결선)이다.
