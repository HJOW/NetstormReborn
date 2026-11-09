# 조사1224 — Bomb Regular16VT의 실제 후단과 현재 연결 경계

**정적 조사만 완료했다. 제품은1223이며 새 구현/실행 통과가 아니다.** Regular 연결40/87VT·잔여47VT/22함수와 M6 전체 미완료 상태를 유지한다.

16VT/TYPE19개가 원문0041e6a0을 공유한다. 현재 Session의 `ConnectedBombTable`은6VT/TYPE9개만 공통 Regular와 대상/효과 본문에 전달한다. NoWorld 기본 Regular에는16VT 모두 미연결이다. 따라서 공통 Handle 한 개의 주소를 연결하거나 기존6VT를16VT로 늘리는 것만으로 전체 주문을 복원했다고 세면 안 된다.

## 실제 분배 행렬

보호10.78 PE의 각 실제 VT를 읽고 원본 생성자 표188행/파생 생성자 recipe와 현재 헤드리스 TYPE 덤프를 연결했다. 이름은 내부 TYPE 이름이다.

| 실제 VT | 원본 TYPE 번호:이름 | 대상 자격+bc | 대상 효과+c0 | 현재 Session 공통 분배 |
| --- | --- | --- | --- | --- |
| `00502370` | 113:bombExplodeSmall, 114:bombExplodeMedium, 115:bombExplodeLarge, 168:bombSpecialOne | `0041e9c0` | `0041ea30` | 연결6 중 하나 |
| `00502448` | 116:bombHeal | `0041ebb0` | `0041ebd0` | 공통 분배 밖 |
| `00502510` | 117:bombInvisible | `0041eca0` | `0041fc50` | 연결6 중 하나 |
| `005025d8` | 171:bombTwister | `0041ecf0` | `0040b380` | 연결6 중 하나 |
| `005026a0` | 179:bombIMano | `0041ede0` | `0040b380` | 공통 분배 밖 |
| `00502778` | 180:bombIIMano | `0041ef30` | `0040b380` | 공통 분배 밖 |
| `00502840` | 181:bombIIIMano | `0041f110` | `0040b380` | 공통 분배 밖 |
| `00502908` | 175:bombGraviton | `0041f2f0` | `0041f310` | 공통 분배 밖 |
| `005029d0` | 183:bombLightingwave | `0041f3d0` | `0041f410` | 공통 분배 밖 |
| `00502aa8` | 182:bombLightingZap | `0041f3d0` | `0041f4e0` | 공통 분배 밖 |
| `00502b70` | 178:BOMBmeteor | `0041f3d0` | `0041f650` | 공통 분배 밖 |
| `00502e68` | 172:bombIITwister | `0041fb60` | `0041fc50` | 연결6 중 하나 |
| `00502f30` | 173:bombIIITwister | `0041fcc0` | `0041fc50` | 연결6 중 하나 |
| `00502ff8` | 118:bombParalyze | `0041fdb0` | `0041fdd0` | 연결6 중 하나 |
| `005030c0` | 119:bombHardener | `0041fe90` | `0041feb0` | 공통 분배 밖 |
| `00503188` | 120:bombTreason | `0041ff20` | `0041ff90` | 공통 분배 밖 |

모든16VT의 +5c는0041e6a0, +b4는0041e3c0이다. +bc는14고유 함수, +c0는11고유 함수다.0040b380은 실제 ret4이므로 그4VT의 대상별 효과가 비어 있는 것은 원문이다. 이 사실을 다른 미복원 자격/효과의 빈 callback 근거로 확장하지 않는다.

현재 기본 폭발·은신·마비·Wind3종은 기존 RawBombTargets/RawBombBlast/RawBombStatus/RawWindSpell과 ConnectDevastation으로 이어진다. 이6VT의 장기 body는 아직 없으며 현재 전역을 호출 앞에서 값으로 복사한다. 나머지10VT의 주문 자격/효과는 이 공통 Session 분배 밖이다. Meltdown의 별도58h/60h 경로를 이16VT가 연결됐다는 근거로 쓰지 않는다. 파일의 주소 문자열 검색에서도 이10VT의 독립 PATCH 자격/효과 연결은 찾지 못했다(CanonDecoder의41feb0은 CD 주소라 제외).

## 유지할 원문 순서와 다음 구현

