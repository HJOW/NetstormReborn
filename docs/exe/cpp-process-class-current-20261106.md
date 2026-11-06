# Process 현재 클래스와 Kernel 소비 경계 — 단위972

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 원본/정밀 자료는 읽기만 했으며 우선순위4를 계속한다.

## 원문 경계와 변경

Process VT PATCH00512098/CD00505990의 preDestroy(+14)는004ae240/004af450이다. 비정렬 PID DWORD(+18)로 Kernel을 조회하고 반환 객체의 가상+c를 반드시 호출한 뒤, Form의 현재 PID를 다시 읽어 제거하고 공통 pre 깊이를 반납한다. 두 판본 모두 처음 NULL을 허용하는 분기는 없다. Kernel 등록도 슬롯1부터 시작한다. **PID0인 bare ProcessForm을 성공적으로 삭제하는 동작을 새로 만들지 않았다.** 유효한 PID를 가진 TYPE2와 PID0인 TYPE2는 다른 입력이다.

반면 부모 삭제 통지(+1c)는004ae2b0/004af4c0이다. Kernel 조회가 NULL일 때만 빈 통지를 허용한다. 실제 객체가 있으면 가상+10을 호출한다. 호스트에서 다른 C++ Kernel 클래스를 조회한 경우를 NULL과 같은 성공으로 숨기지 않고 선택된 실제 장치 미연결로 구별한다. 원본의 잘못된 포인터 호출 자체를 실행하지 않는다.

기존 분배기는 TYPE10~끝 범위마다 같은 Process 훅을 등록했고, Host도 삭제 효과 앞에서 TYPE/현재 상한을 검사했다. 실제 Process VT의 TYPE가 바뀌면 부모 통지·pre/post·Unpop이 거부되거나 다른 fallback에 재진입했다. 네 내장 클래스를 각 실제 VT의 클래스 번호1/2/3/6에 등록하고 Host의 삭제 효과도 현재 Process VT로 선택한다. PID와 실제 Kernel 객체는 원문의 각 소비 위치에서 확인한다. 이벤트 finder의 TYPE/상한 관문은 별도 원문 소비자이므로 유지한다.

호출되지 않는 private TYPE기반 previous 쓰기 helper와 그 전용 상수도 제거했다. 실제 부착/분리는 이미 복원한 현재 VT 공통 getter/setter를 사용한다. 이 변경을 모든 자산 TYPE 범위의 공통 삭제·Kernel 진단/파일 완료로 확대하지 않는다.

## 수정 전 증거와 검사

제품 수정 전 실제 Host/Kernel·공유 부모 삭제·실제 World의3검사에서 **80단언 실패**를 확인했다. 부모 통지는 자식 dead 설정 전에, pre 콜백은 dead 설정 뒤에 TYPE를 바꾸며 소멸자는 Kernel 슬롯이 살아 있는 시점에 호출된다. 새 검사는 이 순서와 최종 Form/Kernel 반납·공유 깊이를 확인한다. 유효 Form TYPE1/2/3/6/7/46/63/69를 사용하며 실제 전투/프레임/통신을 실행하지 않는다.

NULL 부모 통지와 필수 pre PID의 실패를 구별하고 실패 앞의 dead/깊이 쓰기를 보존한다. 실제 World에서는 TYPE2/7/46/69로 바꾼 같은 Process VT를 삭제하며, 별도 finder는 기존 타입 관문에 따라 원래46만 찾는지 확인한다. 생성은 정상 프로세스 타입으로 끝낸 뒤 별칭을 적용한다. 원문의 생성 진단을 생략한 입력으로 바꾸지 않았다.

**Release3명령 모두 종료0·경고0·오류0**이다. 수정전 집중 대상과 최종 게임/집중 대상을 빌드했고, 마지막 검사 입력 정리 뒤 집중 대상도 빌드했다.

최종 **고유16검사(새3/기존13), 실패0**이다. Host/공유 삭제48입력(양판본32+PATCH의 생성 후 현재 상한0 추가16), NULL/다른 Kernel 클래스4입력, 실제 World8입력이 통과했다. CD 독립 Host의 기존 상한 getter는 고정62라 같은 상한 변경 입력을 중복하지 않았다. 기존 생성/현재 Factory4검사, 상위 부착5검사와 Native48행, 혼합4클래스·대여 종료·Gump/Content·Kernel 소멸자 재진입도 통과했다. 지정 수신을 포함하는 Retake 전체 fixture는 실행하지 않았다.

제품972/11.01은 **11,360,256바이트**, SHA256 `a7f730c020f562d77b239cb38c36494b5ee25a323b29d1d08fcc64cf8e8ba2ab`이다. 보호2796개/fixture976개 불변·새fixture0이며 다음 기준은 `extracted/cpp-resume-972/protected-final.json`이다. [단위 보고서](../../cpppj/process-class-current-20261106.json)를 따른다. 솔루션83/checking잔여8은 불변이다. 새 Native/fixture·게임/GUI·MP/프로토콜/통신·공중3종 전용·실제 포획/희생/특정 적 공격/승리·전체CTest/34미션/ZIP은 실행하지 않는다. 마지막 GUI는968이다.

## 남은 수명 경계

초기 PID0의 preDestroy와 달리, 정상 pre 콜백이 바꾼 현재 PID의 Kernel.Remove에는 빈 슬롯 진단이 있다. PATCH004719e0은 `processes[pid] != NULL` 진단 정상 복귀 뒤 현재 슬롯을 다시 읽는다. 현행 Kernel.Remove의 직접 예외와 실제 Client 진단 연결은 다음 범위다. 기존 RawProcessSummary/Client Diagnostics의 procSumm.txt 출력은 이미 있으며, Kernel.Add 소진 호출·실패 UI와의 접합이 남아 있다. 외부 heap·할당 실패, 자산 TYPE 범위/전체 공통 삭제·파일·우선순위4도 남아 있다.
