# 제품 1216 — 자산 종속 통지의 장기 본문 연결

제품1216의 구현·제한된 검증을 마감했다. 전체 개발 목표는 진행 중이다.

## 범위와 근거

HJOW-Athlon에서 마지막 양판본 Client 정밀 디컴파일(2026-11-03, R 전용 2026-11-07)과 현재 호스트가 같음을 확인했다. 정밀 C/metadata·원본 파일 변경은 없다. 원시는 `extracted/cpp-resume-1216/`다.

실제83자산 VT의 +34 중 77개는 `0041c100`의 빈 ret8이다. 나머지6개는 아래와 같다.

| VT | 원본 통지 | 연결과 순서 |
| --- | --- | --- |
| 00509148·00509210·0050eb98 | 0048f0a0 | 두 인자를 무시하고 현재 가상+10(flags0)을 호출한다. 서버 관문이 없다. |
| 00509930 | 0045b4a0 | 서버 관문·현재 Sun 프로세스 조회·Unpop0·plain frame27·Repop0·dirty4/AP8·실제 Form 갱신이다. NULL 프로세스 실패는 공간 효과 뒤다. |
| 005099e8 | 0045b5b0 | 서버 관문·현재 Rain 프로세스·최초 dirty 보존·AP10·첫 현재 시각/dirty2·게임 난수10000~14999의 정수 /1000·그 뒤 다시 현재 시각·실제 Form 갱신이다. |
| 00509aa0 | 0045b680 | 서버 관문·현재 Wind 프로세스·dirty4/AP2·실제 Form 갱신이다. 시각/난수는 읽지 않는다. |

`0048f0a0`은 정밀 C/metadata에 없어 보호 PE의 `0048f0a0..0048f0a9` 10바이트를 ret8 경계까지 직접 읽었다. SHA256 `d8086afdbc62c8bd9ef98bd4c520fe9d1624c576d2312d0b4d244e5c13cf70a6`, 내용은 `mov eax,[ecx]; push 0; call [eax+10]; ret 8`이다. C/metadata를 합성하지 않았다. 세 홈 함수는 정밀 C와 함수 범위 전체 PE를 보존했다. CD10.72 대응 함수는 현재 정밀 metadata에서 확인되지 않았으며 같은 주소/동작이라고 합산하지 않는다.

## 실제 owner와 호출자

`RawSurfaceStorage::NotifyAssetDependency`는 현재 VT만 조회해 알려진83자산을 처리하고 다른 Form은 false로 반환해 기존 Host가 처리하게 한다. TYPE/owner/두 통지 인자를 추가 필터로 사용하지 않는다. 기본 삭제 종속, Native Session의 종속 통지·직접 파생 효과·Flyer 홈 통지가 모두 같은 실제 본문을 사용한다. 독립 typed 판본의 기존 분배는 유지한다.

세 홈 통지는 기존 `FlyerDied` 본문을 정적 공유 함수로 분리했다. 장기 같은 Pool/현재 GameAux TYPE·Host/프로세스 필드·State 시각·Client 게임 난수를 사용하며 별도 World·Session·프로세스 상태를 만들지 않는다. 서버0에서는 Host/TYPE/필드/시각/난수를 읽지 않는다. 실제 Form 갱신 전에 현재 Host/필드 정체성을 확인한다. Native Sun의 plain DWORD는 공간 효과 뒤 같은 물리 슬롯에 쓴다. 강한 Session 역참조·가짜 ACK·빈 성공을 넣지 않았다. 큰 건설 효과16·Client 장치10의 수는 바뀌지 않는다.

## 검증 범위

신규 실행 사례7개는 일반 자산 자기삭제/서버0 자기삭제/dead 관문/빈 ret8/미지 VT 반환/현재 VT 선택/삭제 실패 접두를 확인한다. 실제 client SID와 원문 extra9를 사용하고 네트워크 송신 장치는 호출되면 실패한다. 홈 통지3과 공중3종 전용 fixture/생성/생산/행동 검사는 실행하지 않는다. 사용자 제외 MP·실제 포획/희생·특정 공격·승리 검사는 실행하지 않는다.

83/83 Pre/Post와 6/6 통지 분배가 연결되어도 다른 Form/Regular·큰 비영 AI·배치 호출자/Fort 상위 Finish·전수 owner/수명 감사가 남는다. M6 전체/전체 NoWorld/전체 원본 동일성 완료로 올리지 않는다. M6-l/m·M7·M8·K/S/B와 34개 버그의 상태도 이번 통지 연결만으로 닫지 않는다.

## 확정 결과

Release/x64 제품2·집중5대상은 단일 최종 빌드 핸들 종료0, 경고·오류0이며 소스12개가 고정됐다. 기존306+신규 일반 자산7개 = **313/313 비MP 검사 통과**다. 최초 빌드 C2039는 Wind 필드에 없는 Form() 접근자 사용이 원인이며 실제 inner+4/outer+60의 현재 Form WORD 조회로 교정했다. 최초 로그/고정 해시를 first-*로 보존했고 검사 기대를 약화하지 않았다.

같은 EXE에서 Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 실제 생성/요청 증가→600프레임 대기와015 PNG를 확인했다. 같은 Client800×600의 Edit TEST01↔Main2회/11관측도 종료0이다. 홈 통지3/공중3종 전용 동작이나 포획/희생/특정 공격/승리/TestBattle를 검증한 결과는 아니다. 모든 시작 빌드/집중/GUI 핸들의 실제 종료를 확인했다. 보호 원본/CD/fixture·AGENTS2,800파일 불변, 원본 실행·MP 실행·원격 push0이다.

- Netstorm.exe: 13940224 bytes, SHA256 `2ca683fe582342b89f0ddf4d6868613b73d36f1414a407042c6e749e3d943a9e`.
- R.exe: 677376 bytes, SHA256 `c3e11a8b5d6313572bf842f4c862a3f5f32249bbebb341ce77bdb697e83560bf`.

[현행 인계](../../LEFT_JOBS.md)·[최종 보고서](../../cpppj/asset-notice-owner-20261109.json), 원시 `extracted/cpp-resume-1216/`의 bounded PE·정밀 C/ASM·static-audit·first-*/build/tests/GUI/보호 결과를 따른다.
