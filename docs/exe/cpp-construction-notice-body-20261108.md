# 1169 — M6-c1 건설 통지의 lazy 본문과 현재 owner

2026-11-08 / HJOW-Athlon. 이전 CPP1168/5318a9b2, 제품11.01이다. 기존 Client 정밀2026-11-03·R2026-11-07/같은 호스트의 C/PE를 사용했으며 생성 소스를 바꾸지 않았다.

ConstructionPacketView는 같은 BYTE span의 필드를 실제 소비할 때 읽는다. 전체 typed 통지로 변환하거나19개 SID를 선검사하지 않는다. SID는27+2*i의 비정렬 WORD를 decoder가 요청할 때 읽는다. 사용하지 않는 시각/첫SID도 입구에서 읽지 않으며, 물리 범위 오류는 실제 접근 위치에서 발생해 앞서 끝난 Clear·이력·SP·조각 효과를 취소하지 않는다.

RawConstructionNotice::ActualHandle은4441b0처럼 진입 때 TYPE 레코드 주소·argument/direction을 보관하고 Clear 뒤 같은 TYPE의 현재 flags를 읽는다. packet TYPE가 바뀌어도 기존 선택 레코드를 다른 TYPE로 바꾸지 않는다. 현재 로컬 owner와 같은 카메라 최근5칸 이력, Place 전의 고정 scalar/count와 lazy SID pointer를 구별했다. Place 뒤 선택된 Builder/현재boss 분기에서만 첫SID/시각을 읽는다. 사제 선택·현재 PriestType contained 제거·재시도 좌표·환불/가상삭제·자동 시작의 재조회와 최종 refresh 순서를 유지한다.

RawConstructionPlace::ActualPlace는442c80처럼 TYPE 선택→player9 진단→decoder 생성→비용→각 현재 SID의 Take/isVoid 진단·부분 쓰기/HP/frame/WORD·품질·Owner/Pop→마지막 개수 진단 순서다. decoder를 미리 전수 순회하지 않고 declaredCount를 반복 상한으로 쓰지 않는다. 같은 SidPool 물리 슬롯에 접근하되 free/예약 선거부를 추가하지 않는다. 호스트 풀 바깥 접근은 여전히 실제 접근 시 예외다. 비용 비교의 NaN shortfall과 PE의 별도1/2 Pop 플래그→41h/2, 품질의 signed char/수명 비트도 대조했다.

현재 localPlayer/PlayerKnown/다리·받침/Dais/Priest TYPE를 실제 owner에서 읽는다. 패턴8칸은 현재 piece·Bridge/Island/NoIsland·Gun의Thunder/Rain·GameAux의Wind·기존 AI가 빌리는 마지막 칸을 decoder 생성 때 읽는다. 52f960은 활성 writer가 없고 기존 Bridge/Decay/Editor의 같은 kBridgeCrackLife=5 owner를 빌린다. 실제 storage AssertDisplay로 player9/isVoid/후행count와 품질 조회 진단을 연결했다. RawCampaignConstructionBindings::ReceiveNotice와 actualPlace 어댑터는 같은 World/Pool/TYPE를 사용한다.

기존 typed Place/Handle 두 본문의 정규화 텍스트는 그대로다. 공통 Money/AddMoney/Refund/표면 통지의 현재 owner 조회는 보완했으며 독립 typed 입력의 공유 포인터가 없으면 기존 값형 필드를 사용한다. 새 검사/기대 TSV/fixture 수정0이다.

**이 단위는 본문·owner 부품 완료다.** 실제3fh 등록/Session lease·호출별 지역512BYTE의 같은 버퍼로 방송 뒤 직접 처리·대상별 mutable 송신은M6-c2, outgoing20SID의 후행 진단/부분 Create는c3에 남는다. ActualHandle의 새 입구가 있다는 사실을 실제 caller 연결 완료로 세지 않는다. 일반 비MP Confirm까지 연결되는c2 뒤에는 허용 범위의 단순 미션/건설 집중 확인 필요성을 검토한다. MP/포획/희생/공격/승리 재현은 제외한다.

최종 NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 이번 새 C++단위/MP패킷/게임/통신/ZIP 실행0이며 제품 실행 동작 검증이 아니다. 원문11함수 inclusive PE/C·사본/적용 장부는 extracted/cpp-m6-construction-prep/, 빌드·보호 장부는 extracted/cpp-resume-1169/다. [제품·원문 보고서](../../cpppj/construction-notice-body-20261108.json)를 따른다.
