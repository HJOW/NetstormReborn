# dotnetpj 0.588 — 장기 펜스 이웃 검색과 동맹 결선

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 펜스 이웃 검색을 이식했다. 원문 포팅·장기 결선·finder 연결을 잇는다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

원문 `o/RawFenceNeighbors` 본체를 직역했다. 중심·중심 경계·최근접 선택·방향 이웃·사거리 판정·연결 개수 순서를 원문대로 둔다. 비정렬 DWORD·float 비트 조립과 CRT 절삭(`signed64` 낮은 32비트)을 원문대로 보존한다. finder 순서·동률 유지·한 방향 동맹·현재 extra 검사를 원문대로 둔다.

장기 결선은 같은 장기 Pool/Hash/TYPE에 잇는다. 사거리는 현재 TYPE+44(`OriginalTypeRecord.Range`, 0.588 신설)를 원문 선택 위치에서 읽고, 동맹은 `BindFenceAlliances`로 받은 실제 81 DWORD 읽기 함수에서 행·열만 읽는다. `FenceNeighbors()`는 Native 전송 뒤 동맹 바인딩 뒤에만 만든다. 펜스 수명 본체의 이웃 조회는 다음 단위에서 실제 이웃으로 잇는다.

표식·연결선·재생성·후처리·정기 서브시스템(`RawFenceMarkerClear`·`RawFenceLine`·`RawFenceRebuild`·`RawFencePostPop`·`RawFenceRegular`)과 HostSession 별도 풀 전환·피해/해방 상위 결선은 잔여다.

## 검증

최종 **Core 신규7검사** 통과다. 생성 계약·중심·검색 경계·최근접 선택·사거리 판정·finder 이웃·저장소 바인딩을 확인한다. 원본 fixture 기대값은 바꾸지 않았다. 포팅 과정에서 사거리 절삭 뒤 포함 판정(10.9 포함)과 1단계 해시 체인 구성을 관측으로 확정했다.

집중 134/134(펜스이웃7·펜스6·소용돌이벌룬6·아이스7·간헐천6·사제전처리6·거주Dude6·운반비행신전6·피해전처리6·피해후처리6·Outpost6·공통삭제6·표면수명6·자산상위6·자산삭제6·Process6·보상수명6·Content6·Resources6·물리미러6·Rift 보상HP4·벌룬본체6) 통과·RawSurface289/289 통과·Assets366/366 통과·Release 빌드 0경고·0오류다. Core 전체는 0.571~0.587과 같은 자료 부재 3건(Burning originalCD 2건·PlayerTechnology NETSTORM_DATA 1건)을 유지하며 전체 회귀를 다시 돌리지 않았다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치0/34·데모dn3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP1007행(L1 445·port 557), 대응cpp432단위·fixture319개다. 신규 3행을 추가하고 큰 파일 전체 기준/floor/reflected는 올리지 않았다. 미대응805단위 중 R4는 다음 단위에서 다시 센다. 다음은 펜스 수명 실제 이웃 결선·개별 삭제 몸체·HostSession 별도 풀 전환·피해/해방 상위 결선이다.
