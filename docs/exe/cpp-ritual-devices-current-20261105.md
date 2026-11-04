# 단위876 — 다이스·제단 Regular의 선택 장치

RawDaisRegular/RawAltarSacrifice가 PATCH에서 도달한 효과만 요구하도록 연결했다. 같은 풀·TYPE/프레임 크기 계약은 유지하고 CD의 기존 생성 시 필수 장치 계약은 보존한다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**, 현재 호스트도 일치하며 원본/정밀 산출물은 변경하지 않았다.

Rune/Dais/Altar Regular의 원문 `0044a3a0/0044b150/0044a5e0`와 CD `004846b0/00482c40/00483800`을 정적으로 대조했다. Rune의304 이외 사건은 효과 없이 payload를 반환하고, 제단은 사건 선택 앞에서 현재 위치의 typed Dais 조회를 먼저 한다. PATCH 생성 시 음향/생성/획득 등 모든 장치를 요구하던 조건을 제거하고 각 기존 호출 위치에서 선택 장치를 복사해 요구한다. optional getter의 기존 fallback도 유지한다. callable 복사는 자기 연결 교체 중 함수 대상의 수명을 보존하며 모든 부모 자기 삭제 수명 검증을 뜻하지 않는다.

다이스 optional canonical 사제 fallback과 제단의 사제 내용물/생성 인자는 875의 같은 현재 `TempleRitualState::PriestType()`을 읽는다. 실제 Session의 기존 hook 우선순위를 유지하며 별도 Prepare 사본으로 돌아가지 않는다. 음향·의식/희생·해방·삭제/획득 및 MP 가지는 코드/정적/컴파일 연결이고 해당 효과를 새로 실행하지 않았다.

Release 제품/두 집중 대상은 **1회·142.907초·종료0·경고/오류0**이다. 고정5소스의 해시가 같고 추가 교정0이다. 집중 검사는 **고유24개/9묶음(새2), 첫 실패0**이다. 새 다이스 검사는 빈 효과 장치로 현재 사제 번호74의 자격 조회, 다른 표시 TYPE/free의 Rune 무효과 반환과 Dais 선택 fallback 부재를 확인한다. 새 제단 검사는23의 무효과 반환·다른 사건의 선택 fallback 요구와 그보다 앞선 위치 조회/NaN 물리 경계를 확인한다. 두 검사 모두 CD의 빈 효과 생성 거부를 유지하며 실제 의식·생성·삭제 효과를 실행하지 않았다.

기존 checking/VT·Rune/미션·helper 조회와 수명1,928행의 독립 배열/실제 Player 기록 재생도 통과했다. 수명 재생은 마지막 Damageable 진입에서 중단하는 접두이며 실제 전체 파괴/희생 검증이 아니다. 새 실제 Session/Native/fixture0·기존 기대값 변경0·보호2,761개/fixture941개 모두 불변이다.

제품876/11.01, EXE **10,612,224바이트**, SHA-256 `e51aa2d9b64b71df4101ef148890e425dc55e7f5c49b7ad5eef3c453d6172cae`다. checking10(부모4/부분3/MP3)·완료부모38/부분3 유지이며 전체 Pop·전체 동일성 완료가 아니다.

[JSON](../../cpppj/ritual-devices-current-20261105.json)과 Git 제외 `extracted/cpp-resume-876/`에 원문/첫 소스/빌드/검사/보호 해시를 보존한다. 실제 게임/GUI·포획/희생/특정 적 공격/승리·신규MP/패킷/통신·공중3종 전용·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은0이다. 다음877은 다이스/제단의 남은 물리 쓰기·Player 결과 DWORD/Word setter를 원문과 대조하며 의식 효과는 코드/정적으로만 다룬다. 다른 TYPE/모드·TYPE 초기화 writer·Teleport 다른 생성 함수·전체Pop/Factory/군도·메뉴/Edit/State/파일·Windows·후속 단계는 남는다.
