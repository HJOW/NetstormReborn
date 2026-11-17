# dotnetpj 0.601 — 세션 섬 후처리 서비스(R4-2 postPop 첫 단계)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-2 적재 중 postPop의 첫 단계로 세션 섬 후처리 서비스를 연결했다. 지면·받침 소유자 스탬프의 실제 몸체를 세션 장치에 결선한다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`HostSession.BuildIslandPostPopDevices`가 세션 소유 `RawIslandPostPop`을 만든다. 풀·해시·표·프레임은 지형 장치와 같은 인스턴스를 쓰고, 섬 타입 집합은 세션 지형 값(`BridgeState`, 받침 94·없음 157·종유석 95·다리 82)을 쓴다. 후크는 세션 장기 몸체에 잇는다. 생성·소유자·Pop은 같은 저장소 공장·소유자·공간 등록 몸체(`TerrainPopObject`로 도장 후처리와 공유), 찾기는 같은 일반 finder, 표시 갱신은 세션 지형 표시, 누락은 세션 진단 기록(`IncompleteSupport`·`Assertion`·`Detail` 순서·문구 포함)에 둔다. 섬 연결 순회는 저장소 월드와 같은 미연결 계약으로 둔다(호스트 결선 R6).

## 검증

최종 **Client 신규 3검사**(`HostSessionIslandPostPopTests`) 통과다. 장치 순서·표 계약·미연결 거부, `SetSupportOwner` 3×3 머리·섬·종유석 소유자 전파와 표시 갱신, 누락 상세를 확인한다. 합성 장면 대조에서 섬 타입 선택(`HasIsland`)이 판정을 가름을 확인했고, 포팅 과정에서 finder 버킷 단계(레벨 1은 월드 좌표/2)와 슬롯 기록 폭(좌표 DWORD 4바이트)을 원문대로 교정했다. 공유 live 장면 조립에 서비스 설치를 넣어 기존 풀 전환 검사 9건과 함께 덤프 중립을 유지한다.

집중 Client(HostSession 계열)30/30 통과·Core(Fence|CppPortMap|Surface|Dais) 470/470 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Client 전체는 기존 11건 실패(사전 존재 환경 집합) 외에 통과한다. Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았다(마지막 전체 측정 0.589 직전 2842/2846에서 PortMap 중복 해소 뒤 자료 부재 3건만 남음). L2는 `일치 0/34`, 데모 dn 3 미연결 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs` 범위)의 빌더만 더해 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 지형 Pop 호출 연결(스탬프 실측 발화)이다.
