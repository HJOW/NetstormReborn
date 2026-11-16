# Session의 Fence 현재 공간·예약·수명 결선 (WB04-E612)

기준선 `225553df` 생성자1670의 `ConnectFence`1911..1965를 연결했다.
Session.cpp와 RawFence 계열 원본의 기준선 뒤 변경은0이다. Native 장기
소유자 대여는 실제 Client 결선에 남았으며 독립 typed 부품을 조립했다.

최근접·연결선→후처리→표식/포스 필드 생성·재생성→Regular·표식 삭제→
수명의 원문 생성 순서를 유지한다. 최근접은 같은 현재 TYPE의 range를
읽는188칸 getter 뷰와 같은81칸 동맹을 빌린다. 사거리 수치의 초기 사본을
만들지 않는다. 표식/연결선 TYPE도 같은 GameAux 역할을 소비 때 읽는다.

후처리·Regular·수명의 각 예약은 같은 미부착 Regular 한 개를 공유한다.
원문 unique_ptr 대입처럼 이전 미부착 예약을 교체하며, 같은 객체를
초기화/Host 부착한 뒤 효과 이후 현재 시각으로 event/count0을 쓴다.
누락/부착 실패는 앞선 예약 소비를 보존한 채 전파한다. 실제 Host를 먼저
비운 뒤 미부착 예약과 대여 closure를 닫는다.

가상 생성·표면 통지·실제 소유권 가상74·Pop, 후처리 공통 ValidateBase와
Damageable, 삭제의 DamageablePre, 공통 Regular 반환은 같은 현재 부모다.
수명 clearLine은 원문 mode0처럼 추가 owner/SID 인수를 쓰지 않는다.
재생성만 동맹 최근접을 요구하며 후처리/Regular의 일반 이웃을 이 관문으로
바꾸지 않는다. Teleport 타이머가 이미 참조하는 같은 fenceRebuild도 채웠다.

신규8건은 첫 실행에 모두 통과했다. 실제 Host의 같은 예약/현재 시계·
실패 전파, 현재 사거리/동맹·역할/테마, 실제 부모 분배와 반환을 확인했다.
기존 관련8파일을 포함한 집중30건도 통과했다. 기존 원본 fixture는
기존 부품 시험으로 재생했으며 새 fixture는0이다.

| fixture | 행 | SHA-256 |
| --- | ---: | --- |
| fence-postpop | 330 | `aafbea1ba70bc8485b858d5e4936ec0b8910030e83c5de9b4793d9ffdd708d8e` |
| fence-line | 99 | `f32d6fae769eef91a6b853b10da261baadb7b793c9370c0715c01d2bc39c0e4c` |
| fence-lifetime | 207 | `f3e5be8c131684bc679dba1fb7d27f431ace773495800e92ced8fd7ac856155b` |
| fence-create (재생성 포함) | 578 | `84c1247a20c3a7cc1321f0fa36b5a36553a13f1e01e3df41caa3444a1dc12fd1` |
| fence-marker-clear | 259 | `9dbf35a118c9b319062a3ef6a126142ab0e152c0b3951bc72fccff2a63294c27` |
| fence-neighbors | 1,904 | `79cbc909b122529f0738a4c40b9cce847c40484fc25d23388301fc509dc646a4` |
| fence-regular | 1,372 | `a87290020823e235540e7f38fe98cc43bafa363dd52edfc1995ccf79280dd9d4` |
| fence-event3 | 301 | `e6ae4858b56e961af2a66d9a7f6f09eddb2434cc6279b5792bcd11f79b0e0e59` |

Node v22.23.3·v26.13.0 전체는 각각5,884건 통과했고 실패·취소·건너뜀은0이다.
두 실행 exit0·대응표 오류0·UTF-8/구문/diff 검사를 확인했다.
웹 JS132,376줄/검사782파일·manifest806항목이며 port846/partial39/todo122·session0을 유지한다.
두 전체 명령은 메모리 경쟁을 줄이는 `--test-concurrency=4`를 사용한다.
Node24는 기존 사용자 지시로 생략한다.

실제 World의 가상 삭제/전체 Regular 분배·게시1825·Load/첫 저장 객체와
Native 장기 대여는 잔여다. cpppj의 실제 Session 포스 필드 색/픽셀 시험도
월드/표시 조립 이후 재대조한다. 자연 교전·포획/희생·공중3종 새 전용
실행 검사는 추가하지 않았다. 다음은1671 `ConnectGemReward`2020..2032와
1672..1679 Buried 후처리다. 보호 원본/cpppj 변경·디컴파일·게임/브라우저
실행은 하지 않았다. 검사 로그 접두는 `extracted/wb-e612-`이다.
