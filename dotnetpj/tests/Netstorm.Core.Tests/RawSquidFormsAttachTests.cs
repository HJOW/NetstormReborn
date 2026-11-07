// Forms 부착 바인딩의 장치 위임·기본값·진단을 검증한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// Forms 부착 바인딩을 검증하는 테스트 클래스다. 2026-11-07 F2d Forms 부착 바인딩 이식에서 추가됐다.
public sealed class RawSquidFormsAttachTests
{
    /// <summary>부모·폼 장면을 만든다. 2026-11-07 F2d Forms 부착 바인딩 이식에서 추가됐다.</summary>
    private static (RawSidPool Pool, RawSquidForms Forms, int Parent, int Form) Scene()
    {
        var pool = new RawSidPool(32768);
        var forms = new RawSquidForms(pool, null, (form, flags) => { });
        int parent = pool.Allocate(SidAllocation.Server);
        int form = forms.Create(2);
        return (pool, forms, parent, form);
    }

    /// <summary>빈 묶음을 거부한다. 2026-11-07 F2d Forms 부착 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BindAttachRequiresHooks()
    {
        (RawSidPool _, RawSquidForms forms, int _, int _) = Scene();
        Assert.Throws<ArgumentNullException>(() => forms.BindAttach(null!));
    }

    /// <summary>빈 바인딩은 풀 기본 장치로 부착한다. 2026-11-07 F2d Forms 부착 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundEmptyHooksUsePoolDevices()
    {
        (RawSidPool pool, RawSquidForms forms, int parent, int form) = Scene();
        forms.BindAttach(new FormAttachHooks());
        forms.Attach(form, parent);
        Assert.Equal(form, pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8));
        Assert.Equal(parent, pool.Slot(form)[16] | (pool.Slot(form)[17] << 8));
        Assert.NotEqual(0, pool.Slot(form)[RawSidPool.StateOffset] & 8);
    }

    /// <summary>받은 genus 위반은 보고하고도 부착을 잇는다. 2026-11-07 F2d Forms 부착 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundGenusViolationReportsAssertion()
    {
        (RawSidPool pool, RawSquidForms forms, int parent, int form) = Scene();
        var notes = new List<(string Expression, string File, uint Line)>();
        forms.BindAttach(new FormAttachHooks
        {
            Genus = _ => 0x44200u,
            Assertion = (expression, file, line) => notes.Add((expression, file, line)),
        });
        forms.Attach(form, parent);
        Assert.Single(notes);
        Assert.Contains("gEMPLACEMENT", notes[0].Expression);
        Assert.Equal(form, pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8));
    }

    /// <summary>받은 content 번호 일치는 보고하고 다르면 조용하다. 2026-11-07 F2d Forms 부착 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundContentTypeMatchReportsAssertion()
    {
        (RawSidPool _, RawSquidForms forms, int parent, int form) = Scene();
        var notes = new List<string>();
        forms.BindAttach(new FormAttachHooks
        {
            ContentType = () => 2u,
            Assertion = (expression, _, _) => notes.Add(expression),
        });
        forms.Attach(form, parent);
        Assert.Single(notes);
        Assert.Contains("getType()!=contentFormType", notes[0]);
        (RawSidPool _, RawSquidForms quiet, int quietParent, int quietForm) = Scene();
        var silent = new List<string>();
        quiet.BindAttach(new FormAttachHooks { Assertion = (expression, _, _) => silent.Add(expression) });
        quiet.Attach(quietForm, quietParent);
        Assert.Empty(silent);
    }

    /// <summary>받은 쓰기·Pop 장치를 같은 인자로 부른다. 2026-11-07 F2d Forms 부착 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundDevicesObserved()
    {
        (RawSidPool _, RawSquidForms forms, int parent, int form) = Scene();
        var calls = new List<string>();
        forms.BindAttach(new FormAttachHooks
        {
            SetParent = (sid, table, value) => calls.Add($"parent:{sid}:{table}:{value}"),
            SetPrevious = (sid, table, value) => calls.Add($"previous:{sid}:{table}:{value}"),
            Pop = (sid, value) => calls.Add($"pop:{sid}:{value}"),
        });
        forms.Attach(form, parent, 0x10u);
        Assert.Contains($"pop:{form}:{0x10u | 0x100u}", calls);
        Assert.Contains(calls, call => call.StartsWith($"parent:{form}:"));
        Assert.Contains(calls, call => call.StartsWith($"previous:{form}:"));
    }
}
