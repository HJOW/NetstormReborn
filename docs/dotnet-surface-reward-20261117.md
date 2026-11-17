# dotnetpj 0.571 — Native 장기 HP/보상·Content 수명 결합

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 Reward 장치·상태·본체와 ContentLifetime 장기 결합을 이식했다. 분배기의 Content 등록까지 잇는다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

실제 장기 보상 장치는 Native 전송 결합 뒤에만 설치한다. PATCH·네 장치(SP·editor·표시 갱신·지갑)·최초 보상 본체·빌린 물리 전에 거부하며 함수 값 사본 뒤 효과를 실행하지 않는다. 상태는 같은 장기 주소를 별칭으로 빌린다. fort/battle은 같은 OwnerMode 현재값을 읽고, 보상 비율·자원·약화·TYPE은 같은 GameAux 상자를 빌린다(0.570 너겟 포함, 0.571에서 보상 비율·자원·제단·레인/성장형 상자를 GameAux에 추가). 진단은 같은 장기 checking을 따른다.

보상 본체는 실제 최초 자산 owner 뒤에만 한 번 구성하고 World 반납 뒤에도 유지한다. VirtualCount(FortSaveVirtual Level)·현재 TYPE 배열/뷰 별칭·최초 로딩 정수 비용표는 미연결 잔여로 두고 생성 시 사본과 독립 상위 3비트 경로를 쓴다(Storage.Types가 List라 ArraySegment 물리 뷰를 직접 빌릴 수 없으므로 별도 배열 미러가 필요하다). 표시 갱신·HP 진단·지갑 처리는 실제 장치를 연결한다.

수명 본체는 같은 정렬·삭제·상태·보상을 한 번 연결한다. 네 효과는 Content 장치에서 빌리고 파생 밖 효과는 기본 삭제 훅으로 넘기며 지갑/SP는 같은 보상 본체가 처리한다. 분배기는 Native 뒤에 같은 장기 의존/Gump/Content 수명 Form을 결합한다.

파생 전용 삭제 몸체·HostSession 별도 풀 전환·피해/해방·삭제/배치 상위 결선은 잔여다. 기존 분배기 검사(0.569)는 Content 결합 조건에 맞춰 원천·Content·보상 결선을 포함하도록 갱신했다.

## 검증

최종 **Core 신규12/12** 통과다. 보상 6(전송 없는 설치 거부·중복/필수 누락/CD/빌린 물리 거부·SP 없는 상태 거부·공유 주소 별칭·원천 없는 본체 거부·재사용·수명 재사용·분배기 결합)과 기존 Content6·Process6·Resources6을 포함한 집중 24/24 통과, Rift 보상 HP 물리 뷰 계약(배열/뷰만 허용)도 유지한다. 원본 fixture 기대값은 바꾸지 않았다.

Assets366/366 통과·Release 빌드 0경고·0오류다. Core 전체는 2732/2736이며 실패 4건 중 1건(Rift List 거부)은 이번 수정으로 해소해 3건이 남는다. 남은 3건은 자료 부재다. Burning 2건(originalCD `d/dude.type` FileNotFound), PlayerTechnology 1건(NETSTORM_DATA 실제 자료 루트 필요). 제품 코드와 무관하며 0.568 기준선(2717/2718, 1건 NETSTORM_DATA)과 같은 부류다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치0/34·데모dn3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP979행(L1 445·port 529), 대응cpp418단위·fixture319개다. 과정 장치 5행을 추가하고 큰 파일 전체 기준/floor/reflected는 올리지 않았다. 미대응819단위 중 R4는133단위/13,564줄로 유지, 다음은 파생 전용 삭제 몸체·HostSession 별도 풀 전환·피해/해방·삭제/배치 상위 결선이다.
