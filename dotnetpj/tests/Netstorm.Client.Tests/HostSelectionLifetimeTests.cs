using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 세션의 살아 있는 실제 선택 객체 수명을 창 없이 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>RU-4c 살아 있는 선택 객체의 동일 owner·교체·만료·명시 실패를 검증한다. 0.994에서 추가됐다.</summary>
public sealed class HostSelectionLifetimeTests
{
    /// <summary>같은 장기 선택 owner와 독립 풀·난수로 세션을 만든다.</summary>
    private static HostSession Session(ClientGameplayGlobals game) => new(new Kernel(), new GameRandom(1), new CrtRandom(), sharedGameplay: game);

    /// <summary>같은 세션의 현재 188TYPE 표다.</summary>
    private static List<OriginalTypeRecord> Types() => Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToList();

    /// <summary>할당 성공으로 해석하지 않는 독립 슬롯 뷰다.</summary>
    private static Span<byte> Raw(RawSidPool pool, int sid = 50) => MemoryMarshal.CreateSpan(ref MemoryMarshal.GetReference(pool.Slot(sid)), pool.SlotStride);

    /// <summary>선택 한 번을 실제 표시 없이 수행하는 최소 장치다.</summary>
    private sealed class WorkingHooks
    {
        /// <summary>선택 통지 순서다.</summary>
        public readonly List<(int Sid, uint On)> Noticed = [];
        /// <summary>표시 갱신 순서다.</summary>
        public readonly List<int> Refreshed = [];
        /// <summary>위치 음향 이름 순서다.</summary>
        public readonly List<string> Sounds = [];
        /// <summary>진단 도달 횟수다.</summary>
        public int Dumped;
        /// <summary>기록할 실제 장치 묶음이다.</summary>
        public UserSelectionHooks Hooks => new()
        {
            Paused = () => 0u,
            Enabled = () => 1u,
            Blocked = () => 0u,
            Dump = _ => Dumped++,
            Highlight = _ => { },
            SelectNotice = (sid, on) => Noticed.Add((sid, on)),
            Refresh = sid => Refreshed.Add(sid),
            RangeType = _ => 0u,
            Dais = () => 999u,
            Sound = (x, y, name, flags) => Sounds.Add(name),
        };
    }

    /// <summary>유효 대상 50번의 실제 슬롯을 준비한다. TYPE0·선택 가능·extra0이다.</summary>
    private static void PrepareTarget(HostSession session)
    {
        Span<byte> raw = Raw(session.Pool);
        raw.Clear();
        raw[10] = 0;
        raw[11] = 0;
        raw[40] = 0;
    }

    /// <summary>같은 Pool/TYPE/선택 owner로 만들고 살아 있는 연결로 선택한다.</summary>
    [Fact]
    public void MakeSharesSameOwnersAndSelectsViaActiveLink()
    {
        var game = new ClientGameplayGlobals();
        var session = Session(game);
        var types = Types();
        var working = new WorkingHooks();
        var selection = session.MakeUserSelection(types, game.Selection.SelectionState(), working.Hooks);
        Assert.True(session.HasActiveSelection);
        Assert.NotNull(selection.SessionLifetime);
        PrepareTarget(session);
        Assert.Equal(50u, session.SelectViaActiveSelection(50, 0, 0));
        Assert.Equal(50u, game.Selection.Selected.Value);
        Assert.Contains((50, 1u), working.Noticed);
        Assert.Contains(50, working.Refreshed);
        Assert.Equal(["select.wav"], working.Sounds);
        Assert.Equal(1, working.Dumped);
    }

    /// <summary>다른 선택 owner의 상태는 설치 전 거부하고 연결을 만들지 않는다.</summary>
    [Fact]
    public void RejectsDifferentSelectionOwnerBeforeBinding()
    {
        var game = new ClientGameplayGlobals();
        var session = Session(game);
        var state = new UserSelectionState { Selected = new UIntBox(), MouseType = new UIntBox(), RememberedPoint = new uint[2], MultipleCount = new IntBox() };
        Assert.Throws<ArgumentException>(() => session.MakeUserSelection(Types(), state, new UserSelectionHooks()));
        Assert.False(session.HasActiveSelection);
    }

