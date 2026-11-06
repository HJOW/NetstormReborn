# 커서 취소의 현재 Form Factory 연결 — 단위969

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 원본·정밀 산출물은 읽기만 했다. 우선순위4의 실제 커서 취소 연결을 교정한다.

## 발견과 원문 근거

실제 `RawCampaignSession::ConnectCursor`는 취소 대상의 TYPE가6일 때만 Form으로 분리하고, 복귀에는 새 `SquidForms(core.pool).Pop`을 만들었다. 이 독립 객체에는 World의 진단 장치와 공유 `bookkeeping.depth`가 없다. 기존 공통 Form Pop 부모와 Factory 대여가 구현되어 있어도 이 실제 입구가 우회했다.

원문 PATCH `00446e70`은 source의 현재 VT+48(비void인 경우)과 현재 VT+4c를 호출한다. 네 Form 생성자1/2/3/6의 PATCH 표 `00512110/00512098/00512188/00512208`은 +48=`004ae140`, +4c=`004ac560`, +20=`004ac1d0`을 공유한다. CD 표 `00505a08/00505990/00505a80/00505b00`도 각각 `004af270/004abfb0/004abab0`을 공유한다. 이 정적 확인으로 CD 전체 커서 이관을 주장하지 않는다.

실제 커서의 두 경계를 현재 VT로 분류하고 같은 World의 `Factory.UnpopForm/PopForm`에 연결했다. Form TYPE 번호가 바뀌어도 같은 가상 몸체를 사용하며, 자산은 기존 공간 Unpop/Pop 경로를 유지한다. 공통 부모 자체나 원문 진단 조건을 바꾸지 않았다.

## 회귀 검사와 범위

창을 띄우지 않는 실제 CampaignSession을 적재해 공개 `Cursor.CancelForState`를 호출한다. 원본 Fort/TYPE는 읽기만 한다. 네 실제 Form 생성자와 TYPE6/7/63, void 유무를 조합한24입력이다. source 부모는 실제 free 슬롯으로 지정하고, preview만 실제 ContentForm 삭제로 반납한다. bare 생성 Form은 외부 PID/자원이 없으므로 원래 void 바이트를 복원해 풀에 직접 반납한다.

공통 Pop의 `!xmitJustCreatedFlag` 진단이 상태의 void 쓰기 뒤·공유 깊이 증가 전에 들어오는지 확인한다. 진단 정상 복귀가 상태/현재 VT/공유 깊이를 바꾸고, 같은 Factory의 빈 Form 후처리가 DWORD 감김까지 반납하는지 확인한다. 이어 preview 삭제·커서 목록/lock 정리도 확인한다. 비서버 권한만 선택하며 실제 패킷/네트워크 경로는 호출하지 않는다.

첫 준비 실행은 source 부모0이 free가 아니라 종속 진단을 선택했고, bare ProcessForm2를 실제 삭제하려다 미등록 처리기에서 중단됐다. 검사 준비를 위 방식으로 교정했다. **제품 수정 전** 재실행은8입력 모두 공통 Pop 장치 미연결로 중단되어16단언 실패했다. 첫 실패와 준비 교정 원시는 `extracted/cpp-resume-969/focused-before*.log`, `test-preparation-note.txt`에 보존한다.

첫 수정후24입력은 검사에서 Storage 표시 진단만 바꿔 Session이 실제 대여하는 `services.ai.assertion`을 제공하지 못했다. 24입력/48단언 실패 원시를 보존하고 실제 Session 생성 전에 Services 진단을 제공하도록 준비를 교정했다. 제품의 공통 진단 선택이나 기대 부분효과는 완화하지 않았다.

인접 기존 타이머 검사는 독립 사본 `lifetimeIndex=1`만 바꾸고5초를 기대했다. `fc56410a / 20261101-71 CP`에서 실제 현재 전투 옵션 BYTE2를 연결한 뒤의 오래된 기대다. 원문 시간표10/5/1과 실제 `BattleOption(2)==0`을 확인해, 사본1과 달리 현재 Slow의10초를 기대하도록 교정했다. 제품 타이머 코드는 수정하지 않았다.

**Release4명령 모두 종료0·경고0·오류0**이다. 최초/준비 교정 전후 집중 대상과 최종 게임을 빌드했고, 이후 검사 준비만 교정한 최종 집중 빌드도 완료했다. 최종 고유5검사(새1/기존4)·실패0이며 새 검사24입력이 모두 통과했다. 메뉴 취소·현재 preview 삭제·UI 재연결·실제 현재 옵션의 반환 타이머를 함께 대조했다.

제품969/11.01의 EXE는11,357,696바이트·SHA256 `e8fd503549044a06d7841c30dc9dca7bf305cec6594f0a5348ceac5684568bdb`이다. 보호2796개/fixture976개 불변·새fixture0이며 다음 기준은 `extracted/cpp-resume-969/protected-final.json`이다. [단위 보고서](../../cpppj/cursor-form-factory-20261106.json)를 따른다. 솔루션83/checking잔여8은 불변이다. 새 Native/fixture, 실제 게임/GUI, MP/프로토콜, 공중3종 전용, 포획/희생/특정 적 공격/승리 검사는 실행하지 않는다. 마지막 실제 GUI는968이다. 전체 Form 삭제/외부 heap/Factory/우선순위4 완료로 확대하지 않는다.
