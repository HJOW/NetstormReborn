using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 기계어 독립 관찰의 입력과 기대출력은 재생산하지 않고 원문 그대로 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>장면 음악 감독의9339개원문입력과현재battle재조회/실패prefix를검증한다.</summary>
public sealed class SceneMusicTests
{
    /// <summary>double원문비트를16진수로기록한다.</summary>
    private static string Bits(double value) => BitConverter.DoubleToUInt64Bits(value).ToString("x16", CultureInfo.InvariantCulture);
    /// <summary>16진double비트를호스트double에손실없이설치한다.</summary>
    private static double Double(string bits) => BitConverter.UInt64BitsToDouble(ulong.Parse(bits, NumberStyles.HexNumber, CultureInfo.InvariantCulture));
    /// <summary>원문이름BYTE를대조문자열로기록한다.</summary>
    private static string Hex(string text) => text.Length == 0 ? "-" : Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();

    /// <summary>명시입력을소비하고끝나면마지막값을반복하는독립장치다.</summary>
    private sealed class Feed
    {
        /// <summary>현재명시입력목록이다.</summary>
        private readonly string[] _values;
        /// <summary>실제소비횟수다.</summary>
        private int _used;
        /// <summary>세미콜론으로명시한입력을읽는다.</summary>
        public Feed(string text = "") => _values = text.Length == 0 ? [] : text.Split(';');
        /// <summary>빈입력은기본값을사용하되호출은한번소비한다.</summary>
        public string Take(string fallback) { string value = _values.Length == 0 ? fallback : _values[Math.Min(_used, _values.Length - 1)]; _used++; return value; }
    }

    /// <summary>각판본3113행의모든호출사건과마지막전역10칸을전수대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void EveryOriginalSceneSequenceMatches(string edition)
    {
        string[][] all = X86Fixture.Read("scenemusic"); Assert.Equal(9339, all.Length);
        string[][] rows = all.Where(row => row[0] == edition).ToArray(); Assert.Equal(3113, rows.Length);
        X86Fixture.CheckRows(rows, Replay);
    }

    /// <summary>한행의각설정과호출을정확한순서로음악본체에전달하고원문기대값과비교한다.</summary>
    private static bool Replay(string[] row)
    {
        var state = new SceneMusicState(); new uint[] { 0x1111, 0x2222, 0x3333, 0x4444 }.CopyTo(state.Tints, 0); var random = new GameRandom();
        byte[] current = new byte[256]; double duration = 0; var events = new List<string>(); var tokens = new List<string>();
        var clock = new Feed(); var durations = new Feed(); var sacrifice = new Feed(); var waiting = new Feed(); var option = new Feed();
        // 조회마다현재실제256BYTE의첫NUL앞을다시읽는다.
        string Name() { int end = Array.IndexOf(current, (byte)0); return Encoding.Latin1.GetString(current, 0, end < 0 ? 256 : end); }
        var music = new SceneMusic(state, random, new()
        {
            WallSeconds = () => { events.Add("T"); return Double(clock.Take(Bits(1000))); }, CurrentName = Name,
            Select = name =>
            {
                events.Add("S:" + Hex(name)); byte[] bytes = Encoding.Latin1.GetBytes(name); Array.Clear(current, 0, 255); bytes.AsSpan(0, Math.Min(255, bytes.Length)).CopyTo(current);
                string next = durations.Take("keep"); if (next != "keep") duration = Double(next);
            }, Duration = () => duration,
            Sacrificing = player => { events.Add("C:" + player); return sacrifice.Take("0") == "1"; },
            WaitingRoom = () => { events.Add("W"); return waiting.Take("0") == "1"; }, Refresh = () => events.Add("R"),
            AscendancyPalette = () => { events.Add("O"); return option.Take("0") == "1"; },
            LoadPalette = name => { events.Add("J:" + Hex(name)); events.Add("A"); }, ThunderFlash = () => events.Add("K"), WeatherSound = name => events.Add("N:" + Hex(name)),
        });
        // 명시전역설정은호출사건으로세지않고나머지연산은호출별trace를보존한다.
        foreach (string operation in row[2].Split('|'))
        {
            string[] op = operation.Split(':'); bool called = false; string result = "-";
            switch (op[0])
            {
                case "i":
                    uint value = uint.Parse(op[2], CultureInfo.InvariantCulture);
                    switch (op[1])
                    {
                        case "index": state.Index = unchecked((int)value); break; case "result": state.ResultState = value; break;
                        case "battle": state.Battle = value; break; case "players": state.PlayersReady = value; break; case "local": state.LocalPlayer = value; break;
                        case "tint": state.Tint = value; break; case "dirty": state.PaletteDirty = value; break; default: throw new InvalidDataException("음악fixture전역종류");
                    }
                    break;
                case "t": state.Tints[int.Parse(op[1])] = uint.Parse(op[2], CultureInfo.InvariantCulture); break;
                case "f":
                    double time = Double(op[2]); if (op[1] == "fanfare") state.FanfareEnd = time; else if (op[1] == "defeat") state.DefeatEnd = time; else state.SongEnd.Value = time;
                    break;
                case "cur": Array.Clear(current); X86Fixture.Bytes(op[1]).CopyTo(current, 0); break;
                case "rng": random.SetState(uint.Parse(op[1], CultureInfo.InvariantCulture)); break;
                case "clk": clock = new(op[1]); break; case "durs": durations = new(op[1]); break;
                case "sac": sacrifice = new(op[1]); break; case "wait": waiting = new(op[1]); break; case "opt": option = new(op[1]); break;
                case "lock": called = true; result = music.ResultMusicLocked() ? "1" : "0"; break;
                case "req": called = true; music.Request(X86Fixture.Text(op[1])); break;
                case "next": called = true; music.Next(); break; case "frame": called = true; music.Frame(); break;
                case "start": called = true; music.Start(); break; case "weather": called = true; music.ApplyWeather(); break;
                default: throw new InvalidDataException("음악fixture연산종류");
            }
            if (!called) continue;
            tokens.Add((events.Count == 0 ? "-" : string.Join(';', events)) + '=' + result); events.Clear();
        }
        string[] snapshot = [unchecked((uint)state.Index).ToString(CultureInfo.InvariantCulture), Bits(state.FanfareEnd), Bits(state.DefeatEnd), Bits(state.SongEnd.Value), state.Tint.ToString(CultureInfo.InvariantCulture), state.PaletteDirty.ToString(CultureInfo.InvariantCulture), random.State().ToString(CultureInfo.InvariantCulture), Hex(Name()), Bits(duration), state.ResultState.ToString(CultureInfo.InvariantCulture)];
        return string.Join('|', tokens) == row[3] && snapshot.SequenceEqual(row[4..]);
    }

