using System.Globalization;
using System.Text;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 재생/제어의 명시 장치 입력과 전체 이름 표·위치 계산을 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>세 판본의 soundplay14472·soundcontrol3189관측을 같은 실제 재생 계층에 공급한다.</summary>
public sealed partial class SoundPlayerOracleTests
{
    /// <summary>원본 관측기의 명시 장치 버퍼 핸들 기준이다.</summary>
    private const uint BufferBase = 0x16001000;
    /// <summary>재생 판단을 구현하지 않고 실제 장치 호출/반환 입력만 제공하는 원본 관측 장치다.</summary>
    private sealed class Device
    {
        /// <summary>실제 대체 장치의 버퍼별 현재 상태다.</summary>
        public readonly List<uint> Status = [];
        /// <summary>두 번의 상태 조회 사이에 바뀌는 명시 반환 입력이다.</summary>
        public readonly Dictionary<int, Queue<uint>> Queues = [];
        /// <summary>호출/인자를 원본 순서로 기록한 토큰이다.</summary>
        private readonly List<string> _events = [];
        /// <summary>적재 결과b=새 버퍼/s=소리없음/그 밖=0이다.</summary>
        public char LoadKind = 'b';
        /// <summary>원본 적재가 입력으로 돌려주는 감쇠다.</summary>
        public int LoadAttenuation;
        /// <summary>원본 복제 호출의 HRESULT 입력이다.</summary>
        public int CopyResult;
        /// <summary>원본 음량 설정의 HRESULT 입력이다.</summary>
        public int GainResult;
        /// <summary>원본 좌우 설정의 HRESULT 입력이다.</summary>
        public int BalanceResult;
        /// <summary>명시 장치 상태 조회가 현재 전역을 바꾸는 입력 효과다.</summary>
        public Action? AfterStatus;
        /// <summary>실제 재생 호출이 현재 전역을 바꾸는 입력 효과다.</summary>
        public Action? AfterPlay;
        /// <summary>원본 관측기와 같은16BYTE 간격의 실제 대체 버퍼를 추가한다.</summary>
        private uint Create() { Status.Add(0); return BufferBase + 16u * (uint)(Status.Count - 1); }
        /// <summary>상태표의 실제 대체 버퍼 위치만 허용한다.</summary>
        private int Index(uint buffer)
        {
            if (buffer < BufferBase || (buffer - BufferBase) % 16 != 0 || (buffer - BufferBase) / 16 >= Status.Count) throw new ArgumentOutOfRangeException(nameof(buffer), "알 수 없는 관측 버퍼");
            return (int)((buffer - BufferBase) / 16);
        }
        /// <summary>원본 관측기와 같은#순번 또는 실제 숫자 표기다.</summary>
        private string Ordinal(uint buffer) => buffer >= BufferBase && (buffer - BufferBase) % 16 == 0 && (buffer - BufferBase) / 16 < Status.Count ? $"#{(buffer - BufferBase) / 16}" : buffer.ToString(CultureInfo.InvariantCulture);
        /// <summary>장치 호출만 현재 상태/입력대로 기록하며 재생기의 판단을 대신하지 않는다.</summary>
        public SoundDeviceHooks Hooks() => new()
        {
            Status = buffer => { int index = Index(buffer); if (Queues.TryGetValue(index, out Queue<uint>? queue) && queue.Count != 0) Status[index] = queue.Dequeue(); _events.Add($"Q:{Ordinal(buffer)}"); AfterStatus?.Invoke(); return Status[index]; },
            Play = (buffer, flags) => { Status[Index(buffer)] = 1u | ((flags & 1) != 0 ? 4u : 0u); _events.Add($"P:{Ordinal(buffer)}:0:0:{flags}"); AfterPlay?.Invoke(); },
            SetPosition = (buffer, value) => _events.Add($"C:{Ordinal(buffer)}:{value}"),
            SetVolume = (buffer, value) => { _events.Add($"V:{Ordinal(buffer)}:{value}"); return GainResult; },
            SetPan = (buffer, value) => { _events.Add($"N:{Ordinal(buffer)}:{value}"); return BalanceResult; },
            Stop = buffer => { Status[Index(buffer)] &= ~5u; _events.Add($"X:{Ordinal(buffer)}"); },
            Duplicate = (uint original, ref uint copy) => { _events.Add($"D:{Ordinal(original)}"); if (CopyResult >= 0) copy = Create(); return CopyResult; },
            Load = name => { var result = new SoundLoad(LoadKind == 'b' ? Create() : LoadKind == 's' ? SoundConstants.SilentBuffer : 0, LoadAttenuation); _events.Add($"F:{name}"); return result; },
            Log = (channel, text) => _events.Add($"G:{channel}:{text.TrimEnd('\n', ' ')}"),
        };
        /// <summary>원본 보고 줄 번호만 같은 사건 순서에 기록한다.</summary>
        public void Report(string expression, int line) => _events.Add($"!{line}");
        /// <summary>이번 연산의 사건을 반환하고 다음 관측용으로 비운다.</summary>
        public string Take() { string text = _events.Count == 0 ? "-" : string.Join(';', _events); _events.Clear(); return text; }
    }
    /// <summary>원본 uint 문자열의 signed 비트를 보존한다.</summary>
    private static int Whole(string value) => unchecked((int)Number(value));
    /// <summary>CPP stoul이 받는 signed 문자열도 원문DWORD 비트로 보존한다.</summary>
    private static uint Number(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>실제 byte 이름을1:1 string 경계로 옮긴다.</summary>
    private static string Name(string hex) => Encoding.Latin1.GetString(X86Fixture.Bytes(hex));
    /// <summary>스크립트 입력의 원문 전역만 같은 소유자에 쓴다.</summary>
    private static void Global(SoundState state, string name, uint value)
    {
        switch (name)
        {
            case "initialized": state.Initialized = value != 0; break;
            case "device": state.Device = value != 0; break;
            case "enabled": state.Enabled = value != 0; break;
            case "swap": state.SwapSpeakers = value != 0; break;
            case "max": state.MaxPlaying = unchecked((int)value); break;
            case "playing": state.Playing = unchecked((int)value); break;
            case "master": state.MasterVolume = unchecked((int)value); break;
            case "depth": state.VolumeHoldDepth = unchecked((int)value); break;
            case "pending": state.PendingMasterVolume = unchecked((int)value); break;
            default: throw new ArgumentException("알 수 없는 소리 전역 입력", nameof(name));
        }
    }
    /// <summary>동일 명시 입력을 실제 이름 표/재생기에 실행하고 기대 토큰·전체 raw 해시·누적 전역을 비교한다.</summary>
    private static void Script(OriginalEdition edition, string[] row, bool control)
    {
        Assert.Equal(control ? 11 : 8, row.Length); var device = new Device(); var state = new SoundState(); var list = new SoundList(edition, device.Report);
        var player = new SoundPlayer(list, state, device.Hooks(), device.Report); var results = new List<uint>(); var tokens = new List<string>();
        // $n은 기대값이 아닌 이전 실제 반환값만 참조한다.
        uint Handle(string text) => text[0] == '$' ? results[int.Parse(text[1..], CultureInfo.InvariantCulture)] : Number(text);
        // 원본에 공급한 연산을 순서대로 실행하며 재생/판정 결과를 준비하지 않는다.
        foreach (string operation in row[2].Split('|'))
        {
            string[] op = operation.Split(':'); uint result = 0; string shown = "-"; bool entry = false;
            switch (op[0])
            {
                case "i": list.Initialize(); result = list.Fallback(); entry = true; break;
                case "n": result = list.Lookup(Name(op[1])); entry = true; break;
                case "g": Global(state, op[1], Number(op[2])); break;
                case "view": state.View = state.View with { Left = Whole(op[1]), Top = Whole(op[2]), Right = Whole(op[3]), Bottom = Whole(op[4]) }; break;
                case "cam": state.View = state.View with { CameraX = Whole(op[1]), CameraY = Whole(op[2]) }; break;
                case "st": device.Status[X86Fixture.Int(op[1])] = Number(op[2]); break;
                case "sq": device.Queues[X86Fixture.Int(op[1])] = new Queue<uint>(op[2].Split(',').Select(Number)); break;
                case "hr": if (op[1] == "copy") device.CopyResult = Whole(op[2]); else if (op[1] == "gain") device.GainResult = Whole(op[2]); else device.BalanceResult = Whole(op[2]); break;
                case "ld": device.LoadKind = op[1][0]; device.LoadAttenuation = Whole(op[2]); break;
                case "clr": list.SetField(Handle(op[1]), SoundField.Buffer, 0); break;
                case "play": result = player.Play(Handle(op[1]), Number(op[2]), Whole(op[3]), Whole(op[4]), Number(op[5]), Whole(op[6])); entry = true; break;
                case "loop": result = player.PlayLoopAt(X86Fixture.FloatBits(op[1]), X86Fixture.FloatBits(op[2]), Handle(op[3]), Handle(op[4])); entry = true; break;
                case "once": result = player.PlayOnceAt(X86Fixture.FloatBits(op[1]), X86Fixture.FloatBits(op[2]), Handle(op[3]), Number(op[4]), Number(op[5])); entry = true; break;
                case "stop": player.Stop(Handle(op[1])); break;
                case "isp": result = player.IsPlaying(Handle(op[1])); shown = result.ToString(CultureInfo.InvariantCulture); break;
                case "name": result = player.PlayByName(Name(op[1]), Number(op[2]), Whole(op[3]), Whole(op[4]), Number(op[5]), Whole(op[6])); entry = true; break;
                case "at": result = player.PlayNameAt(X86Fixture.FloatBits(op[1]), X86Fixture.FloatBits(op[2]), Name(op[3]), Number(op[4])); entry = true; break;
                case "recount": result = unchecked((uint)player.Recount()); shown = unchecked((int)result).ToString(CultureInfo.InvariantCulture); break;
                case "stopname": player.StopByName(Name(op[1])); break;
                case "isname": result = player.IsPlayingByName(Name(op[1])); shown = result.ToString(CultureInfo.InvariantCulture); break;
                case "switch": result = unchecked((uint)player.SetNamePlaying(Whole(op[1]), op[2] == "-" ? null : Name(op[2]), Number(op[3]), Whole(op[4]))); shown = unchecked((int)result).ToString(CultureInfo.InvariantCulture); break;
                case "stoploops": player.StopLoops(); break;
                case "master": player.SetMasterVolume(Whole(op[1])); break;
                default: throw new ArgumentException("알 수 없는 원본 연산", nameof(operation));
            }
            if (entry) shown = result != 0 ? unchecked((int)(result - SoundConstants.ListBase)).ToString(CultureInfo.InvariantCulture) : "null";
            results.Add(result); tokens.Add($"{device.Take()}={shown}");
        }
        Assert.Equal(row[3], string.Join('|', tokens)); Assert.Equal(Number(row[4]), list.FirstFree() - SoundConstants.ListBase);
        Assert.Equal(Number(row[5]), X86Fixture.Adler32(list.Raw().Span)); Assert.Equal(row[6], state.Playing.ToString(CultureInfo.InvariantCulture)); Assert.Equal(Number(row[7]), state.Serial);
        if (control) { Assert.Equal(row[8], state.MasterVolume.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[9], state.VolumeHoldDepth.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[10], state.PendingMasterVolume.ToString(CultureInfo.InvariantCulture)); }
    }
    /// <summary>원본 재생/월드 투영/위치 계산을 판본별4824관측씩 재생한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void OriginalPlay4824PerEditionMatchesWholeTableAndGeometry(string name, OriginalEdition edition)
    {
        string[][] all = X86Fixture.Read("soundplay"); Assert.Equal(14472, all.Length); string[][] rows = all.Where(row => row[0] == name).ToArray(); Assert.Equal(4824, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[1] == "Script") { Script(edition, row, false); return true; }
            if (row[1] == "Screen")
            {
                Assert.Equal(8, row.Length); SoundPoint point = SoundGeometry.ToScreen(new SoundView(Whole(row[4]), Whole(row[5]), 0, 0, 640, 480), X86Fixture.FloatBits(row[2]), X86Fixture.FloatBits(row[3]));
                Assert.Equal(row[6], point.X.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[7], point.Y.ToString(CultureInfo.InvariantCulture)); return true;
            }
            Assert.Equal("Point", row[1]); Assert.Equal(12, row.Length); var view = new SoundView(0, 0, Whole(row[4]), Whole(row[5]), Whole(row[6]), Whole(row[7])); var pixel = new SoundPoint(Whole(row[2]), Whole(row[3]));
            Assert.Equal(row[9] == "1", SoundGeometry.OnScreen(view, pixel)); Assert.Equal(row[10], SoundGeometry.Pan(view, pixel, row[8] == "1").ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[11], SoundGeometry.Volume(view, pixel).ToString(CultureInfo.InvariantCulture)); return true;
        });
    }
    /// <summary>원본 제어/재집계/보류 음량의 판본별1063관측을현재상태조회순서까지대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void OriginalControl1063PerEditionMatchesCurrentStateQueries(string name, OriginalEdition edition)
    {
        string[][] all = X86Fixture.Read("soundcontrol"); Assert.Equal(3189, all.Length); string[][] rows = all.Where(row => row[0] == name).ToArray(); Assert.Equal(1063, rows.Length);
        X86Fixture.CheckRows(rows, row => { Script(edition, row, true); return true; });
    }
}
