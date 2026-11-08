# C++ 현재 ProcessForm writer·mutable 송신과 Native Path 복원

2026-11-08 / 단위1177 / HJOW-Athlon / 표시·제품11.01. M6-n의 n8b 현재 헤더·실제 직렬화·같은 송신 예약을 연결하고, 추가 감사에서 발견한 Native Path 본문 차이를 교정했다. 전체 Host·자원 파생/NoWorld caller와 M6-l/n은 미완료다.

## 현재 헤더와 실제 writer

RawGameAuxState가 Gun/Flyer/SunAviary/WindAviary/RainAviary/BombTimer/Path의7개 현재 헤더를 장기로 소유한다. BombTimer00544c98과 Path00544c50의 초기 길이는 실제 DWORD0이며, 최소4를 비교한 뒤에만 하위WORD를 쓴다. Session/World 대여·반납으로 초기화하지 않는다.

Storage.SerializeProcessForm은 실제 Kernel 객체의 현재 writer를 선택한다. 지원하는14 concrete 종류는 Dais, Construction, Gun, 세 Aviary, Flyer, BombTimer, Path, GunAnimation, CannonFire, ArcherAnimation, CrossbowAnimation, IceCannonFire다. 다섯 animation의 OnAttached는 현재 Form/Parent 두DWORD 동기화만 한다는 기존 본문을 확인했다. 미확인 파생을 일반0이나 빈 writer로 가장하지 않는다.

Native Session·Asset/Gun·Archer/Animation·Path와 NoWorld Host의 기본 writer는 같은2,048BYTE 예약을 사용한다. 기존 typed 호출은 지역 capacity/초기 tail/const 관측을 유지한다. writer 반환 길이와 wire 선언WORD를 같은 값으로 가정하지 않으며, 중간 복사·const_cast나 미작성 tail 초기화를 추가하지 않는다.

## 실제 mutable 송신과 observer

Native ProcessForm의 최종 transmit와 공통 의존 Form caller를 실제 Storage.BroadcastProcessForm에 연결했다. 현재 대상별 Player→연결NULL→local제외 뒤 선택한 실제 Source를 보유하고 같은 전체 예약을 가상 송신에 넘긴다. 실제 backend가 자기 관문 뒤 observer를 한 번 호출한 후 현재WORD를 읽는 기존1175 경계를 사용한다. 앞 대상의 변경은 같은 예약에서 다음 대상에도 남는다.

Path30/Compressed7a/BombTimer7d의 순수 observer40a660/40b2a0/40b390를 Client 장기 기본표에 설치했다. Source 생성 경로가 같은 공용 installer를 사용하고 Session lease 반납 뒤에도 기본 observer가 남는다. Native7d의 Session 등록도 같은 실제 observer를 사용해 선행 임의 Finalize를 없앴다. typed7d의 기존 NULL observer/선행 Finalize는 그대로다. 이 연결의 다중 peer 동작을 새 실행 테스트로 확인한 것은 아니다.

## Native Path 추가 발견과 교정

실제 diff 감사에서 기존 typed Path helper의 Form0/Parent0 선행 거부와 부모 genus 인자 선평가가 Native 새 분배로 들어온 것을 발견했다. 원문4895b0에는 이 자격 검사가 없고, 같은 패킷의 현재 부모WORD를 고정 prefix 뒤에 읽는다. 원시 n8b1/b2를 유지하고 별도 Native WriteCurrent와 실제 Native Session caller를 추가 교정했다.

1. df590 message/protocol→unsigned DWORD 최소4→WORD, Form/Parent WORD를 쓴다.
2. 현재 nVert와 행1f/24 진단 정상 복귀 뒤 첫 좌표·시각·helper/tail을 순서대로 쓴다. 시작 좌표를 다시 읽고 packet+0d WORD를0으로 쓴다.
3. 그 뒤 현재packet+6 WORD로 parent/TYPE+ec의 direct genus를 한 번 선택한다. Pool/TYPE의 물리 경계는 실제 선택 위치에 둔다.
4. 압축 분기는 현재 nVert·행1f/29 진단·초기/각 point·각 run 로그를 유지한다. DWORD 거리 감김 뒤 signed 양수만 처리하며 byte store 뒤 현재WORD를 증가한다.
5. 직접 좌표는 이전signedWORD 주소 저장→현재WORD 증가→진단/좌표 읽기→store 순서다. 마지막은 현재signedWORD+24다.