    /// <summary>시각관문이만료/NaN이면현재battle조회자체를실행하지않는다.</summary>
    [Theory]
    [InlineData(99, 100)]
    [InlineData(double.NaN, 0)]
    [InlineData(100, double.NaN)]
    public void FrameGatesCurrentModeRead(double now, double end)
    {
        var state = new SceneMusicState { CurrentBattle = () => throw new IOException("선행현재모드조회") }; state.SongEnd.Value = end;
        var music = new SceneMusic(state, new(), Hooks(() => now)); music.Frame();
    }
    /// <summary>독립검사의선택되지않은하위는명시예외이며벽시각만공급한다.</summary>
    private static SceneMusicHooks Hooks(Func<double> clock) => new()
    {
        WallSeconds = clock, CurrentName = () => throw new IOException("미선택이름"), Select = _ => throw new IOException("미선택재생"), Duration = () => throw new IOException("미선택길이"),
        Sacrificing = _ => throw new IOException("미선택희생"), WaitingRoom = () => throw new IOException("미선택대기"), Refresh = () => throw new IOException("미선택화면"),
        AscendancyPalette = () => throw new IOException("미선택옵션"), LoadPalette = _ => throw new IOException("미선택팔레트"), ThunderFlash = () => throw new IOException("미선택번개"), WeatherSound = _ => throw new IOException("미선택효과음"),
    };
    /// <summary>현재조회와fallback의수명/값이분리되고필수하위미연결은생성시거부된다.</summary>
    [Fact]
    public void CurrentModeUsesLiveProviderAndExplicitFallback()
    {
        uint mode = 17; var state = new SceneMusicState { Battle = 9 }; var music = new SceneMusic(state, new(), Hooks(() => 0)); Assert.Equal(9u, music.CurrentBattle());
        state.CurrentBattle = () => mode; Assert.Equal(17u, music.CurrentBattle()); mode = 0; Assert.Equal(0u, music.CurrentBattle()); Assert.Equal(9u, state.Battle);
        state.CurrentBattle = null; Assert.Equal(9u, music.CurrentBattle()); Assert.Throws<ArgumentException>(() => new SceneMusic(state, new(), new()));
    }

    /// <summary>기본채널+18의음수0/NaNpayload/길이를그대로읽고이웃바이트는변경하지않는다.</summary>
    [Fact]
    public void ChannelDurationReadsExactEightBytesWithoutNormalizingBits()
    {
        byte[] raw = Enumerable.Repeat((byte)0xa5, 40).ToArray();
        // 원문8BYTE만차례로쓰고전체물리를읽기전후대조한다.
        foreach (ulong bits in new ulong[] { 0x8000000000000000, 0x7ff8000012345678, BitConverter.DoubleToUInt64Bits(214.75) })
        {
            BinaryPrimitives.WriteUInt64LittleEndian(raw.AsSpan(0x18, 8), bits); byte[] before = raw.ToArray();
            Assert.Equal(bits, BitConverter.DoubleToUInt64Bits(SceneMusic.ChannelDuration(raw))); Assert.Equal(before, raw);
        }
        Assert.Throws<ArgumentException>(() => SceneMusic.ChannelDuration(new byte[31]));
    }

    /// <summary>최초곡선택효과가현재battle을바꾸면만료된내부Frame은옛Start값대신현재값을다시읽는다.</summary>
    [Fact]
    public void StartRereadsCurrentModeAfterSelectedSongChangesIt()
    {
        uint currentBattle = 0; int reads = 0; string name = "before.mus"; var times = new Queue<double>([10, 200, 201]); var events = new List<string>(); var random = new GameRandom(1);
        var state = new SceneMusicState { Battle = 1, CurrentBattle = () => { reads++; return currentBattle; } };
        var music = new SceneMusic(state, random, new()
        {
            WallSeconds = () => times.Dequeue(), CurrentName = () => name, Duration = () => 0,
            Select = selected => { name = selected; events.Add(selected); if (selected == SceneMusic.MenuMusic) currentBattle = 1; },
            Sacrificing = _ => throw new IOException("미준비Player희생판정"), WaitingRoom = () => throw new IOException("현재전투는대기실아님"),
            Refresh = () => events.Add("refresh"), AscendancyPalette = () => false, LoadPalette = _ => throw new IOException("옵션0팔레트"),
            ThunderFlash = () => throw new IOException("바람곡번개"), WeatherSound = _ => throw new IOException("바람곡효과음"),
        });
        music.Start(); Assert.Equal(2, reads); Assert.Equal(new[] { "ser22.mus", "wind22.mus", "refresh" }, events); Assert.Equal(0, state.Index); Assert.Equal(1u, random.State()); Assert.Equal(381.0, state.SongEnd.Value);
    }
}
