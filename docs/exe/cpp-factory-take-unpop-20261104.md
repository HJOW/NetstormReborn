# Take의 실제 Unpop 호출과 현재 TYPE·PostTake 진단 복원

단위836 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**으로 동일하다. 직전835는 `67d399fc / 20261104-17 CP`다.

PATCH004af610은 모든 !free 슬롯에 현재 VT+48(0)을 호출한다. contained를 먼저 거부하거나 void라고 호출을 생략하던 차이를 제거했다. 현재 지원 VT를 선택하며 현재 TYPE 생성자 VT와의 쌍 일치를 강제하지 않는다. Unpop이 정상 복귀하면 같은 슬롯을 다시 읽어 state&fc를 쓰고, 그 뒤 현재 요청 TYPE의 생성자를 조회·실행한다. TYPE BYTE→현재 void OR→PostTake 순서를 보존한다. 미지원 생성자도 이 실제 조회 위치에서 실패하며 앞선 분리/상태 쓰기를 되돌리지 않는다.

PATCH004ad4a0은 현재 TYPE<70이면 !isForm(Squid.cpp:7f6)을 진단하고 복귀 뒤 현재 TYPE를 다시 읽어 zOrder low BYTE를+35에 쓴다. 그 뒤 현재 contained에 대해 !isContained(7fa)를 진단한다. 부모 dead 또는 void 입력의 contained 잔존도 zOrder 쓰기 전에 거부하지 않는다. 진단 뒤 쓰기를 반복하지 않는다. 원문 PE의83개 자산 VT는 모두+48=004afe50·+2c=004ad4a0이다.

SquidFactoryDiagnostics는 같은 실제 TYPE span과 Client 진단 장치를 빌린다. RawSurfaceWorld는 Storage TYPE/AssertDisplay를, 독립/실제 Session은 같은 core.types와 진단 서비스의 값 사본을 연결한다. 생성 시 사본을 쓰던 독립 검사의 계약은 미연결 기본값으로 유지한다. 현재 TYPE의 생성자/순서 수정이 실제 다음 조회에 반영된다.

새 계약7개와 실제 Client/Session/World 1개(비void/void×live/dead부모4조합)가 첫 실행에 통과했다. 실제 GumpForm/UpdateContents FIFO·현재TYPE zOrder 변경·부모 링크·후단 진단 선행 쓰기를 대조했고 전투 프레임은 실행하지 않았다. 기존 생성 검사8개·contained11개·Unpop7개·생성자표1개를 합해 **집중35개 실패0**이다. 기존 원문 생성6992행/12시작·생성자359행·Unpop1300행/3618단계·VT151을 유지했다. 새 독립CPU/fixture는0이다. Creation fixture 재생/AnimTake의 실제 Unpop 장치만 연결했으며 기대 바이트/체크섬은 변경하지 않았다.

835의 실제 contained 종단과836의 Take 상위 호출자·후단 연결로 **004afe50 checking 부모를 완료** 집계한다. checking14=부모8+부분3+MP분류3, 완료부모34/부분3이다. 이는 전체 외관·모든 시나리오/모든 상위 함수의 정상 동작 검증이 아니다. Factory의 SID 범위/TYPE 불일치 상세 로그·원문 진단 정상복귀는 별도 잔여다. CD의 기존 Take/일반 Unpop/PostTake 경계도 유지했으며 이번 패치판 완료로 확대하지 않는다.

Release 빌드1회 종료0·경고/오류0이다. 제품836/11.01 EXE SHA-256은 `ad5764db96aa59fdebe4010e3c161e83e9438b5586d80c96de417de491325c60`다. 보호2759개/fixture939개 변경0이다. 원시 로그는 `extracted/cpp-resume-836/`, 상세 해시는 [836 JSON](../../cpppj/factory-take-unpop-20261104.json)에 있다. 정적 리뷰에서 발견한 초기 생성자 지원 검사의 잘못된 선행 위치와 TYPE/VT 쌍 강제는 최초 빌드 전에 해소했다. 최초 빌드·검사 실패는0이다.

다음은 우선순위1의 객체 메뉴 Summary/회수 가상80 현재 VT 분배다. 실제 게임/GUI·포획/희생/특정 적 공격/승리·신규MP/통신·공중3종 전용·물리/PCM/CD삽입 검사는0이며 전체71/CTest/34미션/ZIP을 반복하지 않았다. push/DN/WB 변경0이다.
