using System.Buffers;
using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 복사 값이 아닌 실제 Client DWORD의 별칭/실패 접두를 검사한다.

/// <summary>독립 API 보존과 재생성 Screen의 실제 공유 저장소를 검사한다.</summary>
public sealed class SharedPaletteLoadedTests
{
    /// <summary>UIntBox가 값 조회/쓰기를 수행했는지 실제 빌린 Memory의 접근 횟수로 관측한다.</summary>
    private sealed class CountedMemory : MemoryManager<byte>
    {
        /// <summary>외부 DWORD의 실제4BYTE다.</summary>
        public byte[] Bytes { get; } = new byte[4];
        /// <summary>명시 관측 이후 Span 접근 횟수다.</summary>
        public int Accesses { get; private set; }
        /// <summary>동일 물리를 빌려 주며 실제 Value 조회/기록을 센다.</summary>
        public override Span<byte> GetSpan() { Accesses++; return Bytes; }
        /// <summary>이 검사는 고정 주소를 요구하지 않는다.</summary>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException();
        /// <summary>Pin을 선택하지 않으므로 별도 반납할 것이 없다.</summary>
        public override void Unpin() { }
        /// <summary>검사 소유 managed4BYTE만 남으므로 OS 자원 정리가 없다.</summary>
        protected override void Dispose(bool disposing) { }
    }

