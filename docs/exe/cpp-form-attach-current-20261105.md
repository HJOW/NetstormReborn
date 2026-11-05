# 공통 Form 부착의 현재 TYPE·가상 setter·진단 후 부분 쓰기 (단위911)

공통 부착 부모 PATCH004ac800/CD004ac2e0를 현재 Factory에 연결한다. Gump·감시 쌍과 Process의 기존 파생 접두 뒤에서 같은 부모를 사용한다. 공통 Pop 자식과 Process 접두의 나머지 진단·상위 실패 정책은 별도 잔여다.

## 원문 근거와 경계

마지막 전체 정밀 디컴파일과 작업 호스트는 HJOW-Athlon / 2026-11-03이다. 정밀 산출물은 수정하지 않았다. PE의 실제 ret8과 스택을 확인했으며 정밀 소스의 unaff_retaddr는 실제 두 번째 flags 인자다.

- PATCH 부모0은 즉시 반환한다. CD는 intoSid 진단(0x3c2)이 정상 반환하면 슬롯0 부착을 계속한다.
- 현재 raw TYPE의 genus → 진단 정상 복귀 → 현재 VT+8 → 저장한 부모 슬롯/머리 → 자기 머리 진단 → next → 이웃 현재 VT+c → 저장 부모 head → 자신의 현재 VT+c → contained → 직접 Pop(flags|100) → 현재 contentFormType 비교 → 현재 isForm 진단 순서다.
- 원문에 없는 free/dead/예약/같은 부모·동일 SID 선행 거부나 실패 rollback을 공통 부모에서 제거했다. 실제 미지원 VT/선택 장치 부재와 물리 범위 밖은 소비 위치의 명시 호스트 오류다.
- 부모/슬롯 메모리는 호출 중 재할당하지 않는 계약이다. 전역 pool 포인터 자체의 재배치·32비트 this/스택/정수 포인터 ABI 전체는 복제하지 않는다.
- 네 Form의 실제 VT+8은 PATCH004abad0/CD004b04c0, +c는004abae0/CD004b04d0다. 지원 자산 VT83/68개를 PE에서 전수 조회해 +8이004ad020/CD004acca0임을 확인했다. Form은 부모+10/이전+e이고 자산은 부모+e/이전+10이다. raw TYPE에서 배치를 추정하지 않는다.
- PE 확인 결과 Form1/3/6의 +74는 공통 thunk004af160/CD004b04e0다. Process2는004b1440/CD004af380 접두가 있다. Content 정렬 전용 API는 별도 호출 경로이고 VT6의 공통 진입을 정렬 API로 대체하지 않는다.

## 실제 연결

SquidFactory는 매 부착 위치에서 현재 TYPE 표의 flags2와 같은 원문 진단 장치를 사용한다. 실제 World는 현재 Session contained 종단의 contentFormType을 약한 대여로 조회하고, 반납 뒤에는 World 기본 번호로 복귀한다. Gump는 중복 선행 guard를 제거하고 같은 공장으로 보낸다. 감시 쌍은 실제 VT1/3/6의 공통 thunk만 이 부모에 전달한다. Process2를 공통 thunk로 조용히 우회하던 처리는 제거했으며 해당 감시 생성 조합의 Process 접두 미연결은 선택 위치에서 실패한다.

ProcessHost는 기존 직렬화 접두 뒤에 같은 현재 Factory 부착 부모를 호출한다. 접두에 원래 없던 genus 거부를 삭제해 진단 위치를 공통 부모 안으로 옮겼다. 상위 Attach/Reattach의 기존 guard/rollback과 접두의 나머지 진단 정상 복귀는 아직 복원하지 않았다. 새로운 MP·패킷·송신 검사는 하지 않는다.

## 관측과 검사

