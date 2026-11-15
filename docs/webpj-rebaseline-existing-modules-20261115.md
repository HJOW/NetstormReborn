# 재기준선 신규 행 중 기존 웹 부품 검토 (WB04-E514)

E513에서 보수적으로 `todo`로 등록한 107개 중 이름이 같은 웹 모듈이 있는
9개를 기준선 `225553df`의 C++ 마지막 변경, 기존 단위 문서와 시험에 대조했다.
8개는 부품 이식 `port`, 1개는 일부만 이식한 `partial`로 기록한다.
기준선 소스 자체를 새로 옮기거나 원본 게임을 실행한 작업은 아니다.

| 단위 | 판정 근거 | 남은 연결 |
| --- | --- | --- |
| `ClientHelpWord` | E335, 마지막 변경 `c551d159`: 캐시·ASCII 대소문자 비교·설정 교체; `client-local-state-globals.test.mjs`의 cpppj 7건과 웹 계약 | Client·도움말 소비자(4-D/8) |
| `ClientTimeSource` | E336, `c91d9b84`: 웹 호스트 시계와 고정 프레임 시각·DWORD 감김·time32 경계; `client-sp-globals.test.mjs` | C++ 프로세스 전역 대신 Client 인스턴스를 공유; 실제 프레임 조립(4-D) |
| `SystemNewsTime` | E455, `8efc76eb`: 공유 키·시각과 Config 표현식; `system-news-time.test.mjs` | 설정 저장·채팅 소유자(8), 외부 서버 접속 제외 |
| `RawClockSynchronization` | E335, `899c934d`: 70표본·카운터·기한 저장 구조만 이식; `client-local-state-globals.test.mjs` | **등록·Attach·Receive·Poll 본체와 단독 도달 검증이 남아 partial**(4-D) |
| `RawDaisDeletion` | E368, `21bb2550`: 다이스·제단 전후처리·타입 조회·Form 완료; `dais-deletion.test.mjs`와 `altar-sacrifice.test.mjs`, 원본 관측 | 실제 소유자와 공통 후처리 통합(4-D/7) |
| `RawFactoryRegular` | E255, `3c5f5092`: 정기 사건과 현재 레벨·난수·업그레이드; `asset-regular.test.mjs`, 원본 656행 | Session 장치(4-D) |
| `RawPacketCopy` | E335, `a76499e3`: observer 후 길이 재조회·앞방향 복사·소유 토큰; `client-local-state-globals.test.mjs` | 실제 Session 소비자(4-D); 등록표와 분배기는 같은 `registration` 계약 |
| `RawResidenceRegular` | E255, `ae4448e1`: 거주 구역 정기·Dude 생성; `asset-regular.test.mjs`, 원본 261·864행 | Session 장치(4-D) |
| `RawRitualRegular` | E367/E368, `21bb2550`: 룬·다이스·제단 정기 분기; `dais-regular.test.mjs`·`altar-sacrifice.test.mjs`, 원본 관측 | 실제 의식·장치(4-D/7) |

집중 검사 142건과 대응표 검사가 통과했다. 대상 1,004개 중 port 776·partial
36·todo 192이며, `port`는 부품 검증 상태이지 게임 Session에 연결됐다는 뜻이
아니다. 세부 원본 관측과 최초 실패·변형 검사는 각 기존 단위 문서에 보존한다.
Node 22·26 전체 회귀 검사를 수행했다. 다음은 E498~E502의 나머지 신규 대상과
4-D 선행 관계를 대조하며 실제 조립을 진행하는 것이다.
