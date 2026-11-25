using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 세션 저장 쓰기·읽기 조립과 실제 소비자 경로 진입을 검증한다.
// 2026-11-26 1.061에서 SL-8용으로 추가했다.
public sealed class HostSavedStateTests
{
    /// <summary>구독자를 가진 게임으로 독립 세션을 만든다.</summary>
    /// <returns>세션과 게임이다.</returns>
    private static (HostSession Session, ClientGameplayGlobals Game) Scene()
    {
        var subscribers = new ClientSubscriberGlobals();
        subscribers.MySubscriber.Value = 5;
        var game = new ClientGameplayGlobals(subscribers);
        var session = new HostSession(new Kernel(), new GameRandom(71u), new CrtRandom(), sharedGameplay: game);
        session.MissionPlayer.PlayerKnown = 1;
        session.MissionPlayer.BattleOwner = 1;
        return (session, game);
    }

    /// <summary>쓰기·읽기 갈고리를 메모리 스트림에 연결한다.</summary>
    /// <returns>갈고리 묶음과 기록이다.</returns>
    private static (SavedStateWriteHooks Write, SavedStateReadHooks Read, List<byte> Stream, List<string> Opens, Func<int> Closes) MemoryHooks(byte[] fort, string path)
    {
        var stream = new List<byte>();
        var opens = new List<string>();
        int closes = 0;
        var write = new SavedStateWriteHooks
        {
            CurrentFort = () => fort,
            Diagnose = () => { },
            Open = (first, second) => opens.Add($"{first}:{second}"),
            Write = (bytes, _) => stream.AddRange(bytes),
            Close = () => closes++,
            WallSeconds = () => 100.7,
            LocalSp = () => 3.5f,
            Assertion = (_, _) => throw new InvalidOperationException("예상 밖 저장 진단"),
        };
        var read = new SavedStateReadHooks
        {
            Path = () => System.Text.Encoding.Latin1.GetBytes(path + "\0"),
            Exists = name => File.Exists(name) ? 1 : 0,
            Open = (_, _, _, _) => 7u,
            Read = (_, _, _, _, _) => 0u,
            Close = (_, _) => { },
            Pending = _ => { },
            InitializeCore = _ => { },
            Metadata = (_, _, _) => { },
        };
        return (write, read, stream, opens, () => closes);
    }

    /// <summary>미조립 조회·누락 장치·장치 중복 연결을 거부한다.</summary>
    [Fact]
    public void MissingDevicesAndDuplicateBindingFail()
    {
        var (session, _) = Scene();
        Assert.Throws<InvalidOperationException>(() => session.SavedStateWriteGlobals);
        Assert.Throws<InvalidOperationException>(() => session.SavedStateWriteHooks);
        Assert.Throws<InvalidOperationException>(() => session.SavedStateReadGlobals);
        Assert.Throws<InvalidOperationException>(() => session.SavedStateReadHooks);
        var (hooks, _, _, _, _) = MemoryHooks(new byte[300], "x");
        (_, var readHooks, _, _, _) = MemoryHooks(new byte[300], "x");
        Assert.Throws<ArgumentNullException>(() => session.BuildSavedStateDevices(null!, readHooks));
        Assert.Throws<ArgumentNullException>(() => session.BuildSavedStateDevices(hooks, null!));
        session.BuildSavedStateDevices(hooks, readHooks);
        Assert.Throws<InvalidOperationException>(() => session.BuildSavedStateDevices(hooks, readHooks));
    }

