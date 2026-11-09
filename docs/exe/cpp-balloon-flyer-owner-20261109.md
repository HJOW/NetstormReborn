# cpppj 1212 — 벌룬·Flyer 장기 본체 연결

**구현·검증·인계 완료. 제품1212/11.01이며 전체 목표는 미완료다.**

벌룬5040c0/504198의 같은 RawBalloonBody를 장기 Storage가 보유하고 Native Session이 빌린다. Robust·PostPop·PreDestroy·CloudRegular를 같은 현재 Pool/TYPE/프레임/공간/Host/Carrier 부모에 연결했다. 독립 typed 소유 계약은 유지한다. 새 World·Player·난수 상태나 성공 fallback을 만들지 않는다.

## 원문과 수명

| 실제 진입점 | 같은 장기 연결 |
| --- | --- |
| `42ae80` | 현재+8 WORD 그림자 가상 삭제(0) → Damageable → 현재 boss/내용물 조회 → 실제 보충 부모. 두 실제 VT만 분배한다. |
| `42ac90/42aed0` | 현재 TYPE 프레임과 같은 실제 Factory·Pop/Unpop/표시 깊이·Carrier 확인·공통 후처리. Cloud 최초 Regular도 실제 확보 객체를 같은 Host에 부착한다. |
| `42ae60` | 최초 signed frame을 보관한 뒤 현재 TYPE 프레임 수를 읽는 Robust다. |
| `42af60` | 사건3은 RawFrameAdvance와 원문 payload 비트를 보존한다. 나머지는 현재 실제 Runtime pin → 기존 Session Carrier42a750에 같은 인자를 전달한다. |
| `45e7e0` | 세 실제 Flyer VT의 현재+8 WORD 대상 삭제(0) → 같은 장기 Damageable Pre다. Native Session도 이 본체를 빌린다. |

그림자 삭제 후 현재 boss/내용물을 다시 읽는 기존 본체를 재사용했다. 상위 삭제의 dead/로그/깊이를 보존하고 현재 자식 가상 삭제의 거부/실패를 숨기지 않는다. 장기 장치는 선택한 함수 사본을 호출 동안 보유하며 큰 Carrier 사건은 현재 전체 Runtime을 pin한다. Runtime 없는 큰 사건을 NoWorld 성공으로 합성하지 않는다.

원문 **PATCH8개 전체 C/PE**(`42ae80/45e7e0/42ac90/42aed0/42af60/42ae60/426890/42a750`)를 읽기 전용으로 보존했다. CD에 같은 전용 진입점이 있다고 추정하지 않는다. 마지막 Client 정밀 디컴파일 **2026-11-03 / HJOW-Athlon(192.168.0.94)**, R **2026-11-07 / 같은 호스트**이며 현재도 동일하다. 새 디컴파일·원문 변경은 없다.

## 확인한 범위

Release/x64 제품2·집중5대상 종료0, 경고·오류0, 소스13개를 빌드 동안 고정했다. 기존 허용 비MP245개 + 새 벌룬9개 + 기존 그림자489입력/Robust2사례 = **256/256** 통과했다. 같은 장기 물리·실제 두 부모·WORD 그림자 중첩 삭제·자기 dead 재진입·그림자 실패의 두 깊이·실제 그림자 Pop과 Cloud Regular/프레임·큰 Carrier 미연결 경계를 확인했다. Native 방송 장치는 금지하며 실제 client SID/원문0x10 자연 억제를 사용한다.

최초 빌드는45425 종료0/395.055초/소스13고정이었다. 최초 집중20900은255/256으로 CloudFirstPop 검사1건이 실패했다. 도우미의 추상/그림자 extra9가 실제 firstPop flags1을 막은 입력 문제였다. 해당9비트만 지우고 일반 최초 Pop을 사용했으며 기존 process/payload/count/depth 단언을 유지했다. 실제 Host.RunFrame의 count1·frame+1·payload 보존도 추가했다. 최초 빌드·소스 고정·검사·실패 로그를 보존했다. 최종 수명 검토에서 balloonBody가 실제 참조하는 carrierPre보다 먼저 파괴되도록 선언 순서도 교정했다. 중간 통과 제품·검증은 pre-lifetime 원시에 보존하고 새 소스13의 빌드/집중/GUI로 확정했다.

**공중3종 전용 생성/생산/행동 관측과 Flyer 전용 fixture 실행은 사용자 제외로 하지 않았다.** 세 Flyer 전처리는 원문/가상표 분배/Native 동일 본체 대여/컴파일만 확인했다. MP 단위/패킷/통신/게임/기록 검사도 하지 않았다. 벌룬의 비3 큰 Carrier 성공이나 모든 NoWorld 파생 Regular 완성을 주장하지 않는다.

같은 최종 EXE로 Tutorial4 정상 UI 생산·건설 요청 → TYPE75/owner1 생성·요청 증가 → 600프레임 대기/정상 종료를 확인하고 015 PNG를 확인했다. Client800×600 Edit TEST01↔Main2회/11관측도 종료0이다. 건물 완성·Test Battle·특정 공중유닛 생산·공격/포획/희생/승리 검증이 아니다. 모든 시작 핸들의 실제 종료를 확인했다.

- Netstorm.exe 13891072 bytes, SHA-256 `8e359bf6664ef4a43cf34e472457efd09545a5f7180b03e0b932bcfbdb9abdc9`.
- R.exe 677376 bytes, SHA-256 `a81f54634de6b2133084c827b55eab4091fcfbd9681e52b1df0386bc678e8925`.
- 보호 원본/CD/fixture·AGENTS 2,800파일 불변, 원본 게임 실행·새 MP 실행·원격 push0.

## 남은 범위

실제 Pre/Post 분배 **77/83**, 전용 부모 **6**, 통지 **6**이 남는다. 큰 비영 AI·다른 Form/Regular·다른 배치/Fort 상위 Finish·전수 Caller/owner 감사, M6-l/m·M7/M8·K/S/B 상위16계획/등록34버그는 그대로다. 이번 연결을 알려진 버그의 조사/수정 완료로 세지 않는다. 전체34진입 제품644/검증645·최신ZIP1092·최신 전체 솔루션 통과 없음도 유지한다.

[현행 인계](../../LEFT_JOBS.md)·[최종 보고서](../../cpppj/balloon-flyer-owner-20261109.json), 원시 `extracted/cpp-resume-1212/`의 원문·static-audit·source-frozen/build/tests·GUI JSON을 따른다.
