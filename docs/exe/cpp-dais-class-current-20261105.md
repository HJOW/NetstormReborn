# 단위873 — 다이스·제단의 현재 가상 진입과 Player 쓰기 주소

PATCH의 다이스·제단·Rune 가상 분배가 표시 TYPE와 독립적인 실제 VT를 사용하도록 연결했다. 다이스 수명은 도달한 장치만 요구하고 실제 현재 미션/권한/로딩·가상 Count를 읽는다. 실제 포획/희생/해방 효과는 재현하지 않았다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**, 현재 호스트도 동일하며 산출물은 변경하지 않았다.

## 원문과 제품

PE에서 세 VT의 `+14/+18/+20/+5c/+80`을 읽었다. PATCH 다이스 `00507950`의 후처리/삭제후처리/Regular는 `00449320/00449580/0044b150`, 제단 `005076b8`은 `00448ad0/00448b60/0044a5e0`, Rune `005075d8+5c`는 `0044a3a0`이다. 원문 진입은 TYPE164/165/166을 사전에 검사하지 않는다. 따라서 PATCH의 Handles와 내부 Rune/Dais·Altar/Dais 분기를 실제 VT 기준으로 바꾸고, Session의 Rune 삭제 분배도 같은 기준을 사용한다. CD의 기존 TYPE/VT 계약과 명시 `Validate`의 기존 TYPE 사전 검사는 유지한다.

`RawDaisLifecycle`의 PATCH 생성 시 전체 효과 사전 요구를 없애고 실제 도달한 callable만 복사해 호출한다. flags0·억제 상태는 생성/표시/음향 장치를 요구하지 않는다. plain frame 쓰기도 PATCH 물리 슬롯을 사용한다. `suppressed/boss/count`는 실제 Session의 현재 globals/권한/VirtualCount로 연결하고 로딩도 기존 현재 getter로 읽는다. 독립 입력은 기존 상태와 실제 Dais Count로 돌아가며, 다른 VT를 Dais Count로 가장하지 않는다.

`00449320`은 가상 Count **앞에서 Player 쓰기 주소 전체**를 저장한다. CD `00481360`은 owner BYTE를 앞에서 저장하고, Count 뒤의 현재 Player 저장소에 쓴다. `DaisRegularState::GrantFlagSlot`으로 PATCH의 실제 BYTE 주소를 보관하고 CD는 저장한 owner로 현재 기록에 쓴다. 콜백이 owner와 기록 연결을 모두 바꾸는 경우에도 서로 다른 원문 순서를 보존한다. 후속 Player SID쌍 등록은 그 뒤 현재 owner를 읽는다.

정밀 C에 없는 PATCH `00449580`은 PE 기계어 `00449580..0044965b`를 직접 읽어 일치하는 현재 Player SID쌍만 지우고 `0044b9e0`로 tail jump함을 확인했다. 첫-return까지의 보조 출력은 다음 함수를 포함하므로 근거 파일은 별도의 bounded 출력이다. CD `00483670`의 정밀 C 부재도 기록했으며 새 정밀 C를 합성/수정하지 않았다.

제단/다이스 preDestroy `0044a4d0/0044afd0`의 표시 TYPE 사전 조건과 PATCH 할당 확인도 제거했다. 제단 Rune 정리 권한은 원문 `00540bc4`이므로 기존 server 대신 현재 `Boss()`를 읽는다. 이 파생 효과는 코드/정적/컴파일 근거이며 실제 희생/해방이나 사제 재현 검증으로 세지 않는다. 다른 Rune 번호/미션 상태 사본과 전체 Damageable 사전 경계는 별도 잔여다.

## 검증과 최초 실패

Release 세 번은 모두 종료0·경고/오류0이며 **163.819초 / 23.527초 / 24.382초**다. 첫 집중 검사에서 새4개는 통과했으나 기존 수명 원문 재생2개/2단언이 실패했다. 기존 검사는 PATCH의 `validateBase`에서 원문 경계를 관측했지만 `RawCarrierPostPop`의 HEAD는 이미 PATCH Validate를 생략했다. 같은 관측 callback을 실제 `base`에 연결해 기존 trace/raw/Player 기대값을 그대로 보존했다. 제품을 맞춰 기대값을 바꾸지 않았다. 두 번째 빌드 후 +14 추가 대조에서 발견한 server/boss 한 곳은 마지막 제품 빌드 전에 교정했으며 두 번째 검사 실행은 하지 않았다.

최종 집중 검사는 **고유18개/7묶음(새4) 전부 통과**했다. 새 부품3개는 VT 함수 부류, 가상 Count 전후 판본별 Player 주소/owner, 미배치 슬롯·도달 장치와 공통 장치 부재 전의 부분 SID쌍 쓰기를 확인한다. 새 실제 Session1개는 세 VT의 무효과 Regular 반환과 지원 조회만 확인하고 합성 슬롯을 직접 반납했다. 틱·예약·가상 삭제·의식·명령/패킷은0이다.

기존 수명 원문 **고유1,928행을 독립 배열/실제9 Player 기록으로 각각 재생**했다. 마지막 Damageable 진입에서 중단하는 효과 요청 재생이며 실제 전체 파괴/희생을 수행하지 않는다. 기존 checking 부모6개와 Rune 현재 번호/실제 Rune·너겟 연결도 통과했다. 새 Native/fixture0·fixture 기대값 변경0·기존 수동 단언 변경0·보호2,761개/fixture941개 모두 불변이다.

제품873/11.01, EXE **10,597,888바이트**, SHA-256 `8e4d09458b527ec4d66734c82e9ee954da31cf0cd37b051c75503b939591e867`다. checking10(부모4/부분3/MP3)·완료부모38/부분3을 유지한다. 전체 Pop·자연 완주·전체 동일성 완료가 아니다.

[JSON](../../cpppj/dais-class-current-20261105.json)에 첫 실패/교정/최종 소스·빌드·원문 해시를 기록하고 원시는 Git 제외 `extracted/cpp-resume-873/`에 보존한다. 최초 변경11소스 외에 최초에는 변경 없던 DaisLifecycleTests는 교정 후 HEAD의 동일 원문을 source-first에 보존했다. 실제 게임/GUI·포획/희생/특정 적 공격/승리·신규MP/패킷/통신·공중3종 전용·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은0이다. 다음874는 Rune 번호와 미션 상태의 남은 사본/선택 장치 연결이며 의식 효과는 코드/정적으로만 다룬다.
