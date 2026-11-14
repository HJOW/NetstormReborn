// 실제 Gump 등록 표/가상 객체 삭제/메뉴 Form 공급/지연 수명을 공용 SID 풀의 GumpForm 에 연결한다.
// (cpppj client/GumpFormBinding.cpp/.h, 기준선 1f00a25a + 4ee69f4f, 10.78 전용 — CD 10.72 의 Form 삭제/투영 판본 분기는 옮기지 않는다)
// 호스트 Form 토큰에는 원본 SID 를 사용한다. 32비트 원본 포인터/가상 표를 실행 주소로 변환하지 않고, 넓은 SID 를 WORD 로 절단하지 않고 먼저 표현 범위를 확인한다.
// 기준선 뒤 `4ee69f4f`(20261109-105): 메뉴 상태의 현재 객체 Form TYPE 조회(currentObjectFormType)를 연결 때 같은 Host getter 로 공급한다(이미 있으면 보존).
// 웹의 GumpFormHost 는 독립 typed 입력(TYPE 3)만 가지므로 Native 의 BindCurrent(같은 Client TYPE DWORD)는 없고 현재 TYPE 은 host.currentType() 이 있으면 그 값, 없으면 3 이다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다).
// (2026-11-14 WB04-E433: RawMenuControls 선행 Gump Form 연결 이식)
import { GUMP_FORM_TYPE } from '../core/gump-form-host.js';

// 원본의 long SID 를 WORD 로 조용히 절단하지 않고 실제 번호 표현 범위(0..0xffff)를 먼저 확인한다.
function sid(value) {
    value >>>= 0; if (value > 0xffff) throw new RangeError('GumpForm SID 표현 범위 초과');
    return value;
}
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }
// 현재 TYPE 조회다. Native 가 TYPE DWORD 를 연결하면 그 값이고 독립 typed 입력은 내장 3 이다.
const currentType = (forms) => (typeof forms.currentType === 'function' ? forms.currentType() : GUMP_FORM_TYPE) >>> 0;

// 00464740/00466750 의 월드 부착은 같은 Pool 객체 +0e/+12 를 현재 497220 으로 투영한다(10.78 전용). 객체의 최초 위치나 별도 UI 카메라를 저장하지 않고 원문의 현재 raw 두 float 와 픽셀 원점을 매번 읽는다.
// 사용: projectGumpObject(sidPool, cameraView, object) → { x, y }
export function projectGumpObject(pool, camera, object) {
    if (pool.edition !== 'patch1078') throw new TypeError('월드 부착 Gump 의 10.78 객체 투영 필요');
    const raw = pool.slot(sid(object)); const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); // byte 단위 Pool 에서 비정렬 float 를 원문 폭 그대로 읽는다.
    const x = view.getFloat32(14, true); const y = view.getFloat32(18, true);
    const point = camera.project(x, y); return { x: point.x, y: point.y };
}

// 00464190/CD 00492be0: 알림 시점의 등록 표를 조회하고 현재 가상 +4 처리기에 부모 SID 를 전달한다. 매 알림에서 현재 등록 표를 읽으므로 생성 당시의 UI 토큰을 저장하지 않는다.
// 소유 ID 0 생략은 Form 의 notify 가 담당하며 직접 조회에는 원본 등록 표 전체를 사용한다. 사용: new GumpFormHost(pool, factory, types, destroy, { notice: makeGumpFormNotice(createState, releaseObject) })
export function makeGumpFormNotice(state, releaseObject) {
    if (typeof releaseObject !== 'function') throw new TypeError('Gump 가상 객체 삭제 알림 경계 필요');
    return (owner, object) => {
        owner >>>= 0; if (owner >= state.entries.length) throw new RangeError('GumpForm 소유 등록 ID 범위 초과');
        const token = state.entries[owner];
        if (!token) throw new Error('GumpForm 소유 UI 등록이 없습니다');
        releaseObject(token, object);
    };
}

// 00466740/CD 00492ff0 의 기본 가상 +4 다. 전달한 부모 SID 를 사용하지 않고 실제 자식 우선 Kill 을 수행한다(Form 을 먼저 지우고 UI 는 기존 지연 삭제 큐에서 나중에 소멸한다).
// 기본/메뉴처럼 이 가상 함수를 상속하는 노드에서 사용한다. 별도 파생 재정의는 자신의 분배기가 처리해야 한다.
export function releaseGumpObject(lifetime, token, object) { void object; lifetime.kill(token); }

// 메뉴의 Form(type,2)/owner ID/가상 Attach 경계를 같은 SID 풀에 연결하고 실제 내장 타입 3 을 공급한다. widgets 가 훅을 복사하기 전에 연결하며 다른 생성/Attach 저장소와 판본을 맞춰야 한다.
// +0x28 에는 같은 풀의 SID 를 기록한다. 토큰/index 를 동일 번호로 제공하면 원본 stride 나눗셈과 같은 결과다.
// 사용: bindMenuGumpForms(createState, widgetState, widgetHooks, gumpFormHost)
export function bindMenuGumpForms(create, state, hooks, forms) {
    if (forms.pool.edition !== 'patch1078') throw new TypeError('메뉴와 GumpForm 판본이 다릅니다');
    void create; state.objectFormType = currentType(forms);
    if (!state.currentObjectFormType) state.currentObjectFormType = () => currentType(forms); // 실제 메뉴의 매 호출 현재 Host getter 를 보존하고 독립 연결만 같은 고정 Host 를 빌린다.
    // 원본 타입 전역의 다른 값은 별도 Form 재정의가 필요하므로 GumpForm 생성에서 거부한다.
    hooks.objectForm = (type, flags) => {
        if ((type >>> 0) !== currentType(forms)) throw new TypeError('메뉴 객체 부착 GumpForm 현재 타입 오류');
        const form = forms.create(flags); return { token: form, index: form };
    };
    // 등록 ID 는 DWORD 전체를 기록하고 객체 SID 는 원본 번호 범위를 확인해 부착한다. 가상 +0x74 는 소유 ID 를 쓴 뒤 같은 풀의 객체 종속 체인에 연결한다.
    hooks.formOwner = (form, owner) => { forms.setOwner(sid(form), owner >>> 0); };
    hooks.formObject = (form, object, flags) => { forms.attach(sid(form), sid(object), flags >>> 0); };
}

// Gump Kill 의 +0x28 Form 인덱스를 실제 공통 삭제/Unpop/SID 반납에 연결한다. 다른 Form 의 기존 삭제 경계는 보존한다. lifetime 생성 전 호출하며 forms 는 lifetime 과 반환 훅보다 오래 살아야 한다.
// Form 삭제는 UI 알림 없이 공통 삭제에 들어가고, 객체 삭제 알림에서 중첩 호출돼도 dead 분기가 순서를 유지한다. 사용: bindGumpFormLifetime(lifetimeHooks, gumpFormHost)
export function bindGumpFormLifetime(hooks, forms) {
    const previous = hooks.deleteForm;
    // 현재 타입으로 분배하며 반납된 GumpForm 의 dead 기록도 공통 몸체에 전달한다.
    hooks.deleteForm = (form, flags) => {
        const id = sid(form);
        if (forms.isGumpForm(id)) forms.destroy(id, flags);
        else { need(previous, 'GumpForm 외 UI Form 삭제 경계 필요')(form, flags); }
    };
}
