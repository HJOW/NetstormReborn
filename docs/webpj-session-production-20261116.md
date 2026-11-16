# Session의 실제 기술·Deck/생산·동기화 (WB04-E598)

`225553df`의 `RawCampaignSession.cpp`3135..3156을 같은 owner에
연결했다. E597의 내용물 생성기 뒤 기술→TotalMade→Deck→생산→
동기화까지이며, 다음3157 FactoryResearch/Upgrade/Input은 남았다.
해당 cpp와 사용한 기존 기술/Deck/생산/동기화의 기준선 뒤 변경은0이다.
Native 장기 Storage의 전체 Session 결선은 후속 Client 구간이다.

## 같은 실제 자료·원문 순서

- `RawPlayerTechnology`는 같은 Player·실제 사제 Carrier·내용물 생성기·
  현재 World TYPE를 빌린다. 이름 조회는 같은 GameAssets 로딩 시 만든
  원래188TYPE 표(`assets.loadedTypes`)에 기존 이름 바인딩을 사용한다.
  수정 가능한 World TYPE 이름을 별도 정답으로 쓰지 않는다. 기술 진단은
  원문 SessionMissing3135이고 삭제는 같은 World의 가상 삭제다.
- TotalMade는 같은 World 장부의 현재 세 집계와 동일 금지256DWORD를
  빌린다. 범위 진단은 원문3140이며 생산의 허용 조회도 같은 객체다.
- Deck은 같은 Player와 `ClientPlayerRecords.productionDeckMemory()`를
  사용한다. 최초 토큰0x30000000을 주되 장기 메모리의 현재 할당을
  재설정하지 않는다. 할당 입력은 호출 때 현재 서비스에 요구하며 누락을
  0 채움으로 대신하지 않는다. 실제352BYTE와 다섯 DWORD 초기화,
  Player+68 포인터·반납은 기존 몸체가 수행한다.
- 실제 생산은 같은 Core/Player/Carrier/Create/Deck/TotalMade/finder와
  현재 mission/FullDeck/무료 기술 TYPE/신전 상품 TYPE를 사용한다.
  값형 서비스의 production/effects는 원문처럼 복사한다.
- 동기화는 현재 templateOwner의 실제 Deck, 현재 기술 부모, Player의
  신전 DWORD+7c를 조회한다. AI+b0와 구별하고 전체 DWORD를 읽는다.
  FindKind는 같은 Carrier 검색의 SID에서 kind WORD만 반환한다.
  생성/삭제는 같은 내용물/World, 커서는 후설치 현재 포인터를 선택한다.
  원문의 stderr 진단은 브라우저 console.error로 연결한다.
- 선행 내용물 수명 완료 뒤 한 번 실행한다. 생성은 할당/생산·동기화
  동작을 시작하지 않는다. 부모는 World 프로세스 정리 뒤 실제 Deck
  소멸자를 호출하고 Player 대여를 마지막에 반납한다. 반납 뒤 자기
  closure는 오류다. 이 접두는 전체 InstallContents/Session 게시가 아니다.

## 근거·최초 실패·검증

새 전용 원본 관측은 없다. 보호 자산/실제 기존 모듈과 명시 할당/UI
입력 계약을 대조했다. 신규11건 첫 실행은9건 통과/2건 실패였다.
실제 기술 Clear/동기화 삭제가 읽는 선택 UI 입력이 빠져 원문1630에서
실패했다. 해당 검사에 선택 없음0을 기록하는 장치를 공급하고 실제
호출도 단언했다. 제품의 미연결 관문은 완화하지 않았다.

최종 신규11건을 포함한 **관련63건이 모두 통과**했다. Node v22.23.3·
v26.13.0 최종 전체는 각각 **5741건 통과, 실패·취소·건너뜀0**이다.
대응표1160행/오류0도 확인했다. Node24는 사용자 지시로 생략하며22/26 통과 시 호환을 가정한다.
새 fixture는 없고 직접 연결된 manifest 항목은0개라806항목을 유지했다.
Source 계약을 원본 관측으로 등재하지 않았다. JS130884행·검사768개,
대응표 상태와 port845/partial37/todo125·잔여162/34373 C++행·4-D36/51·
session0은 같다.

로그: `extracted/wb-e598-initial.log`, `wb-e598-new.log`,
`wb-e598-focused.log`, `wb-e598-port-map.log`, `wb-e598-node22.log`,
`wb-e598-node26.log`.

## 다음 시작점

InstallContents3157 FactoryResearch → FactoryUpgradeCommand →
ConnectFactoryUpgrade/ConnectFactoryInput을 완성한다. 이어 생성자1651의
Flare/Construction/Damageable/IceTower/CarrierSteal/BombPostPop/AcidZap/
AcidFence·이동/건물로 간다. World.BindRuntime1825·상위 Runtime 공개·
Load/Login·프레임·L2/L3는 여전히 남았다.

cpppj/보호 파일 수정·디컴파일·게임/브라우저 실행·자연 완주·실제 포획/
희생·특정 적 공격/승리·공중3종 전용 관측은 하지 않았다.
실제 미션은 아직 플레이할 수 없다.
