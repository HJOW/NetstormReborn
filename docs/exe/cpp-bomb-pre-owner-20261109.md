# cpppj 1211 — Point Blast 장기 삭제 전처리

**구현·검증·인계 완료. 제품 1211 / 11.01이며 전체 목표는 미완료다.**

Point Blast의 실제 VT `00502370` +14 `0041e960`을 같은 RawSurfaceStorage의 RawBombPreDestroy에 연결했다. 기존 Session 전용 객체를 Native에서 따로 만들지 않고 장기 본체를 빌린다. 독립 typed 경로는 기존 소유 계약을 유지한다.

## 원문과 실제 하위 연결

최초 raw extra&9가 0이고 flags&100000이 있는 경우 `4605d0 → 4a9d70(explosion.wav, loop0) → 비가상4b0950` 순서다. 폭발 뒤 현재 raw 좌표를 다시 읽고, 최초 관문을 재검사하지 않으며 DamageablePre를 추가하지 않는다. 기존 원문96입력에는 효과 사이 좌표/VT/extra 변경도 포함되어 있다.

Storage의 장치는 기존 실제 Damageable 폭발·소리를 공유한다. Client 장치가 현재 실제 Runtime을 pin한 뒤 Session의 기존 `4605d0` Impact/파편 함수 사본을 호출한다. Impact의 현재 로딩/억제/Kernel 관문과 파편 생성은 유지한다. 두 장치도 선택 순간 함수 사본을 보유한다. World·Player·난수 상태를 만들거나 초기화하지 않는다.

선택된 큰 폭발의 Runtime이 없으면 그 위치에서 실패한다. NoWorld 자연 관문과 전용 분배를 연결한 것이며 큰 폭발 성공을 World 없이 재현한 것은 아니다. 전체 NoWorld 큰 하위/큰 비영 AI·호출자/owner 감사는 계속 남는다.

원문 **6개 전체 C/PE**(PATCH5·CD1)를 읽기 전용으로 보존했다. PATCH의 `41e960/4605d0/45a840/4604a0/4a9d70`, CD의 공통 폭발 `4547d0`을 구별한다. CD에 같은 고유 전처리가 있다고 추정해 분배를 추가하지 않았다. 마지막 Client 정밀 디컴파일은 **2026-11-03 HJOW-Athlon(192.168.0.94)**, R은 2026-11-07 같은 호스트다. 현재도 같은 호스트이며 새 디컴파일·원문 변경은 없다.

## 검증

Release/x64 제품2·집중5대상 **종료0/경고·오류0**, 제품 소스7개를 빌드 동안 고정했다. 기존 허용 비MP237개에 새 장기 Bomb7개·기존 원문96입력 사례1개를 더해 **245/245** 통과했다. 자연 extra/flags 관문·현재 VT·선택 해제·실제 공통 Pre/Post/풀 반납·선택된 큰 효과 실패의 앞선 상태를 검사했다. Native 방송 장치는 금지 상태이며 원문0x10으로 실제 외부 전송을 억제한다.

최초 빌드는 새 검사3곳의 비공개 DefaultProcessAssetHooks 호출로 C2248이었다. 정상 완료 검사는 공개 DestroyAsset으로 진입해 실제 상위 깊이/가상 분배/부모/반납을 사용하도록 고쳤다. 큰 효과 실패는 공개 BombPre 본문을 직접 호출한다. 깊이0·선택 해제·효과 관문의 기대를 약화하지 않았다. 최초 빌드·소스 고정 원시를 별도 보존했다. 최초 집중은 244/245였고 실패는 SurfaceAssetDeletion_UnrestoredSpecialPrePreservesPartialEffects였다. 미복원 경계 검사가 이번에 복원한 VT502370을 사용하므로 실제 잔여5040c0/42ae80로 입력만 바꿨다. dead·로그·깊이1·풀 미반납 단언은 모두 유지했다. 처음6소스와 최종7소스/실패 원시를 보존했다.

같은 최종 EXE로 Tutorial4의 정상 UI 생산·건설 요청 → TYPE75/owner1 생성·요청 증가 → 600프레임 대기와 정상 종료를 확인했다. `015-generator-settled.png`를 확인했다. Client800×600의 Edit TEST01→Main 왕복2회/11관측도 종료0이다. 건물 완성·Test Battle·Point Blast 특정 공격/폭발·자연 승리 검증으로 세지 않는다. 모든 시작 빌드·검사·GUI 핸들의 실제 종료를 확인했다.

- `Netstorm.exe`: 13876736 bytes, SHA-256 `a04663c7a007d6e48faae9cd865ef106674b31ad01b49921ba5a5f08c3e1eca2`.
- `R.exe`: 677376 bytes, SHA-256 `9da1a3040e61cb8db230e58334376eb86e2de7aea83a1c3835bdb5eb8cf13e9e`.
- 보호 원본/CD/fixture·AGENTS **2,800파일 불변**. 원본 게임 실행·MP 단위/패킷/통신/게임/기록 검사·원격 push 0이다.

## 현행 잔여와 근거

실제 Pre/Post 분배 **72/83**, 전용 부모 **11**, 통지 **6**이 남는다. 큰 비영 AI·다른 Form/Regular·다른 배치 호출자/Fort 상위 Finish와 전수 감사를 이어간다. M6-l/m·M7의 독립5건/M8·한국어 K1~K5·시나리오 S1~S4·등록34버그 B1~B4는 [현행 인계](../../LEFT_JOBS.md)를 따른다. 이번 연결을 알려진 버그 조사/수정 완료로 세지 않는다. 전체34 진입 제품644/검증645·최신ZIP1092·최신 전체 솔루션 통과 없음은 유지한다.

[최종 보고서](../../cpppj/bomb-pre-owner-20261109.json), 원시 `extracted/cpp-resume-1211/`의 original-evidence/static-audit/source-frozen/build/tests/GUI JSON과 최초 기록이 근거다. 사제 포획/희생·특정 적 공격·시나리오 승리·공중3종 전용 관측과 사용자 제외 검사는 실행하지 않았다.
