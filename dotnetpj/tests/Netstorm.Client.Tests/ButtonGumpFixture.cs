using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 버튼의 전체 상태/바이트/콜백을 보존하는 검사 저장소다.

/// <summary>cpppj/tests/ButtonGumpTests.cpp의 독립 PE 입력/관측 하네스다. 기대 전달/기한/배경 상태는 계산하지 않는다.</summary>
internal sealed class ButtonGumpFixture
{
    /// <summary>기계어 관측의 노드 바이트 크기.</summary>
    private const int RawSize = 0x170;
    /// <summary>기계어 관측의6개 고정 토큰.</summary>
    public static readonly uint[] Tokens = [0x19000010, 0x190001a0, 0x19000330, 0x190004c0, 0x19000650, 0x190007e0];
    /// <summary>signed/unsigned 입력을 같은 DWORD로 읽는다.</summary>
    public static uint Word(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>쉼표 DWORD 입력 목록.</summary>
    private static uint[] Words(string text) => text.Split(',').Select(Word).ToArray();
    /// <summary>raw DWORD의 비정렬 읽기.</summary>
    private static uint Get(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset));
    /// <summary>raw DWORD의 비정렬 쓰기.</summary>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset), value);

    /// <summary>원본 미사용 바이트와 실제 버튼 노드/배경+b4를 함께 소유한다.</summary>
    internal sealed class Record
    {
        /// <summary>모든 처리기가 공유하는 안정된 실제 버튼 객체.</summary>
        public ButtonGumpNode Node { get; } = new();
        /// <summary>원본의 미사용 바이트.</summary>
        private byte[] _original = new byte[RawSize];
        /// <summary>배경에 해당하면 raw+b4는 버튼 기한 대신 이 필드다.</summary>
        public uint BackgroundFlags;
        /// <summary>원본6개 중 마지막2개는 배경 저장소다.</summary>
        public bool Background { get; set; }
        /// <summary>같은 객체에 원본 필드만 대입한다. 호스트 가상 분류Kind는 보존한다.</summary>
        public void Read(byte[] raw)
        {
            Assert.Equal(RawSize, raw.Length); _original = raw; GumpCreateNode decoded = GumpRawSupport.Decode(raw);
            // C++ 기본 구조체 대입처럼 기반/수집 속성만 같은 객체에 복사한다.
            foreach (System.Reflection.PropertyInfo p in typeof(GumpCreateNode).GetProperties()) if (p.CanWrite) p.SetValue(Node, p.GetValue(decoded));
            decoded.Positioning.CopyTo(Node.Positioning, 0); decoded.TextContext.CopyTo(Node.TextContext, 0);
            Node.Value = Get(raw, 160); Node.Pressed = Get(raw, 164); Node.Blink = Get(raw, 168); Node.ButtonFlags = Get(raw, 172);
            Node.FirstDeadline = BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(raw.AsSpan(176))); Node.RepeatDeadline = BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(raw.AsSpan(184))); Node.Background = Get(raw, 256); BackgroundFlags = Get(raw, 180);
        }
        /// <summary>같은 필드만 원본 위치에 쓰고 모든 미사용 바이트를 보존한다.</summary>
        public byte[] Raw()
        {
            byte[] raw = (byte[])_original.Clone(); X86Fixture.Bytes(GumpRawSupport.Encode(Node)).CopyTo(raw, 0); Put(raw, 160, Node.Value); Put(raw, 164, Node.Pressed); Put(raw, 168, Node.Blink); Put(raw, 172, Node.ButtonFlags);
            BinaryPrimitives.WriteUInt64LittleEndian(raw.AsSpan(176), BitConverter.DoubleToUInt64Bits(Node.FirstDeadline)); BinaryPrimitives.WriteUInt64LittleEndian(raw.AsSpan(184), BitConverter.DoubleToUInt64Bits(Node.RepeatDeadline)); Put(raw, 256, Node.Background); if (Background) Put(raw, 180, BackgroundFlags); return raw;
        }
    }
    /// <summary>실제 기본 등록/판본.</summary>
    public GumpCreateState Create { get; } = new();
    /// <summary>실제idle/캡처/삭제 표.</summary>
    public GumpLifetimeState LifeState { get; } = new();
    /// <summary>현재 선택 전역.</summary>
    private readonly GumpEventState _event = new();
    /// <summary>현재overlay 사건 전역.</summary>
    private readonly GumpInputState _input = new();
    /// <summary>원본 초기화의 사건ID와 signed 색.</summary>
    public ButtonGumpState State { get; } = new() { ValueEvent = 0x12345, ClickEvent = 0x12346, RightEvent = 0x12347, RepeatEvent = 0x12348, BaseColor = 56 };
    /// <summary>토큰 순서의 안정된 저장소.</summary>
    public SortedDictionary<uint, Record> Records { get; } = [];
    /// <summary>종류/토큰별 명시 콜백 응답 쓰기.</summary>
    public Dictionary<string, uint[][]> Responses { get; } = [];
    /// <summary>명시 월드 투영 응답.</summary>
    public Dictionary<uint, ScreenPoint> Anchors { get; } = [];
    /// <summary>각 조회의 독립 시계 double 비트.</summary>
    public List<ulong> Times { get; } = [];
    /// <summary>현재 폴링 사건.</summary>
    public InputEvent Poll { get; set; } = new(0x10000, 999, 888);
    /// <summary>각 부모의 전체 반환DWORD.</summary>
    public uint[] ParentReturns { get; } = [1, 1, 1, 1, 1, 1];
    /// <summary>기본 도움말 경계의 전체 반환DWORD.</summary>
    public uint BaseReturn { get; set; } = 0x81234567;
    /// <summary>다음 벽 시계 입력 위치.</summary>
    private int _timeIndex;
    /// <summary>모든 실제 콜백 당시 인자/전체 전역/노드의 순서.</summary>
    public List<string> Calls { get; } = [];
    /// <summary>실제 기본 생성기.</summary>
    public GumpCreate Factory { get; private set; } = null!;
    /// <summary>실제 삭제/idle 처리기.</summary>
    public GumpLifetime Lifetime { get; private set; } = null!;
    /// <summary>실제 현재 좌표 처리기.</summary>
    public GumpCollection Collection { get; private set; } = null!;
    /// <summary>실제 부모 전파/선택.</summary>
    public GumpEvents Events { get; private set; } = null!;
    /// <summary>실제 캡처/뗌 경로.</summary>
    public GumpInput Input { get; private set; } = null!;
    /// <summary>실제 버튼 상태 처리기.</summary>
    public ButtonGump Button { get; private set; } = null!;
    /// <summary>실제 등록 통지/idle 실행.</summary>
    public GumpFrame Frame { get; private set; } = null!;
    /// <summary>CD의 실제 활성idle 길이.</summary>
    private int IdleSize => Create.Collection.Edition == OriginalEdition.Patch1078 ? 500 : 80;
    /// <summary>안정된 기본/버튼 객체.</summary>
    private GumpCreateNode? Find(uint token) => Records.GetValueOrDefault(token)?.Node;
    /// <summary>원본 압축 표의 전체DWORD를 길이:값으로 기록한다.</summary>
    private static string Table(ReadOnlySpan<uint> values)
    {
        var result = new List<string>();
        // 비활성 뒤쪽도 포함해 모든 같은 값 구간을 기록한다.
        for (int i = 0; i < values.Length;)
        {
            uint value = values[i]; int start = i;
            // 연속한 동일DWORD만 축약한다.
            while (i < values.Length && values[i] == value) i++; result.Add($"{i - start}:{value}");
        }
        return string.Join(';', result);
    }
    /// <summary>압축 입력 표를 정확한 길이에 펼친다.</summary>
    private static void ReadTable(string text, Span<uint> output)
    {
        int cursor = 0;
        // 입력 구간의 순서/길이만 재생한다.
        foreach (string item in text.Split(';')) { string[] p = item.Split(':'); int count = X86Fixture.Int(p[0]); output.Slice(cursor, count).Fill(Word(p[1])); cursor += count; }
        Assert.Equal(output.Length, cursor);
    }
    /// <summary>7스칼라를 실제 공유 필드에 쓴다.</summary>
    private void Scalar(uint index, uint value)
    {
        switch (index) { case 0: Create.Count = value; break; case 1: Create.Serial = value; break; case 2: LifeState.Focus = value; break; case 3: LifeState.KillCount = value; break; case 4: LifeState.DialogCount = value; break; case 5: LifeState.CurrentDialog = value; break; case 6: Create.Collection.OverrideHead = value; break; default: throw new ArgumentException("버튼 스칼라 범위 오류"); }
    }
    /// <summary>7스칼라/실제5표의 현재 전체 상태.</summary>
    public string Globals() => string.Join(',', new[] { Create.Count, Create.Serial, LifeState.Focus, LifeState.KillCount, LifeState.DialogCount, LifeState.CurrentDialog, Create.Collection.OverrideHead }) + '#' + Table(Create.Entries) + '#' + Table(LifeState.Idle.AsSpan(0, IdleSize)) + '#' + Table(LifeState.Overlay) + '#' + Table(LifeState.Kill) + '#' + Table(LifeState.Dialogs);
    /// <summary>주소 순서의 모든0x170 노드 바이트.</summary>
    public string Nodes() => string.Join(';', Records.Select(p => p.Key + "=" + Convert.ToHexStringLower(p.Value.Raw())));
    /// <summary>호출 직전 상태를 먼저 기록하고 입력에 지정한 쓰기만 적용한다.</summary>
    public void Respond(string kind, uint token, params ulong[] args)
    {
        string key = kind + ',' + token; Calls.Add(key + (args.Length == 0 ? "" : "," + string.Join(',', args)) + '@' + Globals() + '@' + Nodes());
        if (!Responses.TryGetValue(key, out uint[][]? patches)) return;
        // 기대 결과 대신 명시된 전역/idle/노드DWORD 쓰기만 수행한다.
        foreach (uint[] p in patches)
        {
            if (p[0] == 0) Scalar(p[1], p[2]); else if (p[0] == 1) LifeState.Idle[p[1]] = p[2]; else { Record r = Records[p[0]]; byte[] raw = r.Raw(); Put(raw, (int)p[1], p[2]); r.Read(raw); }
        }
        Create.Collection.Root = Create.Entries[0];
    }
    /// <summary>각 부모 처리 경계의 실제 반환과 상태 관측.</summary>
    private uint Parent(uint t, uint e, uint a, uint b) { Respond("Parent", t, e, a, b); return ParentReturns[Array.IndexOf(Tokens, t)]; }
    /// <summary>실제 버튼 장치/가상 경계를 공급한다. 참조 반환은 실제 배경 필드만 가리킨다.</summary>
    public ButtonGumpHooks Hooks() => new()
    {
        Node = t => Records.GetValueOrDefault(t)?.Node, BaseNode = Find, BackgroundFlags = (uint t) => ref Records[t].BackgroundFlags,
        Now = () => { ulong value = Times[_timeIndex]; Respond("Now", (uint)_timeIndex++, value); return BitConverter.UInt64BitsToDouble(value); },
        Poll = code => { Respond("Poll", 0, code); return Poll; }, Redraw = (t, flag) => Respond("Redraw", t, flag ? 1UL : 0), Sound = () => Respond("Sound", 0), BaseEvent = (t, e, a, b) => { Respond("Base", t, e, a, b); return BaseReturn; },
    };
    /// <summary>같은 상태/노드의 처리기를 만들고 현재 버튼 가상 분배를 마지막에 연결한다.</summary>
    public void Prepare()
    {
        var e = new GumpEventHooks { Node = Find, Handle = Parent }; var i = new GumpInputHooks { Node = Find, Handle = Parent }; var f = new GumpFrameHooks { Node = Find, Idle = t => Respond("OtherIdle", t), Broadcast = (t, code, a, b) => Respond("OtherBroadcast", t, code, a, b), Redraw = (t, flag) => Respond("Redraw", t, flag ? 1UL : 0) };
        ButtonGump.BindButtonGump(e, i, f, t => t == Tokens[1] ? Button : null); Factory = new(Create, new() { Node = Find }); Lifetime = new(Create, LifeState, new() { Node = Find, Push = t => Respond("Push", t) });
        Collection = new(Create.Collection, Factory.CollectionHooks(sid => { Respond("Project", sid); return Anchors[sid]; })); Events = new(Create, LifeState, _event, Collection, e); Input = new(Create, LifeState, _input, Collection, Events, i); Button = new(Create, State, Factory, Collection, Events, Input, Lifetime, Hooks()); Frame = new(Create, LifeState, f);
    }
    /// <summary>독립 계약용 실제6노드/표/시계 장면을 준비한다.</summary>
    public void Basic()
    {
        Create.Count = 6; Create.Serial = 91;
        // 실제 노드를 만들고 기본 필드만 명시한다.
        for (int index = 0; index < Tokens.Length; index++)
        {
            var r = new Record(); r.Read(new byte[RawSize]); r.Background = index >= 4; Records.Add(Tokens[index], r); ButtonGumpNode n = r.Node;
            n.Vtable = 0x19008000 + (uint)index * 0x100; n.Flags = 1; n.Id = (uint)index; n.Serial = 75 + (uint)index; n.Parent = index == 0 ? 0 : Tokens[0]; n.Width = n.BaseWidth = 75; n.Height = n.BaseHeight = 19; n.Scale = 1; Create.Entries[index] = Tokens[index]; Anchors[(uint)index] = new(17, -19);
        }
        Create.Collection.Root = Tokens[0]; Records[Tokens[0]].Node.Child = Tokens[1]; ButtonGumpNode button = Records[Tokens[1]].Node; button.Flags = 0xa01; button.Background = Tokens[5]; button.Kind = ButtonGumpKind.Stone; button.Child = Tokens[5]; Records[Tokens[5]].Node.Parent = Tokens[1]; Times.AddRange(Enumerable.Repeat(BitConverter.DoubleToUInt64Bits(1), 32)); Prepare();
    }
    /// <summary>완성 원본 입력을 그대로 같은 노드/표/콜백에 연결한다.</summary>
    public void Load(string edition, string[] row)
    {
        Create.Collection.Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        // 모든 노드의 미사용 바이트도 보존한다.
        foreach (string item in row[3].Split(';')) { string[] p = item.Split('='); uint token = Word(p[0]); var r = new Record { Background = token >= Tokens[4] }; r.Read(X86Fixture.Bytes(p[1])); Records.Add(token, r); }
        string[] global = row[4].Split('#'); uint[] scalars = Words(global[0]);
        // 일곱 스칼라는 원래 쓰기 순서대로 적용한다.
        for (int index = 0; index < scalars.Length; index++) Scalar((uint)index, scalars[index]);
        ReadTable(global[1], Create.Entries); ReadTable(global[2], LifeState.Idle.AsSpan(0, IdleSize)); ReadTable(global[3], LifeState.Overlay); ReadTable(global[4], LifeState.Kill); ReadTable(global[5], LifeState.Dialogs); Create.Collection.Root = Create.Entries[0];
        string[] extras = row[5].Split('#'); uint[] kinds = Words(extras[0]);
        // 원본 가상 분류만 호스트 Kind로 옮긴다.
        for (int index = 0; index < Tokens.Length; index++) Records[Tokens[index]].Node.Kind = kinds[index] != 0 ? ButtonGumpKind.Stone : ButtonGumpKind.Base;
        Times.AddRange(extras[1].Split(',').Select(X86Fixture.ULong)); uint[] poll = Words(extras[2]); Poll = new(poll[0], unchecked((int)poll[1]), unchecked((int)poll[2])); Words(extras[3]).CopyTo(ParentReturns, 0); BaseReturn = Word(extras[4]); State.BaseColor = unchecked((sbyte)Word(extras[5]));
        // 실제 좌표 계산 대신 명시 투영점만 공급한다.
        foreach (string item in extras[6].Split(';')) { uint[] p = Words(item); Anchors[p[0]] = new(unchecked((int)p[1]), unchecked((int)p[2])); }
        if (row[6] != "-")
        {
            // 종류/토큰 키마다 입력의 명시 쓰기만 보관한다.
            foreach (string item in row[6].Split(';')) { string[] p = item.Split(':'); Responses[p[0]] = p[1].Split('/').Select(Words).ToArray(); }
        }
        Assert.Equal(row[3], Nodes()); Assert.Equal(row[4], Globals()); Prepare();
    }
}
