// Forms Pop 바인딩의 장치 위임·기본값·깊이 규약을 검증한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// Forms Pop 바인딩을 검증하는 테스트 클래스다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.
public sealed class RawSquidFormsPopTests
{
    /// <summary>클라이언트 폼 장면을 만든다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    private static (RawSidPool Pool, RawSquidForms Forms, int Form) ClientScene()
    {
        var pool = new RawSidPool(32768);
        var forms = new RawSquidForms(pool, null, (form, flags) => { });
        int form = forms.Create(1, SidAllocation.Client);
        pool.RawSlot(form)[RawSidPool.StateOffset] |= 4;
        return (pool, forms, form);
    }

    /// <summary>서버 폼 장면을 만든다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    private static (RawSidPool Pool, RawSquidForms Forms, int Form) ServerScene()
    {
        var pool = new RawSidPool(32768);
        var forms = new RawSquidForms(pool, null, (form, flags) => { });
        int form = forms.Create(1);
        pool.RawSlot(form)[RawSidPool.StateOffset] |= 4;
        return (pool, forms, form);
    }

    /// <summary>빈 묶음을 거부한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BindPopRequiresHooks()
    {
        (RawSidPool _, RawSquidForms forms, int _) = ClientScene();
        Assert.Throws<ArgumentNullException>(() => forms.BindPop(null!));
    }

    /// <summary>빈 바인딩은 풀 기본 장치로 Pop 한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundEmptyHooksUsePoolDevices()
    {
        (RawSidPool pool, RawSquidForms forms, int form) = ClientScene();
        forms.BindPop(new FormPopHooks());
        forms.Pop(form, 0u);
        Assert.Equal(0, pool.Slot(form)[RawSidPool.StateOffset] & 4);
    }

    /// <summary>서버 전송은 고정 인자로 기존 전송 경계에 넘긴다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundTransmitUsesFormTransmit()
    {
        (RawSidPool _, RawSquidForms forms, int form) = ServerScene();
        var calls = new List<(int Form, uint First, uint Second, uint Third, uint Flags)>();
        forms.BindPop(new FormPopHooks
        {
            Transmit = (sid, first, second, third, flags) => calls.Add((sid, first, second, third, flags)),
        });
        forms.Pop(form, 8u);
        Assert.Single(calls);
        Assert.Equal((form, 0xffffffffu, 0xffffffffu, 0u, 8u), calls[0]);
    }

    /// <summary>전송 경계가 없으면 서버 전송 앞에서 거부한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundMissingTransmitRejects()
    {
        var pool = new RawSidPool(32768);
        var forms = new RawSquidForms(pool);
        int form = forms.Create(1);
        pool.RawSlot(form)[RawSidPool.StateOffset] |= 4;
        forms.BindPop(new FormPopHooks());
        Assert.Throws<InvalidOperationException>(() => forms.Pop(form, 8u));
    }

    /// <summary>깊이를 되돌리지 않으면 보고한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundDepthImbalanceReportsAssertion()
    {
        (RawSidPool _, RawSquidForms forms, int form) = ClientScene();
        var notes = new List<string>();
        forms.BindPop(new FormPopHooks
        {
            PostPop = (_, _, _) => { },
            Assertion = (expression, _, _) => notes.Add(expression),
        });
        forms.Pop(form, 0u);
        Assert.Single(notes);
        Assert.Contains("curPostPop == basePostPop", notes[0]);
    }

    /// <summary>모르는 가상 표는 후처리 선택에서 거부한다. 2026-11-07 F2d Form Pop 바인딩 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundPostPopUnknownTableThrows()
    {
        (RawSidPool pool, RawSquidForms forms, int form) = ClientScene();
        Span<byte> raw = pool.RawSlot(form);
        raw[0] = 0xef;
        raw[1] = 0xbe;
        raw[2] = 0xad;
        raw[3] = 0xde;
        forms.BindPop(new FormPopHooks());
        Assert.Throws<InvalidOperationException>(() => forms.Pop(form, 0u));
    }
}
