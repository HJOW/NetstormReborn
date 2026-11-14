// 실제 Form 공통 해제의 진단·현재 가상 부모·종속 분리를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>현재 TYPE/VT와 복귀 진단의 원문 부분 효과를 같은 SID 물리에 보존한다.</summary>
public sealed partial class RawSquidForms
{
    /// <summary>세 복귀 진단→void 설정→현재 부모 free 관문→실제 공통 종속 분리를 원문 순서로 실행한다.</summary>
    /// <param name="form">물리 슬롯 번호다. TYPE에서 VT를 추정하지 않는다.</param>
    public void Unpop(int form)
    {
        FormUnpopHooks hooks = _boundUnpop?.Clone() ?? new();
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        // 원문 단언은 도달한 장치만 요구하고 정상 복귀 뒤 현재 바이트를 다시 읽는다.
        void Assertion(string expression, uint patchLine, uint cdLine)
        {
            (hooks.Assertion ?? throw new InvalidOperationException("Form Unpop 원문 진단 장치 미연결"))
                (expression, patch ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", patch ? patchLine : cdLine);
        }
        if (_pool.Slot(form)[TypeOffset] > 0x45) Assertion("isForm()", 0xa59, 0xa10);
        if (form == 0 || (uint)form >= _pool.Capacity) Assertion("isValid()", 0xa5a, 0xa11);
        if ((_pool.Slot(form)[StateOffset] & VoidBit) != 0) Assertion("!isVoid()", 0xa5b, 0xa12);
        uint table = patch ? 0 : Read(form, 0, 4);
        _pool.RawSlot(form)[StateOffset] |= VoidBit;
        if (patch) table = Read(form, 0, 4);
        int parent = hooks.Parent is { } getter ? getter(form, table) : RawContainedLink.CommonContainedParent(_pool, form);
        if ((_pool.Slot(parent)[StateOffset] & FreeBit) != 0) return;
        if (hooks.Detach is { } detach) { detach(form); return; }
        // 독립 입력은 원문 초기 번호를 쓰며 실제 세션은 같은 현재 종속 분리 몸체를 대여한다.
        var state = new SquidContainedUnpopState { ContentFormType = new() { Value = 6 }, GumpFormType = new() { Value = 3 }, UiEvent = new() };
        var effects = new SquidContainedUnpopHooks
        {
            Parent = sid => RawContainedLink.CommonContainedParent(_pool, sid),
            Previous = sid => RawContainedLink.CommonContainedPrevious(_pool, sid),
            SetPrevious = (sid, previous) => RawContainedLink.CommonContainedSetPrevious(_pool, sid, previous),
            ParentNotice = (owner, sid, _) => (_notice ?? throw new InvalidOperationException("ContentForm 부모 제거 알림 미연결"))(owner, sid),
            Assertion = hooks.Assertion,
        };
        new RawSquidContainedUnpop(_pool, state, effects).Detach(form);
    }
}
