// 공장 부착 배선의 대여·부착·분리·Pop 위임을 검증한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// 공장 부착 배선을 검증하는 테스트 클래스다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.
public sealed class RawFactoryAttachTests
{
    /// <summary>188개 타입 목록을 만든다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();

    /// <summary>같은 풀 공장을 만든다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    private static (RawSidPool Pool, List<OriginalTypeRecord> Types, RawSquidFactory Factory) Scene()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var types = Types188();
        return (pool, types, new RawSquidFactory(pool, types));
    }

    /// <summary>부모·폼 체인 슬롯을 만든다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="parent">부모 번호다.</param>
    /// <param name="form">폼 번호다.</param>
    private static void FormChain(RawSidPool pool, int parent, int form)
    {
        Span<byte> head = pool.RawSlot(parent);
        head.Fill(0);
        head[10] = 1;
        head[6] = unchecked((byte)form);
        head[7] = unchecked((byte)(form >> 8));
        Span<byte> raw = pool.RawSlot(form);
        raw.Fill(0);
        raw[10] = 2;
        raw[11] = 8;
        raw[16] = unchecked((byte)parent);
        raw[17] = unchecked((byte)(parent >> 8));
        // 타입 2의 생성자 가상 표다
        raw[0] = 0x98;
        raw[1] = 0x20;
        raw[2] = 0x51;
        raw[3] = 0x00;
    }

    /// <summary>빈 깊이 상자를 거부하고 다른 상자 재연결을 거부한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    [Fact]
    public void BindFormPopDepthGuards()
    {
        (RawSidPool _, List<OriginalTypeRecord> _, RawSquidFactory factory) = Scene();
        Assert.Throws<ArgumentNullException>(() => factory.BindFormPopDepth(null!));
        var first = new UIntBox();
        factory.BindFormPopDepth(first);
        factory.BindFormPopDepth(first);
        Assert.Throws<ArgumentException>(() => factory.BindFormPopDepth(new UIntBox()));
    }

    /// <summary>빈 몸체·다른 풀 몸체를 거부한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    [Fact]
    public void BindCurrentFormPopGuards()
    {
        (RawSidPool pool, List<OriginalTypeRecord> _, RawSquidFactory factory) = Scene();
        Assert.Throws<ArgumentNullException>(() => factory.BindCurrentFormPop(null!));
        var foreign = new RawSquidForms(new RawSidPool(RawSurfaceStorage.PoolCapacity));
        Assert.Throws<ArgumentException>(() => factory.BindCurrentFormPop(foreign));
        var forms = new RawSquidForms(pool);
        factory.BindCurrentFormPop(forms);
    }

    /// <summary>묶은 깊이를 Pop 호출에 공유한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    [Fact]
    public void BindCurrentFormPopSharesDepth()
    {
        (RawSidPool pool, List<OriginalTypeRecord> _, RawSquidFactory factory) = Scene();
        var box = new UIntBox { Value = 7u };
        factory.BindFormPopDepth(box);
        var forms = new RawSquidForms(pool, null, (form, flags) => { });
        factory.BindCurrentFormPop(forms);
        int form = forms.Create(2, SidAllocation.Client);
        pool.RawSlot(form)[RawSidPool.StateOffset] |= 4;
        forms.Pop(form, 0u);
        Assert.Equal(7u, box.Value);
        Assert.Equal(0, pool.Slot(form)[RawSidPool.StateOffset] & 4);
    }

    /// <summary>현재 TYPE genus·기본 content로 부착한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachFormUsesTypeGenus()
    {
        (RawSidPool pool, List<OriginalTypeRecord> types, RawSquidFactory factory) = Scene();
        factory.BindFormPopDepth(new UIntBox());
        var forms = new RawSquidForms(pool, null, (form, flags) => { });
        int parent = pool.Allocate(SidAllocation.Server);
        int form = forms.Create(2, SidAllocation.Client);
        pool.RawSlot(form)[RawSidPool.StateOffset] |= 4;
        factory.AttachForm(form, parent, 0u);
        Assert.Equal(form, pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8));
        types[2].Flags2 = 0x44200u;
        int wild = forms.Create(2, SidAllocation.Client);
        pool.RawSlot(wild)[RawSidPool.StateOffset] |= 4;
        Assert.Throws<InvalidOperationException>(() => factory.AttachForm(wild, parent, 0u));
    }

    /// <summary>바인딩된 전송·content로 부착한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachFormUsesBoundTransmit()
    {
        (RawSidPool pool, List<OriginalTypeRecord> _, RawSquidFactory factory) = Scene();
        factory.BindFormPopDepth(new UIntBox());
        var sent = new List<(int Form, uint Flags)>();
        factory.BindFormAttach(() => 6u, (form, flags) => sent.Add((form, flags)));
        var forms = new RawSquidForms(pool, null, (form, flags) => { });
        int parent = pool.Allocate(SidAllocation.Server);
        int form = forms.Create(2);
        pool.RawSlot(form)[RawSidPool.StateOffset] |= 4;
        factory.AttachForm(form, parent, 0u);
        Assert.Single(sent);
        Assert.Equal(form, sent[0].Form);
    }

    /// <summary>공통 순서로 폼을 푼다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    [Fact]
    public void UnpopFormFollowsCommonOrder()
    {
        (RawSidPool pool, List<OriginalTypeRecord> _, RawSquidFactory factory) = Scene();
        FormChain(pool, 100, 101);
        factory.UnpopForm(101);
        Assert.Equal(4, pool.Slot(101)[11]);
        Assert.Equal(0, pool.Slot(100)[6] | (pool.Slot(100)[7] << 8));
        factory.BindFormDetach(_ => { });
        Assert.Throws<InvalidOperationException>(() => factory.UnpopForm(101));
    }

    /// <summary>묶은 깊이로 직접 Pop 한다. 2026-11-07 F2d 공장 부착 배선 이식에서 추가됐다.</summary>
    [Fact]
    public void PopFormUsesBoundDepth()
    {
        (RawSidPool pool, List<OriginalTypeRecord> _, RawSquidFactory factory) = Scene();
        var box = new UIntBox { Value = 3u };
        factory.BindFormPopDepth(box);
        var forms = new RawSquidForms(pool, null, (form, flags) => { });
        int form = forms.Create(2, SidAllocation.Client);
        pool.RawSlot(form)[RawSidPool.StateOffset] |= 4;
        factory.PopForm(form, 0u);
        Assert.Equal(3u, box.Value);
        Assert.Equal(0, pool.Slot(form)[RawSidPool.StateOffset] & 4);
    }
}
