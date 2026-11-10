# 신전 정기 네 VT와 상승의 장기 소유자 — 제품1249

제품1248 뒤 신전5196f0/5197a8/519860/519918을 연결했다. [확정 보고서](../../cpppj/vortex-regular-owner-20261110.json)·원시 `extracted/cpp-resume-1249/`가 근거다. 알려진 정기 진입87/87은 연결했으며 전체 NoWorld/M6/목표는 미완료다.

## 한 원문 본문과 실제 현재값

- 기존 RawAssetRegular::Vortex 본문을 RunVortexRegular 한 함수로 추출했다. Typed는 기존 효과/상태를 전달하고 Native는 같은 장기 fort/권한/checking/mission·현재 TYPE 프레임·단일 GameRandom을 읽는다. 프레임/난수 장치는 선택된 사건에서만 조회한다. 공통 Damageable 부모도 같은 장기 본문을 호출한다.
- 4dcf00의 실제 PE는 `e91bf3ffff`, 4dc220으로 점프한다. 네 VT의 실제+5c와 원문7함수의 전체 C/기계어를 보존했다. 독립 Typed의 다른 CD 프레임 폭/원문 분기는 유지했다.
- Native Session과 NoWorld는 같은 ClientConstructionGlobals의 RawVortexAscend/현재 noIsland 역할/실제 IslandList를 빌린다. 기존 상승 C++/H 알고리즘은 변경하지 않았다. 실제 중첩80BYTE 확보/반납과 같은 Storage의 가상 삭제를 사용하며 Construction weak→pin을 호출 끝까지 유지한다. Session/World 생성으로 상태·난수·목록을 초기화하지 않는다.
- 큰 이탈 안내/방송·같은51h 수신/Descend는 기존 세 함수 사본을 move 전에 보관해 그대로 대여한다. 선택된 큰 호출은 weak 대여→실제 전체 Runtime pin→함수 사본 순서를 지키며 빈 pin/owner/함수는 거부한다. 새 수신 본문/가짜 ACK를 만들지 않았다.

## 검토와 검증

- 첫 Release 빌드와 소스를 first-build에 보존했고, 큰 후단의 중복 lambda를 기존 함수 사본 재사용으로 정리했다. 첫 집중 실행은287중286통과/새 준비 오류1건이었다. 실제 Player 저장소 없이 IslandList::Owner를 호출한 검사 준비 오류로 원시/당시 소스를 first-tests에 보존했다.
- 실제 PlayerBlock과 추가 두 섬의 유효 Bounds를 준비했다. 기대 집계1/0/1/3을 바꾸지 않았으며 마지막 owner 변경 뒤 Player getter를 선택 금지 장치로 바꾸어 집계가 Player를 읽지 않음을 확인한다. 이 교정으로 제품 코드를 바꾸지 않았다.
- 최종 Release/x64 제품2·집중1대상 **27.710초/종료0·경고/오류0**, 고정15파일 불변이다. **비MP287/287** 통과이며 양판본 신전 프레임 사건1/2의 **원본198입력**, 장기 상태/상승의 World 왕복, 실제 Host의 같은 난수/현재 시각 재예약, count31의 난수 생략, 빈 상승의 실제 finder/버퍼 반납, 현재 섬 owner 조회, Construction 반납을 포함한다.
- 신전 이탈/Descend를 포함한 전체 AssetRegular 재생과 MP/패킷/통신/기록 실행은 제외했다. 기존 VortexAscend3검사는 순수 후보/finder/버퍼 원문 부품 검사다. 실제 특정 적 공격·포획/희생·승리/자연 완주·공중3종 전용 실행은0이다.
- 최종 EXE Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 생성→600프레임 대기·정상 종료0/PNG를 확인했다. 건물 완공·청취·전체 원본 동일성 확인은 아니다.
- 현행 AGENTS75e4eed9e를 포함한 보호 원본/CD/fixture·AGENTS **2,808파일 불변**이다. 글꼴 외부 fonts 적재/배포 지침은 LEFT_JOBS 5.3절·M8·K1/K4/K5에 유지한다. 새 글꼴 계약의 제품/ZIP 준수는 아직 미확정이다. 새 ZIP/디컴파일/원문 변경은0이며 마지막 Client 양판본2026-11-03/R2026-11-07, 모두 HJOW-Athlon이다.

## 남은 작업과 다음

알려진 자산83+내장Form4의 Regular 진입은 **87/87**, 알려진 잔여0이다. 큰 신전 이탈/수신/Descend·GunTarget/빔/피해/분해·Sound47/큰 Flyer 프레임·Template/사제/의식·최초 후처리·모든 processVT/호출자/owner·Path/AI/배치/Fort Finish·M6-l/m은 별도다.

다음 caller/owner 감사의 구체적 발견은 RawSurfaceFactoryRegular.cpp의 FactoryCall이다. 현재 pin 함수/반환 owner 누락을 거부하지 않아 M6 계약과 다르며 이번 단위에서는 미수정이다. 실제 활성 경로/효과 순서를 대조해 비MP 허용 검사와 함께 고친다. MP/Outpost 전용 실행은 계속 제외한다. 이후 M7/M8와 한국어·시나리오·버그 계획을 계속한다.
