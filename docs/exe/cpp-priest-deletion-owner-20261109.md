# cpppj 1214 — 사제 삭제와 현재 보호막 조회

**구현·검증·인계 완료. 제품1214/11.01이며 전체 목표는 미완료다.**

사제의 실제 Pre4919b0을 같은 장기 Storage의16칸 사제 목록·보호막 조회4918e0·가상 삭제(flags0)·Carrier426890에 연결했다. Native Session의 삭제 분배와 Shield/낙하도 같은 장기 Pre/lookup을 빌린다. Session 주소나 별도 Player/World를 장기 본체에 보관하지 않는다.

## 원문 순서와 현재 자료

Native 전처리는 그때의 extra9가0이면 signed40ea00으로 모든 일치 DWORD를 제거한다. 남은 순서·높은 DWORD·stale 꼬리를 보존하고 음수 count는 읽기 없이 유지한다. 이어 실제 finder로 보호막을 조회해 SID1..capacity-1이면 해당 객체의 실제 가상 destroy(flags0)를 호출한 뒤 Carrier 부모를 실행한다. 앞선 목록 압축 뒤 lookup 실패는 그대로 남는다. Native 앞에는 TYPE158/상태9/Items/Carrier 사전 검사를 삽입하지 않으며, 독립 typed는 기존 Handles/Items/Carrier 검증 순서를 유지한다.

장기 lookup은 실제 주소 고정 TYPE 표와005412f4 DWORD를 빌린다. 좌표를0방향 절삭한 한 점에서 일반 finder의 현재 버킷/next/발자국을 쓰고, 각 후보의 BYTE TYPE를 전체 역할 DWORD와 비교한 뒤 현재 owner를 비교한다. 후보 free/dead/void/contained를 추가로 제외하지 않으며 finder의 extra8 제외만 유지한다. Shield 생성도 같은 lookup.Type()을 사용한다. 초기167은 PATCH005412f4/CD0051cbe0의 실제 PE4BYTE로 확인했다. 독립 typed/CD의 TYPE 사본 계약은 그대로다.

원문7함수 전체C/PE를 보관했다: PATCH4919b0/4918e0/40ea00/4b16d0/4b1810, CD40c330/40bf00. 직접 장기 연결한 고유 본문은 앞의 사제2함수이며 목록 압축·일반 finder·Carrier는 기존 본체를 공유한다. CD Pre는 extra35/36BYTE 슬롯·내장 목록 압축·40c0d0 경유·Carrier4e43b0이므로 PATCH 주소/슬롯으로 덮지 않는다.

마지막 정밀 디컴파일은 Client양판본 **2026-11-03 / HJOW-Athlon(192.168.0.94)**, R **2026-11-07 / 같은 호스트**이며 현재도 같다. 새 디컴파일/원문 변경0이다.

## 검증

Release/x64 제품2·집중5대상 종료0, 경고·오류0, 소스14개 고정. 기존 허용268개+신규 사제11개+기존 원문/typed8개 = **287/287** 통과했다. 기존 사제Pre 판본별2184행·보호막 판본별2976행의 세 판본과 typed 거부 계약을 확인했다. 송신 관찰이 포함된 별도 Kernel 통합 사례는 새 실행에서 제외했다.

신규는 본체 정체성·무초기화·정확한 목록 압축/부모 깊이·추상/매장의 선행 생략·현재 TYPE 발자국/전체 역할/owner·원래 후보 필터·실제 보호막 중첩 삭제가 Carrier보다 먼저 끝나는 순서·음수 count·lookup 실패 전 효과·TYPE변경 후 VT 분배를 확인한다. 실제 client SID와 원문 억제 인자를 사용하고 외부 네트워크 장치는 금지 callback이다. 이 검사는 포획/희생/특정 공격을 플레이해 재현한 결과가 아니다.

첫 빌드6656/287검사95653/Tutorial1251은 모두 통과했다. 마감 전 코드 검토에서 Native Session의 바깥 삭제 분배에 남은 TYPE158/167 고정 관문을 발견했다. 성공 원시를 pre-caller 접두부로 보존하고 삭제 전용 VT 분배와 같은 실제 사제 래퍼 검사를 보강한 뒤 최종 빌드/집중/GUI를 다시 확인했다. 기존 기대값을 약화하지 않았다.

같은 EXE에서 Tutorial4 정상UI 생산·건설 요청→TYPE75/owner1 생성·요청 증가→600프레임 대기/정상 종료와015 PNG를 확인했다. Client800×600 Edit TEST01↔Main2회/11관측도 종료0이다. 사제 삭제·포획/희생·특정 공격·승리·TestBattle·건물 완성을 실제 플레이로 검증한 것은 아니다. MP 단위/패킷/통신/게임/기록과 공중3종 전용 실행도0이다. 시작한 모든 빌드/검사/GUI 핸들은 실제 종료했다.

- Netstorm.exe 13900288 bytes, SHA-256 `7dc958936110eb209a8c46c856fd14e450d50d76382f3fc7b993aa7d9c5058b8`.
- R.exe 677376 bytes, SHA-256 `3e4c545e997f15b05ac94ebe899c9cf210a830c94aa2452eb753a1156eb60933`.
- 보호 원본/CD/fixture·AGENTS **2,800파일 불변**. 원본 실행·원격 push0.

## 남은 경계

실제 Pre/Post 분배 **81/83**, 다이스5076b8(44a4d0/448b60)·507950(44afd0/449580)의2쌍과 통지6이 남는다. 큰 비영 AI·다른 Form/Regular·다른 배치/Fort 상위 Finish·전수 Caller/owner 감사·M6-l/m·M7독립5건/M8·K/S/B의 상위16계획·등록34버그를 유지한다. 이번 연결을 알려진 버그 조사/수정 완료로 세지 않는다. 전체34진입 제품644/검증645·최신ZIP1092·최신 전체 솔루션 통과 없음도 유지한다.

[현행 인계](../../LEFT_JOBS.md)·[최종 보고서](../../cpppj/priest-deletion-owner-20261109.json), 원시 `extracted/cpp-resume-1214/`의 original-evidence/role-pe/static-audit/source-frozen/build/tests/GUI JSON이 근거다.