    /// <summary>생성/인계/identity 거부는 전체 DWORD를 미리 읽거나 복사하지 않는다.</summary>
    [Fact]
    public void ConstructionAndIdentityUseNoValueReadOrWrite()
    {
        using var memory = new CountedMemory();
        BinaryPrimitives.WriteUInt32LittleEndian(memory.Bytes, 0x80000000u);
        var box = new UIntBox(memory.Memory); int before = memory.Accesses;
        var palette = new ClientPaletteGlobals(box); var screen = new Screen(4, 2, box);
        var raw = new ColorSortStartupGlobals(box); palette.RequireScreen(screen);
        Assert.Throws<ArgumentException>(() => palette.RequireScreen(new Screen(4, 2)));
        Assert.Equal(before, memory.Accesses); Assert.Same(box, screen.PaletteLoadedOwner); Assert.Same(box, raw.PaletteLoadedOwner);
        Assert.Equal(0x80000000u, screen.PaletteLoaded); raw.PaletteLoaded = 0xf1234567u;
        Assert.Equal(0xf1234567u, screen.PaletteLoaded); Assert.Equal(0xf1234567u, BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes));
    }

    /// <summary>기존2인자 API는 독립0을 유지하고 명시 null은 초기 저장소로 대체하지 않는다.</summary>
    [Fact]
    public void LegacyScreensAreIndependentAndExplicitNullIsRejected()
    {
        var first = new Screen(4, 2); var second = new Screen(4, 2);
        Assert.Equal(0u, first.PaletteLoaded); Assert.Equal(0u, second.PaletteLoaded);
        Assert.NotSame(first.PaletteLoadedOwner, second.PaletteLoadedOwner);
        first.PaletteLoadedOwner.Value = 7; Assert.Equal(0u, second.PaletteLoaded);
        Assert.Throws<ArgumentNullException>(() => new Screen(4, 2, null!));
        Assert.Throws<ArgumentNullException>(() => new ClientPaletteGlobals(null!));
        Assert.Throws<ArgumentNullException>(() => new ColorSortStartupGlobals(null!));
    }

    /// <summary>기존 Raw 부모는 진입 시점의 같은 DWORD를 검사하고 진단 복귀/실패의 원문 prefix를 보존한다.</summary>
    [Fact]
    public void RawColorSortReadsCurrentClientDwordAtExistingAssertionPosition()
    {
        var palette = new ClientPaletteGlobals(); var screen = new Screen(4, 2, palette.Loaded);
        var raw = new ColorSortStartupGlobals(palette.Loaded); int assertions = 0, allocations = 0;
        var hooks = new ColorSortStartupHooks
        {
            Assertion = line => { Assert.Equal(451u, line); assertions++; palette.Loaded.Value = 0x80000000u; },
            Allocate = _ => { allocations++; return 0; },
        };
        RawColorSortStartup.InitializeColorSort(7, raw, hooks);
        Assert.Equal(1, assertions); Assert.Equal(3, allocations); Assert.Equal(0x80000000u, screen.PaletteLoaded);
        RawColorSortStartup.InitializeColorSort(7, raw, hooks); Assert.Equal(1, assertions); Assert.Equal(6, allocations);
        palette.Loaded.Value = 0; raw.Primary = 9; var failure = new InvalidOperationException("loaded-assert");
        hooks.Assertion = _ => { palette.Loaded.Value = 0xfedcba98u; throw failure; };
        Assert.Same(failure, Assert.Throws<InvalidOperationException>(() => RawColorSortStartup.InitializeColorSort(7, raw, hooks)));
        Assert.Equal(9u, raw.Primary); Assert.Equal(6, allocations); Assert.Equal(0xfedcba98u, screen.PaletteLoaded);
    }

    /// <summary>모드 정상/같음/거부와 새 Screen 생성은 같은 Client의 loaded를 초기화하지 않는다.</summary>
    [Fact]
    public void SetModeAndRecreationPreserveClientDwordWithoutCopyingPaletteArrays()
    {
        var palette = new ClientPaletteGlobals(); palette.Loaded.Value = 0x81234567;
        var first = new Screen(4, 2, palette.Loaded);
        first.SetPalette(17, 1, [new ScreenColor(3, 5, 7, 9)], true);
        Assert.True(first.SetMode(ScreenMode.BackBufferDib)); Assert.True(first.SetMode(ScreenMode.BackBufferDib));
        Assert.False(first.SetMode(ScreenMode.Animating)); Assert.Equal(0x81234567u, palette.Loaded.Value);
        Assert.True(first.SetMode(ScreenMode.FullScreen | ScreenMode.Clouds)); Assert.Equal(0x81234567u, palette.Loaded.Value);
        var second = new Screen(8, 4, palette.Loaded); palette.RequireScreen(second);
        Assert.Equal(0x81234567u, second.PaletteLoaded); Assert.Equal(default, second.Palette[17]);
        Assert.Equal(new ScreenColor(3, 5, 7, 9), first.Palette[17]);
        palette.Loaded.Value = 0; Assert.Equal(0u, first.PaletteLoaded); Assert.Equal(0u, second.PaletteLoaded);
    }

    /// <summary>두 API는 같은 관문 순서이며 live callback의 전체 DWORD 변경을 정상/실패 뒤 보존한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void LiveClearPreservesCallbackMutationAfterReturnOrThrow(bool fail)
    {
        var box = new UIntBox(); var view = new ColorSortStartupGlobals(box); var calls = new List<string>();
        var failure = new InvalidOperationException("wait");
        var hooks = new ScreenPaletteApplyHooks
        {
            Lock = () => { Assert.Equal(1u, view.PaletteLoaded); calls.Add("L"); },
            FillBlack = () => calls.Add("B"), Unlock = () => calls.Add("U"),
            WaitVerticalBlank = () => { calls.Add("W"); view.PaletteLoaded = 0xfedcba98u; if (fail) throw failure; },
            PresentPreservingClip = () => calls.Add("P"), Flip = () => calls.Add("F"),
        };
        if (fail) Assert.Same(failure, Assert.Throws<InvalidOperationException>(() => ScreenPaletteApply.ApplyScreenPaletteClear(1, 1, box, hooks)));
        else ScreenPaletteApply.ApplyScreenPaletteClear(1, 1, box, hooks);
        Assert.Equal(fail ? "LBUW" : "LBUWPF", string.Concat(calls)); Assert.Equal(0xfedcba98u, box.Value);
        uint oldApi = 0x80000000u; calls.Clear();
        ScreenPaletteApply.ApplyScreenPaletteClear(0, 1, ref oldApi, new());
        Assert.Equal(1u, oldApi); Assert.Empty(calls);
    }
}