관측 도구는 두 PE 각432개·고유864개 입력에서 부모 명령만 실행한다. genus/setter/직접 Pop/진단은 명시 장치이며 실제 자식·게임/OS/통신은 실행하지 않는다. 부모0/9, raw TYPE1/6/70, genus2종, head0/자신/이웃, flags2종과 변경6프로필을 교차했다. 현재 가상 토큰/저장 head·부모·Pop 직전 링크/진단 순서/현재 content DWORD/네 슬롯 전체를 동결했다. 동일864개 재관측2회가 모두 최초 파일과 일치했다(3명령/2,592행 실행·고유864입력). 첫 도구와 fixture 원본을 별도로 보존했다. 최초 stage 검증은 Python의 gitattributes LF 정규화와 CRLF 작업 사본 해시 차이로 종료1이었다. canonical 도구도 LF로 맞춘 뒤 두 번째 재관측과 stage 검증을 수행했다. 도구의 내용/fixture/컴파일 소스 변경은 없고 첫 도구와 최종 도구 해시는 같다.

기존 검사 교정은 두 곳이다. 단위910의 미연결 +74 입력은 Content 생성자에서 실제 별도 접두인 Process 생성자로 바꿨다(단언 수/예상 실패 경계 유지). FormLifecycle의 누락 Pop 장치 검사는 원문에 없는 전체 불변 기대를 제거하고, 완료된 parent/next/contained와 void 유지 상태12를 확인한 뒤 명시 로컬 Pop/Unpop으로 복구한다. 기존 Native fixture 기대 바이트는 수정하지 않는다. 단위910 당시 Content 정렬 VT 분류는 이 PE 근거로 정정한다.

- 첫 고유35사례(새3/기존32)가 모두 통과했다. 실제 혼합 VT/WORD와 Content 공통 thunk의 두 사례를 추가하고 TYPE 비교의 원문 읽기 위치를 명시해 마지막 빌드를 했다. 최종 **고유37사례(새5/기존32)** 모두 통과·총26명령/72사례 실행·처음/최종 실패 단언0이다.
- Release2명령 모두 종료0·경고/오류0이다. 마지막 빌드 전2,878소스 snapshot과 최종 소스가 같으며 실제 빌드가 끝나기 전 소스 편집/다른 빌드를 하지 않았다.
- 현재 TYPE/VT 진단 정상 복귀와 약한 Session 번호 대여/반납·실제 Gump 호출·늦은 Content 진단·선택된 미지원 이웃의 부분 쓰기·실제 자산/Form WORD 배치를 확인했다. 기존 감시 생성/공통 Form 수명·혼합 삭제·분해·Graph·생성/Take/Unpop/CD 공간 해제를 함께 확인했다.
- 보호 기존2,785개/fixture965개 불변·새 최초 fixture1개로 현행2,786개/fixture966개다. 새 fixture SHA-256은 `da5dcfb44bff278d696f7ab92144bcd5eeac3540233e939c37f93964b73f32b7`이며 첫 Native 바이트와 같다. 기존 Native 기대 바이트 변경0이다.
- 제품911의 표시·파일·제품 버전11.01, 크기10,778,112바이트, SHA-256 `d4ad6ebd212c56b3db10099ec578c438837970342853e0676b1045b86d077da7`다.
- 실제 게임/GUI·사제 포획/희생·특정 적 공격/승리·공중3종 전용·신규 MP/수신·패킷·송신·통신·사용자 확인·전체 검사/34미션/ZIP은 실행하지 않았다. Native 장치 관측은 실제 자식/통신 검사와 구별한다.

원시는 `extracted/cpp-resume-911/`에 있으며 Git 제외·현재 PC 전용이다. 커밋되는 [JSON](../../cpppj/form-attach-current-20261105.json)·관측 도구·최초 fixture가 근거다. checking8=부모3+부분2+MP3/완료 부모39·부분4와 활성74프로젝트·과거 전체/GUI/34/ZIP 기준은 유지한다.

## 남은 작업

공통 Pop PATCH004ac560/CD004abfb0의 원문 진단 후 void/현재 권한·flags/전송 관문·공유 postPop 깊이와 실제 VT+20을 복원한다. 현재 Pop/ValidatePop의 기존 선행 guard가 이 자식 위치에 남아 있으므로 공통 부착 부모 일치로 전체 Pop을 완료 처리하지 않는다. Process 접두·Content 정렬·Gump 현재 번호/다른 클래스 수명·Factory/Baseprocess 실패 정책, State/파일·객체 UI·Edit·Windows·무검사 MP·4~8차는 전체 목표에 유지한다.
