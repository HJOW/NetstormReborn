# 1029 — 발사·회전 전체 수신과 기본 생성 공장

10.78의3bh/00466d30·3ch/00466b60과 공통00466cc0을 같은 Session/World/Kernel에 연결했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 원문/보호 PE는 정적으로 읽었으며 PATCH 수신 ABI를 CD와 섞지 않았다.

신규 free Form은 dirty ff 진단 뒤 패킷+9의 DWORD 부모 슬롯 TYPE를 읽는다. 발사 공장00468830은 현재00541174(71)이면 썬 디스크 발사기,00541258(128)이면 썬 캐논을 선택한다. 회전 공장00468900은128이면 썬 캐논,00541288(140)이면 크로스보우 공통 회전을 선택한다. 나머지는 원문 Gunanim.cpp의23b/248 진단 뒤 NULL 반환 경로이며 임의 다른 파생으로 대신하지 않는다.

네 기본 생성자는 부모/Form0·기본 inner VT·시간0.0·dirty ff와 실제 파생 VT만 기록한다. 썬 디스크 발사기는64/76/80의0과68/72의float180만 추가한다. 일반 생성자의 gun/target setter·현재 게임 시각·rest56 초기화·InitializeFire의 각도/공간 효과는 실행하지 않는다. 썬 캐논 발사는 기존 실제 RawCannonFire,회전은 RawGunAnimation,썬 디스크/크로스보우 회전은 각각 기존 RawArcherAnimation 본체를 사용한다.

공통 decoder는 부모DWORD를 별도 반환하고 dirty2/4/8의 gunWORD·targetWORD·시간8BYTE만 복사한다. observer나 dirty 소비가 없으며 미선택 바이트/파생 상태는 보존한다. 신규 객체는 현재35/36·명시 Form·flags10으로 기존 공통 부착을 사용한다. live Form은 bit1 진단 뒤 같은 PID의 공통 접두만 갱신하고 재부착/생성자/현재 시각 덮어쓰기를 하지 않는다. 미발견 PID도 실제 필드가 선택되지 않으면 원문처럼 저장 없이 반환하며 선택된 NULL 저장에서만 호스트 오류가 드러난다. 기존 Ice/Crossbow/Cannon/Gun/Archer 클래스의 같은 접두를 빌려 임시 객체를 만들지 않는다.

다섯 TYPE 전역의 보호 PE 초기값35/36/71/128/140을 장기 GameAux에 두고 수신 공장이 같은 현재 주소를 읽게 했다. **기존 일반 생성/조회 쪽35/36 상수와 부모 TYPE의 지역 시계 별칭은 아직 이 장기 값에 통합하기 전이며 다음 단위에서 대조한다.**3bh/3ch는 observer NULL/버전1/정책0/가변 길이 등록과 Editor 충돌/반납 목록을 공유한다. 애니메이션 writer의 전체 프로세스 분배 연결도 후속이다.

Release 빌드는 종료0·경고/오류0이다. 제품1029/11.01은12,024,832바이트, SHA-256은 `c8f7a56db7623be5cbcf6b088aae7bca4281520fc9df43157fd3f8e8e4b3fccd`다. 신규MP/단위/게임/통신 검사와 fixture 생성은0이며 보호2800파일 불변이다. 정적 복원/컴파일을 실제 공격·발사·LAN 플레이 검증으로 세지 않는다.

다음은 애니메이션 writer/일반 TYPE 별칭과 나머지 프로세스 decoder 감사·재동기화/Activate 부모다. PlayerKnown binding,메타/Outpost/checking5,한국어 후속과 Edit 시나리오 UI도 남아 있다.
