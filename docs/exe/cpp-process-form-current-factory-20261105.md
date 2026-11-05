# ProcessForm·Gump Take의 현재 Factory와 World 대여 (단위907)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 원문과 실제 연결

원문 Baseprocess0041bd20/CD0048f590은 Kernel 등록 뒤 현재 processFormType00541078/CD0051c968으로 공통 Create 또는 Take를 호출한다. Create는 실제 반환 객체에서 Form SID를 얻지만, 지정 슬롯의 Take 반환은 무시하고 원래 요청 슬롯에 프로세스 TYPE/PID를 기록한다. 이 차이를 유지했다. C++ Attach의 생성/지정 두 분기와 Reattach의 free 분기 세 곳을 `CreateOrTakeForm`에 연결했다.

실제 장기 SquidProcessHost는 현재 ProcessHostRuntimeBindings.factory를 선택 위치에서 빌린다. 공장 포인터를 장기 Host 자체에 저장하지 않으며 실제 World만 자기 공장을 짧은 대여에 넣는다. 바인딩은 다른 Pool 공장을 거부하고, 현재 공장이 없으면 선택 위치에서 오류를 내며 옛 World나 별도 공장으로 우회하지 않는다. 외곽 RuntimeCall의 activeCalls가 실행 중 반납을 막는다. 직접 World Host는 같은 수명의 Factory를 명시 연결하며 다른 Pool/주소 재연결과 장기 Host에 대한 직접 연결은 거부한다.

독립 Host도 TYPE snapshot으로 만든 같은 공통 Factory 부모를 사용한다. 고정 ProcessForm VT 생성 분기를 남기지 않았다. 새 GameAux.ProcessFormType 최초2의 주소를 양판본 ProcessHostStorage가 빌리며 일반 World 준비에서도 번호를 보존한다. 실제 GumpFormHost::Take도 같은 Factory.Take/현재 생성자/반환 VT+2c를 사용한다. Form 하위 Unpop/상태·TYPE·부모 알림 계약 자체를 새로 복원한 것은 아니다.

## 입력과 검증 범위

공통 생성 부모가 TYPE 생성자를 읽으므로 관련 독립 World/Host 입력82파일·127표에 원문 내장1/2/3/6 생성자 주소를 보충했다. 실제 RiftTypeTable 로더와 같은 전제조건이며 생산 코드에서 누락/주소0을 고정 Form VT로 덮지 않는다. 기존 단언2,387개와 검사 이름은 모두 같다. 보호된 fixture와 기대값은 변경하지 않았다. 관련 추가 파일58개는 같은 집중 대상에 컴파일하되 새 의식/공중/패킷/송신/MP 전용 검사는 실행하지 않는다.

새 검사는 양판본 현재 Form 번호7→8/생성자 변경·실제 연속 World 대여/반납 뒤 공장 부재, 독립 Host의 전체DWORD, Gump Take의 현재 생성자·payload/부모 체인·Pool 보존, 다른 Pool/누락 Factory와 장기 Host 직접 주입 거부를 확인한다. 기존 독립 Process/수명·공통 생성 관측을 재사용하며 신규 Native/fixture는0이다.

Release 제품/집중 빌드1명령이 첫 시도 종료0·경고/오류0이다. 줄 끝 보존 도우미 종료→2,870소스 snapshot→CMake를 같은 명령에서 순차 실행했다. 최종 소스는 snapshot과 같고 빌드 중 소스 편집/다른 빌드는 없었다.

새4/기존51의 **고유55사례·19명령/55실행이 첫 실행부터 실패0**이다. 새 현재 번호/생성자/World 대여·독립DWORD·Gump Take·부재/다른 Pool4, 기존 Process10·HostStorage11·Form 생성3·Form Take4·Create 부모6·Take 부모3·생성자 표1·bare Form1·Regular4/보호막 Regular8을 대조했다. 등록 이름에서 선택 사례의 중복0도 확인했다. 추가 파일58개는 컴파일했고 새 의식/공중/수신/패킷/송신/MP 검사는 실행하지 않았다.

기존 보호2,782개/962fixture는 불변이며 신규 Native/fixture0이다. 입력82파일의 기존 단언2,387개와 검사 이름도 같다. 원시 로그/소스 snapshot/보호 해시/입력 단언 비교/선택 명단은 `extracted/cpp-resume-907/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/process-form-current-factory-20261105.json)이다. 활성74프로젝트/과거 전체·GUI·34미션·ZIP 기준을 유지한다.

제품은 **907 / 표시·파일·제품11.01**,10,745,856바이트, SHA-256 `2e24596466f63da3269df6b119948df2d401b43d7268851ae87a85d4aa3044f3`다.

## 잔여

Baseprocess 부모의 기존 선행 guard/실패 rollback과 다른 현재 필드·가상 부착/부모 상태 진단은 전체 원문 부모 완료로 세지 않는다. Form1/2/3/6 실제 해제의 현재 TYPE/VT·void·부모 제거 알림과 CD Unpop, 다른 Factory/HP caller는 이어서 대조한다. 독립 SquidForms의 직접 생성자/편의 API와 실제 Product caller를 구별한다.

checking8(부모3/부분2/MP3), 완료부모39/부분4와 State/파일·객체UI/Edit·Windows·무검사MP 및4~8차의 전체 목표를 유지한다. 실제 게임/GUI/진단 UI·포획/희생/특정 적 공격/승리·새 의식/공중/수신/패킷/송신/MP/통신 검사는 실행하지 않는다.
