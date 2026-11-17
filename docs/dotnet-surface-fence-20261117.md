# dotnetpj 0.587 — 장기 펜스 수명 본체와 분배 연결

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 펜스 수명 본체를 이식했다. 원문 포팅·장기 결선·분배 라우팅을 연결한다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

원문 `o/RawFenceLifetime` 본체를 직역했다. Clear(4577f0)·Refresh(4578f0)·PreDestroy(457990)의 원본 순서·스냅샷·현재 raw 재조회를 보존한다. 최초 extra 검사·동맹 이웃 0/2/4/6 순서·네 이웃 스냅샷 뒤 자기 Unpop·현재 dead/타입 재확인 뒤 이웃 갱신·부모 Repop·일반 이웃 첫 두 칸 지연 Regular·Damageable 전처리 순서를 원문대로 둔다. 비정렬 float 좌표와 DWORD 조립을 원문 비트대로 읽는다.

장기 결선은 같은 장기 Pool과 Host 장치에 잇는다. 이웃·표식·연결선·재생성은 호스트 공급 장치(`BindFenceLifetimeDevices`)로 빌리며, 펜스 이웃 서브시스템 자체는 다음 단위에서 잇는다. Unpop·Repop은 같은 저장소 자산 해제·재등록에 잇고, 예약·부착은 같은 자원 Regular에 잇고, Damageable은 같은 피해 전처리에 잇는다. 분배 라우팅에서 펜스 전처리(0x00457990)를 장기 본체로 보낸다.

펜스 이웃·표식·연결선·재생성 서브시스템(`RawFenceNeighbors`·`RawFenceLine`·`RawFenceMarkerClear`·`RawFenceRebuild`·`RawFencePostPop`·`RawFenceRegular`)과 HostSession 별도 풀 전환·피해/해방 상위 결선은 잔여다.

## 검증

최종 **Core 신규6검사** 통과다. 효과 누락 거부·차단 시 이웃 미조회·동맹 네 방향 순서·죽음 시 Unpop/Repop 생략·앞 두 이웃 Regular·저장소 Emit 분배를 확인한다. 원본 fixture 기대값은 바꾸지 않았다. 포팅 과정에서 Refresh 스냅샷 4회와 Clear 4회를 합쳐 이웃 조회 8회를 관측으로 확정했다.

집중 127/127(펜스6·소용돌이벌룬6·아이스7·간헐천6·사제전처리6·거주Dude6·운반비행신전6·피해전처리6·피해후처리6·Outpost6·공통삭제6·표면수명6·자산상위6·자산삭제6·Process6·보상수명6·Content6·Resources6·물리미러6·Rift 보상HP4·벌룬본체6) 통과·RawSurface282/282 통과·Assets366/366 통과·Release 빌드 0경고·0오류다. Core 전체는 0.571~0.586과 같은 자료 부재 3건(Burning originalCD 2건·PlayerTechnology NETSTORM_DATA 1건)을 유지하며 전체 회귀를 다시 돌리지 않았다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치0/34·데모dn3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP1004행(L1 445·port 554), 대응cpp431단위·fixture319개다. 신규 3행을 추가하고 큰 파일 전체 기준/floor/reflected는 올리지 않았다. 미대응806단위 중 R4는 다음 단위에서 다시 센다. 다음은 개별 삭제 몸체·HostSession 별도 풀 전환·피해/해방 상위 결선이다.
