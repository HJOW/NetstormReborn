# 더스트 데빌 정기와 공유 음향 장부의 장기 수명 — 제품1248

제품1247 뒤 WindFlyer1VT/0045e8b0을 장기 연결했다. [확정 보고서](../../cpppj/wind-flyer-regular-owner-20261110.json)와 원시 `extracted/cpp-resume-1248/`가 근거다. 전체 목표/M6/NoWorld는 미완료이며 목표를 계속 진행한다.

## 같은 본문과 현재 owner

- Native Session과 NoWorld는 기존 RawWindFlyer 정기·공격·초기화·애니메이션 본문을 같은 Pool/Hash/TYPE/Kernel 시각/현재 boss로 사용한다. 기존 알고리즘 C++/H는 변경하지 않았다. Typed의 기존 독립 몸체는 유지했다.
- Sound47·RawFlyerFrame·Carrier 입력·WindFlyer가 같은 Client 수명의 SoundProcessState/TYPE별 마지막 재생 장부를 빌린다. Client가 NoWorld에서도 현재 하위DWORD 프레임을 갱신한다. 실제 Client SoundList의 activeSound 토큰을 최초 한 번 적재하고 Session/World 왕복과 같은 이름표 재결합으로 덮어쓰지 않는다.
- Native의 가상bc는 현재 TYPE의 hpPerSec/+48과 useairdamage/+1b0·airdamage/+1b4를 읽는다. 원문 parser의 정수 절삭·대소문자·속성 순서/마지막 중복·float32 저장을 유지했다. 두 공중 속성은10.78 전용이다. 독립 TAFF 판독의 실제 비영 선언은 썬 디스크 TYPE71의1/20 한 건이며 CD에는 없다.
- 표적 판정은 기존 GunTarget의 현재 문맥/Player+54/친군사격/진단을 그대로 재사용한다. 큰 표적/피해만 weak 대여→실제 전체 Runtime pin→함수 사본으로 선택 호출한다. World/Runtime이 없는 선택을 빈 성공으로 숨기지 않는다. 다리 약화는 같은 장기 DamageablePost/현재 WORD·공간 통지·표시에 연결한다.
- actual VT+5c=45e8b0, +bc=4d4d40, +c8=45ec30을 실제 PE로 대조했다. 원문12함수의 전체 C/기계어를 보존했고 새 디컴파일/원문 변경은0이다. 마지막 Client 양판본은2026-11-03 / HJOW-Athlon, R은2026-11-07 / 같은 호스트다.

## 검토와 검증

- 첫 빌드/고정 소스를 `first-build/`에 보존했다. 빌드 중 정적 검토에서 새 검사의 부재 생성자 VT 준비값을 찾아 실제 Factory 자산 VT로 교정했으며 해당 오기 검사는 실행 전이었다. 기존 Native 서비스의 이름표/위치 재생 함수 사본도 연결하여 Session 생성 계약을 유지했다. 첫 결과를 최종 성공으로 덮어쓰지 않았다.
- 최종 Release/x64 제품2·집중1대상은 **164.108초/종료0·경고/오류0**, 고정11파일 불변이다. **비MP277/277**이 통과했다. 기존267에 공통 음향/현재 피해율4, 공중 속성 로더1, 기존 SoundProcess5를 더했다.
- 새 검사는 실제 일반 객체의 Sound47 프레임 중복 억제/다음 프레임 재생, 같은 장부·역할·토큰의 World 왕복, 실제 이름표 정체성, 현재 source/target TYPE와 공중 마스크·비영 정수 선택을 확인한다. WindFlyer 로직 getter의 동일성만 확인했으며 전용 유닛 생성/정기/행동 검사는 실행하지 않았다.
- 최종 EXE Tutorial4에서 정상 UI 생산·건설 요청→TYPE75/owner1 생성→600프레임 대기→종료0 및 PNG를 확인했다. 실제 특정 적 공격·포획/희생·승리/자연 완주·공중3종 전용·MP 실행은0이다. 건물 완공·청취·전체 원본 동일성 확인으로 확대하지 않는다.
- 최초 보호 원본/CD/fixture **2,807파일 불변**이다. AGENTS.md는 사용자 커밋75e4eed9e의 글꼴 지침 변경을 다시 읽어 LEFT_JOBS 5.3절·M8·K1/K4/K5에 반영했다. 최초 해시/외부 변경/현행 보호2,808파일 기준을 구별해 보존했고 이 작업에서 AGENTS를 수정하지 않았다. 새 배포ZIP은 만들지 않았다.

## 남은 범위

알려진87VT 정기 진입은 **83/87**, 잔여 **4VT/2주소**다. 다음은 신전5196f0/5197a8/519860/519918의4dc220·4dcf00이며 후자는 실제 jmp thunk다. 준비 원시는 `extracted/cpp-meta-regular-plan-20261110/`에 있다. 큰 GunTarget/빔/피해/분해·Sound47 실행기/큰 Flyer 프레임·Template/사제/의식·최초 후처리·Path/AI/배치/FortFinish·전수 caller/owner·M6-l/m과 나머지 상위 계획은 계속 남는다.83/87을 전체 NoWorld/MP 완료로 세지 않는다.