    /// <summary>세션 실제 값이 쓰기 전역에 그대로 들어간다.</summary>
    [Fact]
    public void SessionValuesFillWriteGlobals()
    {
        var (session, game) = Scene();
        var (hooks, _, _, _, _) = MemoryHooks(new byte[300], "x");
        (_, var readHooks, _, _, _) = MemoryHooks(new byte[300], "x");
        session.BuildSavedStateDevices(hooks, readHooks);
        SavedStateWriteGlobals globals = session.SavedStateWriteGlobals;
        Assert.NotNull(globals.MissionPost); Assert.False(globals.MissionPost!());
        Assert.NotNull(globals.PlayerKnown); Assert.True(globals.PlayerKnown!());
        Assert.NotNull(globals.Checking); Assert.False(globals.Checking!());
        Assert.NotNull(globals.LocalOwner); Assert.Equal(1u, globals.LocalOwner!());
        Assert.NotNull(globals.MySubscriber); Assert.Equal(5u, globals.MySubscriber!());
        var crtBox = new UIntBox { Value = 1u };
        var modeBox = new UIntBox { Value = 0u };
        var seed = new RawGameRandomSeed(session.Game, crtBox, modeBox, () => 946684800);
        session.BindSpReseed(() => seed.Reseed());
        session.Sp.Set(1u, BitConverter.SingleToUInt32Bits(3.5f));
        Assert.Equal(3.5f, session.SavedStateLocalSp());
        session.MissionPlayer.PlayerKnown = 0;
        Assert.Equal(0f, session.SavedStateLocalSp());
        session.MissionPlayer.PlayerKnown = 1;
        session.Sp.Set(1u, BitConverter.SingleToUInt32Bits(3.5f));
        // 이름 NUL 앞부분만 복사 대상이 되므로 기록에 직접 둔다.
        session.PlayerRecords[180 + 12] = (byte)'Y';
        session.PlayerRecords[180 + 13] = (byte)'o';
        Assert.NotNull(globals.Handle);
        Assert.Equal((byte)'Y', globals.Handle!()[0]);
        Assert.Equal((byte)'o', globals.Handle!()[1]);
        Assert.Equal(0, globals.Handle!()[2]);
        Assert.Same(session.PlayerRecords, globals.Players!());
        Assert.NotNull(globals.SetRestoreOwner); Assert.NotNull(globals.SetRestoreSubscriber);
        globals.SetRestoreOwner!(7); globals.SetRestoreSubscriber!(9);
        Assert.Equal(7u, session.SavedStateRestoreOwner.Value);
        Assert.Equal(9u, session.SavedStateRestoreSubscriber.Value);
        Assert.Same(session.SavedStatePreviousHandle, globals.PreviousHandle);
        Assert.NotNull(globals.Tail); Assert.Equal(3, globals.Tail!.Length);
        Assert.All(globals.Tail!, tail => Assert.Throws<InvalidOperationException>(() => tail!()));
        session.LocalState.LoginEnd.Value = 2.5;
        Assert.NotNull(globals.LoginEnd); Assert.Equal(2.5, globals.LoginEnd!());
    }

    /// <summary>전투 중 기한이 지나면 한 번 저장하고 10초를 예약한다.</summary>
    [Fact]
    public void PollSavesOncePerDeadline()
    {
        var (session, _) = Scene();
        var (hooks, _, stream, opens, closes) = MemoryHooks(new byte[300], "x");
        var tail = new Func<uint>?[3] { () => 0x11111111u, () => 0x22222222u, () => 0x33333333u };
        var custom = new SavedStateWriteGlobals
        {
            MissionPost = () => false,
            PlayerKnown = () => true,
            Checking = () => true,
            LocalOwner = () => 1u,
            MySubscriber = () => 2u,
            Handle = () => [(byte)'Y', (byte)'o', 0],
            Players = PlayerRecords9,
            SetRestoreOwner = _ => { },
            SetRestoreSubscriber = _ => { },
            PreviousHandle = new byte[64],
            Tail = tail,
            LoginEnd = () => 0.0,
        };
        (_, var readHooks, _, _, _) = MemoryHooks(new byte[300], "x");
        session.BuildSavedStateDevices(hooks, readHooks, custom, null);
        session.PollSavedStateWrite(false);
        Assert.Empty(stream);
        session.PollSavedStateWrite(true);
        Assert.NotEmpty(stream);
        Assert.Equal(["2:1"], opens);
        int count = stream.Count;
        session.PollSavedStateWrite(true);
        Assert.Equal(count, stream.Count);
        Assert.Equal(1, closes());
    }

    /// <summary>없는 파일은 0으로 정상 복귀한다.</summary>
    [Fact]
    public void MissingFileReturnsZero()
    {
        var (session, _) = Scene();
        string missing = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"), "!state.dat");
        var (hooks, _, _, _, _) = MemoryHooks(new byte[300], missing);
        var (_, readHooks, _, _, _) = MemoryHooks(new byte[300], missing);
        session.BuildSavedStateDevices(hooks, readHooks);
        var scratch = new byte[SavedStateFormat.ScratchBytes];
        Assert.Equal(0, session.ReadSavedState(scratch));
    }

    /// <summary>현재 9기록을 만든다.</summary>
    private static byte[] PlayerRecords9()
    {
        var records = new byte[9 * 180];
        for (int player = 0; player < 9; ++player)
        {
            System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(records.AsSpan(player * 180 + 0x2c), 0x1000u + (uint)player);
            System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(records.AsSpan(player * 180 + 0x50), 0x40u);
            System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(records.AsSpan(player * 180 + 0x58), 0x2000u + (uint)player);
        }
        return records;
    }
}
