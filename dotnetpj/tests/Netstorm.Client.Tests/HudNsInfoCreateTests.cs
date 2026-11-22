using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 독립 원본 NSInfo 생성의 전체 메모리/문자열/호출 순서를 대조한다.

/// <summary>앞10입력만 재생해170관측의 정상120행·중단50행과 전체 guard를 비교한다.</summary>
public sealed class HudNsInfoCreateTests
{
    /// <summary>명시 원문 장치가 중단한 경계다.</summary>
    private sealed class Stop : Exception { }
    /// <summary>현재 같은 기본 주소가 속하는 파생 종류다.</summary>
    private enum Kind { Base, Ns, Player, Battle, Button }
    /// <summary>원문 DWORD 숫자다.</summary>
    private static uint N(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>원시 숫자를 문화권 없는 같은 인자 문자열로 만든다.</summary>
    private static string U(uint value) => value.ToString(CultureInfo.InvariantCulture);
    /// <summary>현재 비정렬DWORD 입력이다.</summary>
    private static uint Get(byte[] raw, int at) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at));
    /// <summary>현재DWORD만 변경하고 이웃/guard는 보존한다.</summary>
    private static void Put(byte[] raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at), value);
    /// <summary>원문 좁은 바이트를 같은16진 문자열로 기록한다.</summary>
    private static string Hex(string value) => Convert.ToHexStringLower(Encoding.Latin1.GetBytes(value));
    /// <summary>실제 할당 한 개의 원시 메모리와 안정된 형별 주소다.</summary>
    private sealed class Record
    {
        /// <summary>원문 new 요청 크기다.</summary>
        public uint Size;
        /// <summary>현재 실제 주소의 파생 종류다.</summary>
        public Kind Kind;
        /// <summary>미작성 영역을 포함한 전체0x180바이트다.</summary>
        public required byte[] Raw;
        /// <summary>일반 하위 장치의 실제 기본 주소다.</summary>
        public readonly GumpCreateNode Plain = new();
        /// <summary>동일 주소의 NSInfo 본체다.</summary>
        public readonly HudNsInfoNode Ns = new();
        /// <summary>동일 주소의 PlayerInfo 본체다.</summary>
        public readonly HudPlayerInfoNode Player = new();
        /// <summary>동일 주소의 BattleInfo 본체다.</summary>
        public readonly HudBattleInfoNode Battle = new();
        /// <summary>동일 주소의 특수 버튼 본체다.</summary>
        public readonly HudPlayerButtonNode Button = new();
        /// <summary>하위 스크롤 장치의 명시 필드다.</summary>
        public uint TextFlags;
        /// <summary>하위 일반 버튼 장치의 명시 필드다.</summary>
        public uint ButtonFlags;
        /// <summary>원본처럼 현재 파생의 같은 상속 주소만 반환한다.</summary>
        public GumpCreateNode Base => Kind switch { Kind.Ns => Ns, Kind.Player => Player, Kind.Battle => Battle, Kind.Button => Button, _ => Plain };
        /// <summary>명시 raw 입력을 현재 필드에만 반영한다.</summary>
        public void Import()
        {
            Base.Vtable = Get(Raw, 0); Base.Flags = Get(Raw, 4); Base.Command = Get(Raw, 0x20); TextFlags = Get(Raw, 0xd0); ButtonFlags = Get(Raw, 0xac);
            if (Kind == Kind.Ns)
            {
                Ns.Background = Get(Raw, 0xa0);
                // 원본의 고정8자식 주소 순서다.
                for (int i = 0; i < 8; i++) Ns.Players[i] = Get(Raw, 0xa4 + i * 4);
                Ns.Roster = Get(Raw, 0xc4); Ns.Title = Get(Raw, 0xc8); Ns.UpperDivider = Get(Raw, 0xcc); Ns.LowerDivider = Get(Raw, 0xd0); Ns.Battle = Get(Raw, 0xd4);
            }
            if (Kind == Kind.Player) { Player.Text = Get(Raw, 0xa0); Player.Master = Get(Raw, 0xa4); Player.Button = Get(Raw, 0xa8); Player.Start = Get(Raw, 0xac); Player.Player = Get(Raw, 0xb0); Player.ButtonType = Get(Raw, 0xb4); Player.Reserved = Get(Raw, 0xb8); }
            if (Kind == Kind.Battle)
            {
                Battle.Title = Get(Raw, 0xa0); Battle.Summary = Get(Raw, 0xa4); Battle.Body = Get(Raw, 0xa8); Battle.Battle = Get(Raw, 0xac); Battle.Changed = Get(Raw, 0xb0);
                // 실제22쌍과 뒤의 미작성3BYTE도 함께 읽는다.
                for (int i = 0; i < 22; i++) { Battle.Settings[i, 0] = Get(Raw, 0xb4 + i * 8); Battle.Settings[i, 1] = Get(Raw, 0xb8 + i * 8); }
                Raw.AsSpan(0x164, 4).CopyTo(Battle.Bytes); Battle.Mode = Get(Raw, 0x168);
            }
            if (Kind == Kind.Button) { Button.ButtonFlags = Get(Raw, 0xac); Button.UntouchedC0 = Get(Raw, 0xc0); Button.Entry = Get(Raw, 0xc4); }
        }
        /// <summary>제품이 사용하는 필드만 내보내고 나머지384바이트는 보존한다.</summary>
        public void Export()
        {
            Put(Raw, 0, Base.Vtable); Put(Raw, 4, Base.Flags); Put(Raw, 0x20, Base.Command);
            if (Kind == Kind.Base) { Put(Raw, 0xd0, TextFlags); Put(Raw, 0xac, ButtonFlags); }
            if (Kind == Kind.Ns)
            {
                Put(Raw, 0xa0, Ns.Background);
                // 같은 고정배열의 현재 포인터만 기록한다.
                for (int i = 0; i < 8; i++) Put(Raw, 0xa4 + i * 4, Ns.Players[i]);
                Put(Raw, 0xc4, Ns.Roster); Put(Raw, 0xc8, Ns.Title); Put(Raw, 0xcc, Ns.UpperDivider); Put(Raw, 0xd0, Ns.LowerDivider); Put(Raw, 0xd4, Ns.Battle);
            }
            if (Kind == Kind.Player) { Put(Raw, 0xa0, Player.Text); Put(Raw, 0xa4, Player.Master); Put(Raw, 0xa8, Player.Button); Put(Raw, 0xac, Player.Start); Put(Raw, 0xb0, Player.Player); Put(Raw, 0xb4, Player.ButtonType); Put(Raw, 0xb8, Player.Reserved); }
            if (Kind == Kind.Battle)
            {
                Put(Raw, 0xa0, Battle.Title); Put(Raw, 0xa4, Battle.Summary); Put(Raw, 0xa8, Battle.Body); Put(Raw, 0xac, Battle.Battle); Put(Raw, 0xb0, Battle.Changed);
                // guard와 미작성BYTE를 변경 없이 포함한다.
                for (int i = 0; i < 22; i++) { Put(Raw, 0xb4 + i * 8, Battle.Settings[i, 0]); Put(Raw, 0xb8 + i * 8, Battle.Settings[i, 1]); }
                Battle.Bytes.CopyTo(Raw, 0x164); Put(Raw, 0x168, Battle.Mode);
            }
            if (Kind == Kind.Button) { Put(Raw, 0xac, Button.ButtonFlags); Put(Raw, 0xc0, Button.UntouchedC0); Put(Raw, 0xc4, Button.Entry); }
        }
    }
    /// <summary>명시 입력의 원시 노드/문자열/변경과 외부 장치만 보유한다.</summary>
    private sealed class Scene
    {
        /// <summary>원문13개 실제 전역 주소 순서다.</summary>
        private static readonly uint[] Addresses = [0x540421,0x540440,0x540457,0x5ca920,0x5ca924,0x5467b8,0x5411c8,0x5bec3c,0x54db4c,0x540bac,0x594fc8,0x595344,0x593150];
        /// <summary>현재 원문DWORD 입력이다.</summary>
        private readonly uint[] _globals;
        /// <summary>같은 현재 전역12의 직접 owner다.</summary>
        public readonly UIntBox Current;
        /// <summary>실제 할당 순서와 같은 주소 순서의 레코드다.</summary>
        public readonly SortedDictionary<uint, Record> Records = [];
        /// <summary>실제 번역 순서의512BYTE 문자열 장치다.</summary>
        private readonly SortedDictionary<uint, byte[]> _strings = [];
        /// <summary>원문 고정 서버 버퍼256BYTE다.</summary>
        private readonly byte[] _server;
        /// <summary>명시 형식 문자열과 원격 서버 입력이다.</summary>
        private readonly string _formatInput, _serverInput;
        /// <summary>전체 입력 메모리의 초기BYTE다.</summary>
        private readonly byte _fill;
        /// <summary>실제 TYPE 조회 장치의 명시 응답이다.</summary>
        private readonly uint _typeResult;
        /// <summary>명시 원문 장치별 실제 호출 수다.</summary>
        private readonly Dictionary<string, int> _calls = [];
        /// <summary>지정 경계에서 적용하는 실제 주소 변경이다.</summary>
        private readonly string[][] _effects;
        /// <summary>명시 중단 경계다.</summary>
        private readonly HashSet<string> _stops;
        /// <summary>new 실패를 반환할 실제 할당 호출번호다.</summary>
        private readonly HashSet<int> _failures;
        /// <summary>실제 new 호출 수다.</summary>
        private int _allocations;
        /// <summary>외부 형식화의 현재 결과다.</summary>
        private string _formatted = "";
        /// <summary>원문 인자 그대로의 외부 호출 순서다.</summary>
        public string Trace = "";
        /// <summary>같은 현재 owner와 장치를 사용하는 실제 제품 본체다.</summary>
        public readonly HudNsInfoCreate Body;
        /// <summary>입력10열만 설치하고 뒤8관측은 읽지 않는다.</summary>
        public Scene(string[] row)
        {
            _globals = row[2].Split(':').Select(N).ToArray(); Assert.Equal(13, _globals.Length); Current = new() { Value = _globals[12] }; _fill = unchecked((byte)N(row[3])); _typeResult = N(row[4]);
            _formatInput = Encoding.Latin1.GetString(Convert.FromHexString(row[5])); _serverInput = Encoding.Latin1.GetString(Convert.FromHexString(row[6])); _server = new byte[256]; Array.Fill(_server, _fill);
            _effects = row[7] == "-" ? [] : row[7].Split(';').Select(value => value.Split('/')).ToArray(); _stops = row[8] == "-" ? [] : row[8].Split(';').ToHashSet(); _failures = row[9] == "-" ? [] : row[9].Split(':').Select(value => (int)N(value)).ToHashSet();
            if (row[1] != "hidden") Add(row[1] switch { "ns" => 0xd8U, "player" => 0xbcU, "battle" => 0x16cU, _ => 0xc8U });
            Body = new(Current, Hooks());
        }
        /// <summary>같은 현재 주소의 파생 종류만 바꾸고 모든 현재 필드를 유지한다.</summary>
        private Record Select(uint token, Kind kind) { var record = Records[token]; if (record.Kind != kind) { record.Export(); record.Kind = kind; record.Import(); } return record; }
        /// <summary>각 new의 실제 초기 원시 블록이다.</summary>
        private uint Add(uint size)
        {
            uint token = 0x1100000 + (uint)Records.Count * 0x200; byte[] raw = new byte[0x180]; Array.Fill(raw, _fill); var record = new Record { Raw = raw, Size = size }; record.Import(); Records.Add(token, record); return token;
        }
        /// <summary>현재 전역 owner를 실제 주소로 읽는다.</summary>
        private uint Read(uint address) { int index = Array.IndexOf(Addresses, address); if (index < 0) throw new InvalidOperationException("NSInfo 검사 전역 주소"); return index == 12 ? Current.Value : _globals[index]; }
        /// <summary>명시 변경을 같은 원시 노드/문자열/전역에만 쓴다.</summary>
        private void Effect(uint address, uint value)
        {
            if (address >= 0x1100000 && address < 0x1110000) { uint token = 0x1100000 + (address - 0x1100000) / 0x200 * 0x200; Put(Records[token].Raw, (int)(address - token), value); return; }
            if (address >= 0x1200000 && address < 0x1210000) { uint token = 0x1200000 + (address - 0x1200000) / 0x200 * 0x200; Put(_strings[token], (int)(address - token), value); return; }
            int index = Array.IndexOf(Addresses, address); if (index < 0) throw new InvalidOperationException("NSInfo 검사 변경 주소"); value = index < 3 ? value & 255 : value; if (index == 12) Current.Value = value; else _globals[index] = value;
        }
        /// <summary>현재 제품 쓰기→관측 인자→명시 변경→같은 객체 재반영→중단 순서다.</summary>
        private void Event(string name, params string[] values)
        {
            // 경계 이전의 같은 모든 현재 필드를 원시 메모리에 반영한다.
            foreach (var record in Records.Values) record.Export();
            if (Trace.Length != 0) Trace += ';'; Trace += name + string.Concat(values.Select(value => ":" + value)); string trigger = name + (_calls[name] = _calls.GetValueOrDefault(name) + 1).ToString(CultureInfo.InvariantCulture);
            // 같은 경계의 여러 변경은 원문 입력 순서를 지킨다.
            foreach (var effect in _effects) if (effect[0] == trigger) Effect(N(effect[1]), N(effect[2]));
            // 현재 주소는 교체하지 않고 실제 입력으로 필드만 바꾼다.
            foreach (var record in Records.Values) record.Import();
            if (_stops.Contains(trigger)) throw new Stop();
        }
        /// <summary>원문 하위 장치가 명시한 기본3필드만 변경한다.</summary>
        private void Device(uint token, uint flags = 1)
        {
            // 현재 제품 필드를 먼저 반영해 다른 확장을 보존한다.
            foreach (var record in Records.Values) record.Export();
            var raw = Records[token].Raw; Put(raw, 0, 0x1000000); Put(raw, 4, flags); Put(raw, 0x20, 0);
            // 같은 모든 실제 주소에 현재값만 다시 쓴다.
            foreach (var record in Records.Values) record.Import();
        }
        /// <summary>원문 정적/현재 번역/공유 서버 버퍼/형식 결과의 C문자열이다.</summary>
        private string Text(uint token)
        {
            string? fixedText = token switch { 0x50c894 => "~w~E%s\nPlayers in Zone %d\n", 0x50cd3c => "~r~EBATTLEMASTER", 0x50cd2c => "Start Battle", 0x50d068 => "LAN Server", 0x1f00000 => _formatted, _ => null }; if (fixedText is not null) return fixedText;
            byte[] bytes = token == 0x594970 ? _server : _strings[token]; int end = Array.IndexOf(bytes, (byte)0); if (end < 0) throw new InvalidOperationException("NSInfo 검사 C문자열 NUL 없음"); return Encoding.Latin1.GetString(bytes, 0, end);
        }
        /// <summary>명시 원문 번역 장치가 반환하는 새512BYTE를 만든다.</summary>
        private uint Translate(uint source)
        {
            string key = Text(source), value = key == "~w~E%s\nPlayers in Zone %d\n" ? _formatInput : "T:" + key; uint token = 0x1200000 + (uint)_strings.Count * 0x200; byte[] bytes = new byte[512]; Array.Fill(bytes, _fill); var input = Encoding.Latin1.GetBytes(value); input.CopyTo(bytes, 0); bytes[input.Length] = 0; _strings.Add(token, bytes); Event("translate", Hex(key), U(token)); return token;
        }
        /// <summary>실제 외부 가상 부착의 전달 인자다.</summary>
        private void Attach(string name, uint token, float x, float y, uint parent, uint mode) => Event(name, U(token), U(BitConverter.SingleToUInt32Bits(x)), U(BitConverter.SingleToUInt32Bits(y)), U(parent), U(mode));
        /// <summary>원본 callback 장치만 설치하며 제품 생성/정렬 계산을 대신하지 않는다.</summary>
        private HudNsInfoCreateHooks Hooks() => new()
        {
            Node = token => Select(token, Kind.Ns).Ns, Player = token => Select(token, Kind.Player).Player, Battle = token => Select(token, Kind.Battle).Battle, Button = token => Select(token, Kind.Button).Button,
            Base = token => Records[token].Base, AddTextFlags = (token, flags) => Records[token].TextFlags |= flags,
            AddButtonFlags = (token, flags) => { var r = Records[token]; if (r.Kind == Kind.Button) r.Button.ButtonFlags |= flags; else r.ButtonFlags |= flags; }, Read = Read,
            Allocate = size => { uint token = _failures.Contains(++_allocations) ? 0 : Add((uint)size); Event("alloc", U((uint)size), U(token)); return token; },
            BaseConstruct = (token, command, width, height, flags) => { Device(token, flags); Event("base", U(token), U(command), U(BitConverter.SingleToUInt32Bits(width)), U(BitConverter.SingleToUInt32Bits(height)), U(flags)); },
            Background = (token, width, height, flags, padding, type, frame) => { Device(token); Event("background", U(token), U(BitConverter.SingleToUInt32Bits(width)), U(BitConverter.SingleToUInt32Bits(height)), U(flags), U(padding), U(type), U(frame)); return token; },
            Scroll = (token, a, b, c, d) => { Device(token); Records[token].TextFlags = 0x100; Event("scroll", U(token), U(unchecked((uint)a)), U(unchecked((uint)b)), U(c), U(d)); return token; },
            Text = (token, a, b, c, d) => { Device(token); Event("text", U(token), U(unchecked((uint)a)), U(unchecked((uint)b)), U(c), U(d)); return token; },
            ButtonBase = (token, a, b, flags) => { Device(token); Records[token].Button.ButtonFlags = 0x100; Event("buttonBase", U(token), U(unchecked((uint)a)), U(unchecked((uint)b)), U(flags)); },
            Stone = (token, a, b, label, color, context) => { Device(token); Records[token].ButtonFlags = 0x100; Event("stone", U(token), U(unchecked((uint)a)), U(unchecked((uint)b)), U(label), U(color), U(context)); return token; },
            Scrollable = (token, value) => { var r = Records[token]; r.TextFlags = (r.TextFlags & ~4U) | ((value & 1) * 4); Event("scrollable", U(token), U(value)); },
            Slider = (token, value) => { var r = Records[token]; r.TextFlags = (r.TextFlags & ~2U) | ((value & 1) * 2); Event("slider", U(token), U(value)); }, Wrap = (token, value) => Event("wrap", U(token), U(value)),
            ParseScroll = (token, text) => Event("parseScroll", U(token), Hex(Text(text))), ParseText = (token, text) => Event("parseText", U(token), Hex(Text(text))),
            FitText = (token, a, b) => Event("fitText", U(token), U(a), U(b)), FitStone = token => Event("fitStone", U(token)),
            Attach = (token, x, y, parent, mode) => { Kind kind = Records[token].Kind; if (kind == Kind.Player) Body.AttachPlayer(token, x, y, parent, mode); else if (kind == Kind.Battle) Body.AttachBattle(token, x, y, parent, mode); else Attach(kind == Kind.Button ? "baseAttach" : "attach", token, x, y, parent, mode); },
            BaseAttach = (token, x, y, parent, mode) => Attach("baseAttach", token, x, y, parent, mode), Translate = Translate, Type = type => { Event("type", U(type), U(_typeResult)); return _typeResult; },
            ServerName = () => { bool lan = Read(0x54db4c) == uint.MaxValue; string text = lan ? Text(Translate(0x50d068)) : _serverInput; var input = Encoding.Latin1.GetBytes(text); input.CopyTo(_server, 0); _server[input.Length] = 0; if (!lan) Event("remote", U(Read(0x54db4c)), "0", "0", U(0x594970), "80", "0"); return 0x594970; },
            FormatZone = (format, server, zone) => { string fmt = Text(format), name = Text(server); _formatted = fmt.Replace("%s", name, StringComparison.Ordinal).Replace("%d", zone.ToString(CultureInfo.InvariantCulture), StringComparison.Ordinal); Event("format", Hex(fmt), Hex(name), U(unchecked((uint)zone))); return 0x1f00000; },
            Assertion = line => Event("assert", U(line)), MetamapLayout = (kind, token) => Event("meta", kind == HudNsInfoChild.Player ? "player" : "battle", U(token)),
        };
        /// <summary>전체 현재384BYTE 블록을 원문 할당 순서로 기록한다.</summary>
        public string Nodes()
        {
            // 관측 직전 현재 제품 필드만 반영한다.
            foreach (var record in Records.Values) record.Export();
            return Records.Count == 0 ? "-" : string.Join(';', Records.Select(pair => U(pair.Key) + ':' + U(pair.Value.Size) + ':' + Convert.ToHexStringLower(pair.Value.Raw)));
        }
        /// <summary>번역512BYTE·서버256BYTE·형식 결과 전체를 순서대로 기록한다.</summary>
        public string Strings() => string.Join(';', _strings.Select(pair => U(pair.Key) + ':' + Convert.ToHexStringLower(pair.Value))) + '|' + Convert.ToHexStringLower(_server) + '|' + Hex(_formatted);
        /// <summary>같은 현재 owner를 포함한 원문13DWORD 순서다.</summary>
        public string Globals() => string.Join(':', Addresses.Select(address => U(Read(address))));
    }
    /// <summary>모든 정상/부분 관측에서 입력과 관측을 분리해 실제 제품 본체를 실행한다.</summary>
    [Fact]
    public void Native170ParentsChildrenFullMemoryStringsAndPartialEffects()
    {
        int rows = 0, partial = 0, full = 0;
        // 원본 두 정밀도의 모든 입력을 전수 재생한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "hud-nsinfo-create-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue; var row = line.Split('\t'); Assert.Equal(18, row.Length); var scene = new Scene(row); bool completed = true; uint result = 0;
            try { result = row[1] switch { "hidden" => scene.Body.CreateHidden(), "ns" => scene.Body.Construct(0x1100000), "player" => scene.Body.ConstructPlayer(0x1100000), "battle" => scene.Body.ConstructBattle(0x1100000), _ => scene.Body.ConstructButton(0x1100000, 16, 15) }; } catch (Stop) { completed = false; }
            if (!completed) partial++; if (completed && (row[1] is "hidden" or "ns") && result != 0) { full++; Assert.Equal(50, scene.Records.Count); }
            Assert.Equal(row[10] == "1", completed); if (completed) Assert.Equal(N(row[11]), result); Assert.Equal(row[12], scene.Globals()); Assert.Equal(row[13], scene.Nodes()); Assert.Equal(row[14], scene.Strings()); Assert.Equal(row[15], scene.Trace.Length == 0 ? "-" : scene.Trace); Assert.True(N(row[17]) > 0);
            if (completed) Assert.Equal(row[1] == "button" ? "12" : "4", row[16]); rows++;
        }
        Assert.Equal(170, rows); Assert.Equal(50, partial); Assert.True(full > 0);
    }
    /// <summary>기본 부착 뒤에만 현재 두 관문을 읽고 선택한 실제 메타 후속 누락은 거부한다.</summary>
    [Fact]
    public void RequiresSelectedMetamapContinuationAfterActualBaseAttachment()
    {
        uint meta = 0, known = 0; int attached = 0; var body = new HudNsInfoCreate(new(), new() { Read = address => address == 0x594fc8 ? meta : known, BaseAttach = (token, x, y, parent, mode) => { Assert.Equal(7U, token); Assert.Equal(1, x); Assert.Equal(2, y); Assert.Equal(8U, parent); Assert.Equal(9U, mode); attached++; } });
        body.AttachPlayer(7, 1, 2, 8, 9); meta = 1; body.AttachBattle(7, 1, 2, 8, 9); known = 1; Assert.Throws<InvalidOperationException>(() => body.AttachPlayer(7, 1, 2, 8, 9)); Assert.Equal(3, attached);
    }
}