    /// <summary>새 선택이 살아 있는 연결을 교체하고 이전 선택으로 우회하지 않는다.</summary>
    [Fact]
    public void SecondMakeReplacesActiveLink()
    {
        var game = new ClientGameplayGlobals();
        var session = Session(game);
        var types = Types();
        session.MakeUserSelection(types, game.Selection.SelectionState(), new UserSelectionHooks { Paused = () => throw new InvalidOperationException("이전 선택 호출") });
        var working = new WorkingHooks();
        session.MakeUserSelection(types, game.Selection.SelectionState(), working.Hooks);
        PrepareTarget(session);
        Assert.Equal(50u, session.SelectViaActiveSelection(50, 0, 0));
        Assert.Equal(50u, game.Selection.Selected.Value);
    }

    /// <summary>선택을 반납하면 약한 연결이 만료되고 이후 선택은 명시 실패한다.</summary>
    [Fact]
    public void ReleasedSelectionExpiresActiveLink()
    {
        var game = new ClientGameplayGlobals();
        var session = Session(game);
        CreateAndDrop(session, Types(), game);
        for (int index = 0; index < 3 && session.HasActiveSelection; index++)
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        Assert.False(session.HasActiveSelection);
        var error = Assert.Throws<InvalidOperationException>(() => session.SelectViaActiveSelection(50, 0, 0));
        Assert.Contains("선택 수명", error.Message);
    }

    /// <summary>반납 범위를 벗어나면 강한 참조가 남지 않는다.</summary>
    private static void CreateAndDrop(HostSession session, List<OriginalTypeRecord> types, ClientGameplayGlobals game)
    {
        var selection = session.MakeUserSelection(types, game.Selection.SelectionState(), new UserSelectionHooks());
        Assert.True(session.HasActiveSelection);
        Assert.NotNull(selection);
    }

    /// <summary>살아 있는 선택이 없으면 선택 요청이 명시 실패한다.</summary>
    [Fact]
    public void SelectWithoutActiveThrowsExplicitly()
    {
        var game = new ClientGameplayGlobals();
        var session = Session(game);
        Assert.False(session.HasActiveSelection);
        var error = Assert.Throws<InvalidOperationException>(() => session.SelectViaActiveSelection(50, 0, 0));
        Assert.Contains("선택 수명", error.Message);
    }

    /// <summary>살아 있는 선택이 없으면 재확인은 찾기를 부르지 않고 조용히 반환한다.</summary>
    [Fact]
    public void RecheckWithoutActiveReturnsSilently()
    {
        var game = new ClientGameplayGlobals();
        var session = Session(game);
        session.RecheckActiveSelection((x, y, type) => throw new InvalidOperationException("찾기 호출"));
        Assert.False(session.HasActiveSelection);
    }

    /// <summary>살아 있는 선택의 재확인은 기억한 좌표/TYPE을 찾아 마우스 TYPE을 지운다.</summary>
    [Fact]
    public void RecheckViaActiveClearsMouseType()
    {
        var game = new ClientGameplayGlobals();
        var session = Session(game);
        var working = new WorkingHooks();
        session.MakeUserSelection(Types(), game.Selection.SelectionState(), working.Hooks);
        PrepareTarget(session);
        game.Selection.MouseType.Value = 7;
        var found = new List<(uint X, uint Y, uint Type)>();
        session.RecheckActiveSelection((x, y, type) => { found.Add((x, y, type)); return 50u; });
        Assert.Equal([(0u, 0u, 7u)], found);
        Assert.Equal(50u, game.Selection.Selected.Value);
        Assert.Equal(0u, game.Selection.MouseType.Value);
    }

    /// <summary>살아 있는 선택의 해제는 단일 선택을 지우고 이전 선택 통지를 남긴다.</summary>
    [Fact]
    public void ClearActiveSelectionClearsSingle()
    {
        var game = new ClientGameplayGlobals();
        var session = Session(game);
        var working = new WorkingHooks();
        session.MakeUserSelection(Types(), game.Selection.SelectionState(), working.Hooks);
        PrepareTarget(session);
        Assert.Equal(50u, session.SelectViaActiveSelection(50, 0, 0));
        session.ClearActiveSelection();
        Assert.Equal(0u, game.Selection.Selected.Value);
        Assert.Contains((50, 0u), working.Noticed);
    }
}
