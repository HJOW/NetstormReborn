# WB04-E734 공유 Regular 수신

`7eh`를 28바이트로 등록하고 같은 Session의 Process Host에 연결했다. 수신기는 부모·시각·사건·payload·횟수를 복원한 다음 공유 타입으로 부착한다. 패킷의 미작성 SID는 사용하지 않고, 일반 공유 생성자의 깊이 진단과 현재 시각 덮어쓰기도 거치지 않는다. 수신한 float32 payload 원본 비트는 다음 산술 갱신 전까지 유지해 신호 NaN도 재송신할 때 바뀌지 않는다.

근거는 기준선 `225553df`의 `SquidProcess.cpp`와 `RawCampaignSession.cpp`이며 해당 본문의 기준선 이후 변경은 없다. 이 경로에 새 원본 fixture나 직접 cpppj 검사는 없다. JavaScript 할당 실패는 C++ `nothrow`의 null 반환과 달리 런타임 예외로 남는다.

실제 Host와 Tutorial 1 Session 분배기 검사에서 시각·횟수·부모 부착을 확인했고, 신호 NaN의 float32·double 비트 재송신을 대조했다. 처음 전체 회귀를 시작한 뒤 float32 변환의 신호 NaN 변경을 발견해 그 회귀를 중단하고 원본 비트 보존을 추가했다. 집중 검사 39건과 별도 writer 검사 17건이 통과했다.

Node v26.13.0 전체 회귀 6,208건을 통과했다. 대응표 오류는 0건이며 Node v22 전체 재검사 주기는 4/8회다.

다음은 `7ah` 압축 객체 수신과 나머지 Carrier 등록이다. 내부 로그인 왕복과 앱의 미션 연결은 아직 남아 있다.
