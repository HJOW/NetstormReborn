# 실제 자식 반납 뒤 Session·고정뷰·게임 전역의 수명 (단위454)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 수행일은 **2026-10-23**이다. 기존 정밀 C·보호 PE는 읽기 전용으로 대조했으며 원본/CD·AGENTS.md를 바꾸지 않았다.

## 문제와 변경

원문 State 정리는 현재 AI·Player 링크·Kernel을 먼저 정리하고, 뒤의 위치에서 Pool·Core·Player와 전역을 갱신한다. 기존 C++ Session은 실제 자식 반납 증빙을 소비해도 마지막 일반 Gameplay 대여 소멸자가 PlayerKnown·커서·Editor를 다시 초기화했다. 이 쓰기는 원문 부모의 현재 전역과 다음 prepared 설치를 손상시킬 수 있었다.

Session 소멸자가 같은 실제 모듈의 **AI owner1~8 반환 증빙, 전체 Player 링크 반환 증빙, 단일 Kernel Clear 뒤 현재 처리기 반환 증빙**을 모두 확인하도록 연결했다. 하나라도 없으면 기존 일반 종료 정책을 유지한다. brain이나 Kernel이 비었다는 이유만으로 성공을 합성하지 않는다.

증빙이 모두 소비된 경우에만 비공개 `Use::WithdrawForState`가 현재 Session 공개를 닫고 최종 전역 초기화를 생략한다. 이 함수는 같은 공개 대상과 대여를 확인하며 Session만 호출할 수 있다. 외부 성공 setter나 전역 사본은 추가하지 않았다. 고정 Player 뷰·표면 대여는 여전히 전체 Impl과 실제 소비자 소멸 뒤에 반납된다. 그 뒤에만 Core0·Pool·Player 재할당을 진행할 수 있다.

이전 단일 Clear가 앞 슬롯에 남긴 실제 Regular는 장기 Host/Kernel에 그대로 남는다. Session 소멸에서 두 번째 Clear를 실행하거나 다른 대여의 프로세스를 지우지 않는다. 반납 증빙 조회에서 손상된 외부 필드를 발견하면 진단하고 기존 C++ 최종 정리를 계속한다. 이를 Native 부분 실패 후 재개 프로토콜의 구현으로 설명하지 않는다.

## 검증과 최초 실패

- Source1 build1: 종료0·경고/오류0,285.733초. 집중1은99개 중 신규3개 실패였다.
- 실패 중2개는 검사 장면이 장기 ProcessHost 대신 기존 raw Kernel 경로를 조립한 것이고,1개는 Native 후반 Player 초기화 앞의 `PlayerKnown=0` store를 생략한 것이다. 제품 관문·기대값·Native fixture를 유지하고 입력 조립과 원문 순서를 교정했다.
- Source2 build2: 종료0·경고/오류0,53.382초. 집중2는 **99고유/99관측·실패0, 신규5개 전부**다. 같은 실제 AI·9Deck·Kernel의 정상 반환, 여덟 owner 각각의 미소비 증빙, 링크/Kernel 미소비 증빙, 일반 경로의 기존 무효화, 실제 Regular 잔류와 고정뷰 반납을 확인했다.
- 같은 Source2 제품 SHA `8624fefbf9c1022aecff9e9733ff21c85de6fedd03ee5f0e236419961c6d2350`의 **전체 CTest3,096개·실패0, 메뉴 GUI2, 자연 Early2→Tutorial3**를 통과했다. 이전450의3,076개 실제 이름과 이후451~453의15개·이번5개를 현재 등록 소스의 이름 집합과 대조했다. 기존 검사 본문2파일의 전체 원시 바이트도 그대로 보존됐다.
- Early2는 원본 생산 타이머1, 상태·SP·단계 직접 주입 없이 A~I·회수75·최종SP3075·Tutorial3 첫 안내까지 통과했다. 초기 완료값1을 보존했으므로 영구 진행 저장0→1 검증은 아니다. 실제 화면3개도 육안 확인했으며 픽셀·음향의 원본 동일성으로 확대하지 않는다.
- 동결 소스·네 실행 파일·원본/CD 등 보호1,821파일은 각 실행 전후 동일했다. [실행 보고서](../../cpppj/state-runtime-retirement-20261026.json), 원시 `extracted/cpp-state-retirement-coordinator-454-{source2,build2,focused2,batch1}`를 본다. 집중1의 신규3실패와 최초 빌드·검사 로그도 보존했다.

## 다음 정리 등록표의 실제 대상

읽기 전용 PE 명령 대조에서 `00472830`의20포인터 표에 등록되는 정적 생성자 **9개**를 확인했다. 각 생성자는 `0044fb20`에 다음 용량을 넘기며, 정리 대상은 각 헤더 `+0x0c`의 현재 개수다. 표 포인터를0으로 만들거나 각 목록의 데이터 전체를 지우는 정리가 아니다.

| 실제 헤더 | 목록 | 생성 용량 |
| --- | --- | --- |
| 0055a4d0 | factories | 500 |
| 0055a4ec | dudePopulation | 30 |
| 0055a708 | outposts | 100 |
| 00592f80 | providers | 256 |
| 005950d8 | geysers | 100 |
| 005954c0 | priests | 16 |
| 0059a9d4 | residences | 50 |
| 005caf28 | multipleSelection | 30 |
| 005e1898 | temples | 20 |

[정적 감사 보고서](../../cpppj/state-sid-list-static-audit-20261026.json)와 원시 `extracted/cpp-state-retirement-coordinator-454-list-audit.json`에 실제 push/mov/call 바이트를 기록했다. 현행 Session은 사제·거주 구역·신전에 각각100칸을 잡으므로 실제16/50/20과 구별해야 한다. 선택 목록을 제외한 실제 목록/count의 장기 소유와20포인터 정리는 다음 구현 대상이다. 이 단위에서 이를 완료했다고 합산하지 않는다.

전체 Native cleanup/prefix·초기 HUD·자연 Edit 화면 설치·전체 미션 완주와 표시/음향 동일성은 계속 남아 있다.
