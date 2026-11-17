# WB04-E723: 공통 객체 수신의 실제 Session 연결

cpppj `RawCampaignSession.cpp` 기준 `225553df`의 `ReceiveSquid`(5148~5170)와 Carrier 등록의 2ch(5298~5301)를 현재 `RawCampaignSession`에 연결했다. 별도 세계나 Player를 만들지 않고 같은 Pool/Factory, 현재 TYPE, 표면 WORD·소유권, Core 기술 장부, Player 이름·경고 표식, World Pop, 로그와 경고 장부를 사용한다. `GameAuxState`에 원본 0054d4d8 검증 SID DWORD를 추가하고 기존 0054d4e4 이전 TYPE을 공유한다.

일반 타입의 독립 Pool에서 기술 장부의 유무에 따른 안내·Player 표식·배치를 확인하고, 실제 Tutorial 1 Session의 일반 Form 검증 패킷이 등록된 분배기와 같은 전역·로그를 사용하는지 확인했다. 공중 3종 전용 생성·행동, 사제 포획·희생·자연 승리는 실행하지 않는다. 실제 게임과 브라우저 창도 열지 않았다.

집중 검사 30건이 통과했다. 최초 검사에서는 시험용 Factory가 생성 타입을 슬롯에 기록하지 않아 기술 경고 분기를 타지 않았고, 실제 Factory의 타입 저장 계약을 시험 장치에 반영한 뒤 통과했다. 대응표 검사도 오류 0이다.

Node v26 전체 회귀 6,184건이 모두 통과했다. Node v22는 직전 E722에서 순차 전체 검사했고 현재 v26 주기는 1/8회다.

`AttachBaseAsset` 본체는 웹에 아직 없으므로 종속 자산 수신이 해당 지점에 도달하면 명시 오류다. 압축 7ah, 다른 Carrier 수신과 서버 응답 왕복, Native 및 앱 미션 연결도 남았다. 다음은 원본 등록 순서의 3ah 총 Process 수신 및 종속 자산 부착이다.