1. **공통 장기 본문:** 같은 Storage의 Pool/Frame/현재 TYPE와 Boss·로컬 owner·5fh 헤더/선택 장치를 공유한다. Native Session도 같은 본문을 빌린다. 기존 독립 typed 입력을 유지하고 Session별 표/권한 사본으로 장기 연결을 대신하지 않는다.
2. **TYPE 조회:** count0에서 carrier WORD+c가 비영일 때 현재 casttime의 두 배를45a6c0(carrier,6,...)에 전달하고, 그 효과 뒤 현재 TYPE의 A 첫 프레임을 다시 읽는다. count가 비영이면4afc90(1,0)을 호출한다. 현재 casttime은 TYPE+1b8, A 첫 프레임은+130이며 단순히 하나의 초기 자산값으로 고정하지 않는다.
3. **시간·권한:** 프레임 효과 뒤 현재 Boss를 읽고 signed count×payload를 float32에 저장한다. 원문 FILD casttime과 이 float를 비교한 뒤 엄격하게 casttime이 작을 때만400바이트 할당/대상 검색으로 간다. 같음·NaN은 원문 base payload 반환이다.
4. **대상/배포:** 실제 new400의 성공/실패와 동일 검색 결과·해제 수명을 보존한다.41e4b0의+b4→검색→현재+bc를 사용하며 Wind의+bc는 생산 프로세스를 만들고 항상0을 반환하는 실제 부수 효과다. 순수 bool 대용으로 바꾸지 않는다.210바이트5fh 예약은 현재 BYTE/BYTE/WORD와FF 초기화를 보존하고 기존 실제 Client 방송·Source별 observer에 전달한다. 원문에 없는 선행 길이 마무리/가짜 ACK를 넣지 않는다.
5. **현재 수신/수명:** 방송 뒤 현재 로컬 owner로 기존420bf0 전체 본문을 호출한다. 대상별 현재+c0, 실제 HP/표시/프레임/소유자/효과·소리·가상10을 연결한다. 실제 큰 Runtime이 필요한 하위는 weak 대여→실제 전체 pin→함수 사본으로 호출하며 미연결10VT를6VT의 효과로 대체하지 않는다. 반환 상태는 해제 전에 저장하고 실제 free 뒤0/-1을 반환한다.
6. **구현 순서:** 먼저 공통 장기 owner와 기존6VT의 같은 실제 경로를 연결하되6/16 범위를 명시한다. 이어서 미연결10VT의 자격/효과14/11 분배를 종류별로 채우고 원문의 실제 빈 ret4와 구별한다.16VT의 공유 Handle만 연결한 수량과 전체 하위 완료를 따로 기록한다.

## 근거·검증·한계

현재와 마지막 정밀 Client 양판본2026-11-03/R전용2026-11-07은 HJOW-Athlon이다. 새 디컴파일/정밀 원문 변경0이다. 이번에는 PATCH만 직접 조사했고 CD 대응/호환성을 완료했다고 세지 않는다. 원시 `extracted/cpp-resume-1224/`에 실제 표, 헤드리스 TYPE 덤프, 함수 요청38개 중 **독립 전체 C/PE33개**와 **C metadata 없는5개**의 제한 PE 근거를 보존했다.5개는41e3c0·41ebd0·41fc50·41fdd0·41feb0이다.

41e3c0은4ac550→TYPE.range+44→CRT ftol4e49c0의13바이트 tail jump다. 최초 보조 경계 검색은 다음 함수의 RET까지 포함했고 교정 원시를 first-*에 보존했다. 다른4개는 첫 RET까지의 실제 PE 몸체다. 독립 전체 C가 없는5개를 C 복원33개에 더하지 않는다. 효과 고유 함수의 최초 보조 기대 개수12는 실제 집합11로 고쳤고 집계 스크립트 원시를 보존했다. 제품 코드/fixture 기대값은 바꾸지 않았다.

게임 구동·검사·GUI·MP·패킷·통신 실행은0이며 창 없는 --dump-types만1회 실행했다. 새 빌드/회귀/ZIP을 반복하지 않았다. 감사한 소스11개와 제품1223 EXE 바이트가 불변이다. 제품1223의59/59·Tutorial4·Edit와 보호2,801파일 결과는 직전 구현 단위의 결과이며 이번 조사에서 새로 수행한 검증으로 표시하지 않는다.

[현행 인계](../../LEFT_JOBS.md)·[정적 감사 JSON](../../cpppj/bomb-regular-audit-20261109.json)·[확정 제품1223](cpp-content-owner-20261109.md).