루프/진단마다 현재 nVert를 읽고 진단 복귀 뒤 count를 자격 검사하지 않는다. 실제 접근이 이미지/송신 예약 밖에 이르는 순간 host 물리 경계에서 멈추며 앞선 쓰기를 원복하지 않는다. ftol은 masked signed64 하위DWORD 모델, 거리 연산은 DWORD 감김을 사용한다. 기존 typed Write/Truncate의 본문·자격 검사·예외 정책은 유지한다. 로그는 기존 Read와 같은 현재 PathTraceLevel()/Client TraceGraph를 사용한다.

## 근거와 적용

마지막 정밀 디컴파일은 클라이언트2026-11-03/R2026-11-07이며 모두 현재 HJOW-Athlon이다. 생성 원문·원본/CD·기대자료를 수정하지 않았다.

| 층 | 원시 경로 | 범위 |
| --- | --- | --- |
| n8b1 | `extracted/cpp-m6-resource-process-writers-prep/` b1.before/after |20파일/79추가25삭제 |
| n8b2 | 같은 경로 b2.before/after |6파일/34추가16삭제 |
| Native Path | `extracted/cpp-m6-native-path-write-fix/` |4파일/74추가7삭제/patch3162dde9cdbc66a02aae2ad5b70b0ca5f07b6b533fceb5445473e9f67f39480f |

기반 writer의19함수 C/PE 집합과 추가 Native Path의9함수 집합을 구별한다. 두 집합의 겹친 함수를 중복해 고유 함수 수로 세지 않는다. 현재 헤더의7쌍/두DWORD는 보호 PE9전역 자료로 확인했다. 모든 inclusive range/함수표·원문·PE hash는 해당 원시와 보고서 nativeEvidence에 보존했다.

현재1176의 장기14F/Editor 반납·메타 표시 본문을 보존해 최소 hunk로 병합했다. 실제 고유 제품 변경은22파일이며 각 적용 전후와 동결/최종hash는 `extracted/cpp-resume-1177/`에 있다.

## 빌드와 허용 비MP 확인

NetstormCpp/NetstormRoot Release/x64 최종0·경고/오류0이다. 각 실제 빌드·게임 시도는 보고서의 builds/liveAttempts에 보존한다.

| 미션 | 범위 | 결과 |
| --- | --- | --- |
| tutorial1 | 정상 UI 다리·실제 서버 객체/건설 요청 증가·90frame | 통과/정상종료0 |
| tutorial4 | 정상 UI 윈드 제네레이터·실제 서버 객체/요청 증가·600frame | 통과/정상종료0 |
| thewarbegins | 캠페인1-1 정상 진입·90frame·게임 시각 증가 | 통과/정상종료0 |

격리1920×1080/영어/무음에서 현재 상태/TSV/PNG를 관측하고 정상 입력만 사용했다. 보호2800파일·설치 사본·EXE·동결 소스가 불변이다. 새 MP·단위·패킷·통신/ZIP 검사와 포획·희생·특정 적 공격·승리·공중3종 전용 검사는 실행하지 않았다. 이 세 사례는 전체34미션 회귀나 전체 원본 동일성을 뜻하지 않는다.

| 실행물 | 바이트 | SHA-256 |
| --- | ---: | --- |
| Netstorm.exe | 13,448,192 | `d893c312147aae8110a540491a5ca2b1a5f7ee7074bc173400079156562fb0a0` |
| R.exe | 677,376 | `e0eaca00d0f61ff545d799dc552c078498a92de73c66bb571b1372727bf0f71b` |

## 남은 범위

전체 M6-n은 미완료다. Host 생성/부착/삭제/Regular 가상 사건, 자원 파생과 실제 NoWorld caller, debugBurst/전수 마감이 남는다. JoinTestBattle도 장기 NoWorld 준비/snapshot과 진단 child를 별도로 복원해야 한다. 준비 사본은 제품 완료로 세지 않는다.

현재14개 태그와 무태그74개를 유지한다. 기존 태그의 기본 전역/장기 observer/같은 F 대여반납/실제 Source·직접 송신·Fail·live/pending/Node큐 수명까지 닫힌 수와 구별한다. 전범위와 실제 device/리스너 수명이 닫히기 전 지연 큐를 활성화하지 않는다. M7 다섯 항목·M8을 마친 뒤 K1~K5·S1~S4를 계속한다.

현행 [보고서](../../cpppj/process-form-native-writers-20261108.json)·[M6 감사](cpp-lan-remaining-audit-20261108.md)·[LEFT_JOBS](../../LEFT_JOBS.md)를 따른다. 전체 `/goal`은 진행 중이다.
