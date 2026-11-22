using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 원문공통Fire의5534관측을그대로재생하고효과뒤현재값을대조한다.
namespace Netstorm.Core.Tests;

/// <summary>썬/썬더의실제Frame/생성/가상Step/Launch와Host수명을검사한다.</summary>
public sealed class RawCannonFireTests
{
    /// <summary>미설치Missile은코너효과전에실패하며미도래Frame은가상Step을요구하지않는다.</summary>
    [Fact]
    public void MissingEffectsFailAtSelectedSourceBoundary()
    {
        var scene=new Scene();string[] row=File.ReadLines(Path.Combine(AppContext.BaseDirectory,"X86/cannon-fire-x86.tsv")).First(line=>line.Length!=0&&!line.StartsWith('#')).Split('\t');scene.Prepare(row);scene.Types[158].Flags1=0x10;scene.Pool.RawSlot(60)[11]=0;int points=0;var logic=new RawCannonFire(scene.Pool,scene.Types,scene.Frames,scene.State,new(){TargetPoint=(_,_)=>{points++;return[0,0];}});Assert.Throws<InvalidOperationException>(()=>logic.Launch(scene.Fields));Assert.Equal(0,points);
        scene.Pool.RawSlot(60)[11]=2;BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Bytes.AsSpan(48),100);byte[] before=scene.Fields.Bytes.ToArray();logic.FrameWithStep(scene.Fields,null);Assert.Equal(before,scene.Fields.Bytes);BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Bytes.AsSpan(48),10);Assert.Throws<InvalidOperationException>(()=>logic.FrameWithStep(scene.Fields,null));Assert.Equal(0,points);
    }
    /// <summary>실제PATCH Host의현재Gun통지·같은CannonFire삭제와Kernel/깊이를닫는다.</summary>
    [Fact]
    public void ActualHostNotifiesAndDeletesCurrentCannonFire()
    {
        var scene = new Scene(true); scene.Types[131].Group = 1; scene.Frames[131].Frames = new([new RiftFrameCode((byte)'A',0,0,0)]); scene.Pool.RawSlot(50).Clear(); scene.Pool.RawSlot(50)[10] = 131; scene.Pool.RawSlot(60)[10] = 158; scene.Pool.RawSlot(60)[11] = 2;
        var kernel = new Kernel(); var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, new RawSquidHash(), new byte[65536]), scene.Types); byte[] buffer = new byte[64]; int packets = 0; var host = new SquidProcessHost(scene.Pool,scene.Types,kernel,destroy,new(){Now=10},transmit:new(){WriteData=(process,changed)=>Assert.IsType<GunProcess>(process).WritePacket(buffer,changed,false),Transmit=_=>packets++});
        try
        {
            var gunLogic = new RawGunProcess(scene.Pool,scene.Types,new(){Now=10,Boss=false},new()); GunProcess gun = Assert.IsType<GunProcess>(RawGunProcess.CreateGunProcess(host,gunLogic,50)); Put(gun.Fields.Bytes,28,6); var state = new CannonFireState{Now=10}; var logic = new RawCannonFire(scene.Pool,scene.Types,scene.Frames,state,RawCannonFire.MakeHostHooks(host,new(){Unpop=(_,_)=>{},Repop=(_,_)=>{}})); CannonFire fire = Assert.IsType<CannonFire>(RawCannonFire.Create(host,logic,(ushort)gun.Form(),60)); int form=fire.Form(); Assert.Equal(2,kernel.Size()); fire.RunFrame(); Assert.Equal(7u,gun.Fields.Stage()); Assert.Null(host.FromForm(form)); Assert.Equal(1,kernel.Size()); Assert.True((scene.Pool.Slot(form)[11]&3)!=0); Assert.True(packets>0); Assert.Equal(0u,destroy.PreDepth); Assert.Equal(0u,destroy.PostDepth); host.Kill(gun,0x40); Assert.Equal(0,kernel.Size());
        }
        finally{kernel.DrainOwnedForShutdown();}
    }
    /// <summary>독립입력의십진DWORD다.</summary>
    private static uint N(string text) => uint.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>부호있는원문Step/효과음차이입력이다.</summary>
    private static int I(string text) => int.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>현재DWORD폭만기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>실제전체raw와동일한소문자표시다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>기대열을읽지않고현재입력/효과경계를준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>원문5~70번호를확보한같은풀이다.</summary>
        internal RawSidPool Pool { get; }
        /// <summary>현재HP/원문생성자입력표다.</summary>
        internal OriginalTypeRecord[] Types { get; }
        /// <summary>현재문자코드구간입력이다.</summary>
        internal PriestPlainCanonType[] Frames { get; }
        /// <summary>효과뒤바뀌는원문전역이다.</summary>
        internal CannonFireState State = new();
        /// <summary>패딩을포함한실제64BYTE다.</summary>
        internal GunAnimationFields Fields { get; } = new();
        /// <summary>현재독립입력0~10열이다.</summary>
        internal string[] Row = [];
        /// <summary>효과의실제순서/인수기록이다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>원문명시슬롯을같은할당자로확보한다.</summary>
        internal Scene(bool server = false)
        {
            Pool = new(24000, server); Types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray(); Frames = Enumerable.Range(0, 188).Select(_ => new PriestPlainCanonType()).ToArray();
            // 관측이사용한네슬롯과GunForm을실제원문번호로확보한다.
            for (int sid = 5; sid <= 70; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>원문입력만으로상태/프레임/패딩/다섯슬롯을재구성한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(15, row.Length); Row = row; Events.Clear(); State = new() { Now = 10, GrowlOffset = I(row[8]), RoarOffset = I(row[9]) }; bool sun = N(row[0]) == 0; if (sun) { State.Kind = CannonFireKind.Sun; State.Type = 128; State.DisplayType = 80; State.EffectType = 11; State.Delay = .07f; } Types[158].Flags1 = N(row[5]) != 0 ? 0x10u : 0;
            var codes = new List<RiftFrameCode>();
            // 연속/단일/교차/불균등문자코드는native입력과같은명시자료다.
            for (int i = 0; i < 32; i++) { uint style = N(row[6]); int letter = style == 0 ? 65 + i / 8 : style == 1 ? 65 : style == 2 ? 65 + i % 2 : i == 0 ? 65 : i < 6 ? 66 : i < 19 ? 67 : 68; codes.Add(new((byte)letter,0,0,0)); }
            Frames[State.Type].Frames = new(codes);
            // 생성자가쓰지않는패딩은관측의동일패턴을보존한다.
            for (int i = 0; i < Fields.Bytes.Length; i++) Fields.Bytes[i] = unchecked((byte)(i * 13 + 7));
            Put(Fields.Bytes, 0, sun ? 0x50b318u : 0x50b370u); Put(Fields.Bytes, 4, 81); Put(Fields.Bytes, 8, 50); Put(Fields.Bytes, 12, 0x42700000); Put(Fields.Bytes, 16, 0x42700000); Put(Fields.Bytes, 20, 0x426e0000); Put(Fields.Bytes, 24, 0x426e0000); Put(Fields.Bytes, 32, sun ? 0x50b310u : 0x50b368u); Fields.Bytes[40] = 0x20; BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(42), 70); BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44), 60); BinaryPrimitives.WriteDoubleLittleEndian(Fields.Bytes.AsSpan(48), N(row[3]) != 0 ? 10 : 10.25); Put(Fields.Bytes, 56, 0);
            // free/dead를포함한관측raw슬롯을각입력마다다시쓴다.
            foreach ((int sid, byte type) in new[] { (50,(byte)State.Type),(51,(byte)State.Type),(60,(byte)158),(61,(byte)158),(70,(byte)34) })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); Put(raw, 0, 0x1131a000); raw[10] = type; raw[11] = sid == 60 ? (byte)N(row[4]) : (byte)0; raw[12] = 0xa5; raw[34] = 1; Put(raw, 14, BitConverter.SingleToUInt32Bits(sid < 60 ? 50.5f : 60)); Put(raw, 18, BitConverter.SingleToUInt32Bits(sid < 60 ? 50.5f : 60)); Put(raw, 36, N(row[2]));
            }
        }
        /// <summary>원문과같은선택효과뒤에서만현재필드/전역을바꾼다.</summary>
        internal void Mutate(string where)
        {
            uint value = N(Row[10]); if (where == "Center" && value == 1) Put(Pool.RawSlot(50), 36, 2); if (where == "Center" && value == 2) BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44), 61);
            if (where == "Unpop" && value == 3) { State.Now = 100; BinaryPrimitives.WriteDoubleLittleEndian(Fields.Bytes.AsSpan(48), 777); } if (where == "Repop" && value == 4) Pool.RawSlot(60)[11] = 2;
            if (where == "Point" && value == 5) { Put(Fields.Bytes, 8, 51); BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44), 61); Put(Fields.Bytes, 12, BitConverter.SingleToUInt32Bits(77.25f)); Put(Fields.Bytes, 16, BitConverter.SingleToUInt32Bits(78.5f)); State.Speed = 37; State.DisplayType = State.Kind == CannonFireKind.Sun ? 82u : 133u; }
            if (where == "Missile" && value == 6) BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(42), 71); if (where == "Sound" && value == 7) { State.RoarOffset = State.GrowlOffset; Put(Fields.Bytes, 8, 51); State.Now = 100; }
        }
        /// <summary>실제Missile호출인자만관측하며생성/비행완료를대신하지않는다.</summary>
        internal void Projectile(GunProjectileRequest request)
        {
            Events.Add("A:128"); uint[] args = [request.Firing,request.Target,BitConverter.SingleToUInt32Bits(request.Speed),request.DisplayType,request.EffectType,BitConverter.SingleToUInt32Bits(request.TargetPosition[0]),BitConverter.SingleToUInt32Bits(request.TargetPosition[1]),BitConverter.SingleToUInt32Bits(request.TargetCenter[0]),BitConverter.SingleToUInt32Bits(request.TargetCenter[1]),request.Ricochet,BitConverter.SingleToUInt32Bits(request.FiringPosition[0]),BitConverter.SingleToUInt32Bits(request.FiringPosition[1])]; Events.Add("J:" + string.Join(',', args.Select(v => v.ToString(CultureInfo.InvariantCulture)))); Mutate("Missile");
        }
        /// <summary>같은현재슬롯/시각변경과명시월드효과를관측한다.</summary>
        internal GunAnimationHooks Hooks() => new()
        {
            Center = sid => { Events.Add($"C:{sid}"); Mutate("Center"); return [60,50]; }, Unpop = (sid, flags) => { Events.Add($"U:{sid}:{flags}"); Mutate("Unpop"); }, Repop = (sid, flags) => { Events.Add($"P:{sid}:{flags}"); Mutate("Repop"); }, TargetPoint = (sid, target) => { Events.Add($"T:{sid}:{target}"); Mutate("Point"); return [52.25f,52.75f]; }, CreateProjectile = Projectile, Sound = (_, name) => { Events.Add("S:" + name); Mutate("Sound"); }, Notify = (_, ev, info, value) => Events.Add($"M:{ev}:{info}:{value}"), Kill = (form, flags) => Events.Add($"K:{form}:{flags}"),
        };
    }
    /// <summary>두파생의Frame/생성/가상Step/Launch5534건을실제원문전체이미지와대조한다.</summary>
    [Theory]
    [InlineData(0, 0, 0, 838)] [InlineData(1, 0, 0, 2118)]
    [InlineData(0, 1, 3, 9)] [InlineData(1, 1, 3, 2569)]
    public void MatchesAllIndependentNativeFramesAndEffectReentry(int profile, int first, int last, int expected)
    {
        var scene = new Scene(); int count = 0;
        // 기대열11~14는비교에만쓰고복원값이나입력생성에쓰지않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/cannon-fire-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); int kind = I(row[1]); if (I(row[0]) != profile || kind < first || kind > last) continue; scene.Prepare(row); var logic = new RawCannonFire(scene.Pool, scene.Types, scene.Frames, scene.State, scene.Hooks()); string result = "return";
            switch (kind)
            {
                case 0: logic.Frame(scene.Fields); break;
                case 1: logic.Initialize(scene.Fields, 70, 60); scene.Events.Add("B:35:50:0:16"); Put(scene.Fields.Bytes, 4, 81); Put(scene.Fields.Bytes, 8, 50); break;
                case 2: result = logic.Step(scene.Fields, I(row[7])).ToString(CultureInfo.InvariantCulture); break;
                case 3: result = logic.Launch(scene.Fields) ? "1" : "0"; break;
                default: throw new InvalidOperationException("캐논 독립입력의미지본문");
            }
            Assert.Equal(row[11], result); Assert.Equal(row[12], string.Join(';', scene.Events)); Assert.Equal(row[13], Hex(scene.Fields.Bytes)); string slots = string.Concat(new[] { 50,51,60,61 }.Select(sid => Hex(scene.Pool.Slot(sid)))); Assert.Equal(row[14], slots); count++;
        }
        Assert.Equal(expected, count);
    }
}
