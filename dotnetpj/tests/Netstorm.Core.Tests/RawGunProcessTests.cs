using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 두 PE의 raw/packet/효과와 현재 owner의 소비 순서를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 gunprocess2996행과 checking 통지904행을 독립 기대값으로 비교한다.</summary>
public sealed class RawGunProcessTests
{
    /// <summary>입력의 unsigned 십진수를 읽는다.</summary>
    private static uint N(string value) => uint.Parse(value, System.Globalization.CultureInfo.InvariantCulture);
    /// <summary>정렬되지 않은 현재 DWORD를 읽는다.</summary>
    private static uint Get(ReadOnlySpan<byte> bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    /// <summary>주변 padding을 보존하고 실제 DWORD만 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>원본 fixture의 실제 데이터 행만 읽는다.</summary>
    private static IEnumerable<string[]> Rows(string file) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", file)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));

    /// <summary>native 하네스와 같은 물리 슬롯/경계 인자만 준비하고 출력은 계산하지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>판본별 실제 Pool이다.</summary>
        internal RawSidPool Pool { get; }
        /// <summary>각 부모 group을 주입할 실제 TYPE 입력이다.</summary>
        internal OriginalTypeRecord[] Types { get; }
        /// <summary>현재 게임 상태 입력이다.</summary>
        internal GunProcessState State { get; } = new();
        /// <summary>원문0x38과 모든 padding을 포함한다.</summary>
        internal GunProcessFields Fields { get; } = new();
        /// <summary>관측의 기존64BYTE 버퍼다.</summary>
        internal byte[] Packet { get; } = new byte[64];
        /// <summary>실제 호출 인자/순서의 기록이다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>현재 입력 판본의 필드 폭 선택이다.</summary>
        internal bool Patch { get; }
        /// <summary>탐색 경계의 입력 반환 SID다.</summary>
        internal ushort Found;
        /// <summary>현재 애니메이션 반환 SID다.</summary>
        internal ushort Animation;
        /// <summary>실제 Kill 경계의 존재 관문이다.</summary>
        internal bool Exists;
        /// <summary>발사 경계의 입력 결과다.</summary>
        internal bool ShotResult;
        /// <summary>난수 반환 float의 입력 비트다.</summary>
        internal uint Random;
        /// <summary>동일한 판본의 초기 Client SID5~127을 확보한다.</summary>
        internal Scene(bool patch, bool server = false)
        {
            Patch = patch; Pool = new(24000, server, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            Types = Enumerable.Range(0, patch ? 188 : 171).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, i) }).ToArray();
            // 독립 관측에서 사용하는 실제 client 번호를 같은 순서로 확보한다.
            for (int sid = 5; sid < 128; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>입력 필드와 이전 버퍼 바이트를 다시 준비한다. 기대 출력 열은 읽지 않는다.</summary>
        internal void Prepare(string[] row)
        {
            Events.Clear(); State.SharedAuthority = null; State.AllianceRead = null;
            // 실제 new의 padding 패턴도 독립 입력과 같다.
            for (int i = 0; i < Fields.Bytes.Length; i++) Fields.Bytes[i] = unchecked((byte)(i * 13 + 7));
            // 미작성 packet tail도 원문 관측의 이전 바이트로 둔다.
            for (int i = 0; i < Packet.Length; i++) Packet[i] = unchecked((byte)(i * 7 + 3));
            Put(Fields.Bytes, 0, Patch ? 0x50b668u : 0x501118u); Fields.Bytes[8] = (byte)N(row[4]);
            BinaryPrimitives.WriteUInt64LittleEndian(Fields.Bytes.AsSpan(16), ulong.Parse(row[17]));
            BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(24), (ushort)N(row[3]));
            Put(Fields.Bytes, 28, N(row[2])); Put(Fields.Bytes, 32, N(row[5])); Put(Fields.Bytes, 40, Patch ? 0x50b648u : 0x5010f8u); Put(Fields.Bytes, 44, 70); Put(Fields.Bytes, 48, 50);
            Span<byte> parent = Pool.RawSlot(50); parent.Clear(); parent[10] = (byte)N(row[6]); parent[11] = (byte)N(row[8]); parent[Patch ? 34 : 32] = 1; parent[Patch ? 40 : 35] = (byte)N(row[7]);
            BinaryPrimitives.WriteUInt16LittleEndian(parent[6..], 70); Put(parent, 14, 0x42480000); Put(parent, 18, 0x42500000);
            Span<byte> target = Pool.RawSlot(60); target.Clear(); target[11] = (byte)N(row[9]); target[Patch ? 34 : 32] = 2;
            Types[parent[10]].Group = (int)N(row[5]); State.Now = BitConverter.UInt64BitsToDouble(ulong.Parse(row[18])); State.Boss = N(row[10]) != 0; State.Server = N(row[11]) != 0;
            State.FortMode = N(row[12]) != 0; State.FixedMode = N(row[13]) != 0; State.Disabled = N(row[14]) != 0; State.FriendlyFire = new(N(row[15]) != 0); State.Checking = new(false);
            State.Allies[1, 2] = N(row[16]) != 0; Found = (ushort)N(row[19]); Random = N(row[20]); Animation = (ushort)N(row[21]); Exists = N(row[22]) != 0; ShotResult = N(row[23]) != 0;
        }
        /// <summary>원문 외부 경계의 현재 인자/순서만 기록한다.</summary>
        internal GunProcessHooks Hooks() => new()
        {
            RemoveDependency = (form, target) => Events.Add($"D:{form}:{target}"), AddDependency = (target, _) => Events.Add($"A:{target}"), RepopForm = _ => Events.Add("P"),
            AnimationForm = parent => { Events.Add($"L:{parent}"); return Animation; }, KillAnimation = (form, flags) => { if (Exists) Events.Add($"K:{form}:{flags}"); }, RefreshRange = sid => Events.Add($"R:{sid}"),
            TargetingMode = (sid, owner) => { Events.Add($"M:{sid}:{owner}"); return 2; },
            FindTarget = (x, y, type, mode, owner) => { Events.Add($"F:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{type}:{mode}:{owner}"); return Found; },
            RandomBetween = (low, high) => { Events.Add($"N:{BitConverter.SingleToUInt32Bits(low)}:{BitConverter.SingleToUInt32Bits(high)}"); return BitConverter.UInt32BitsToSingle(Random); },
            StartAnimation = (stage, form, target) => Events.Add($"S:{stage}:{form}:{target}"), ParentBlocked = sid => (Pool.Slot(sid)[Patch ? 40 : 35] & 0x20) != 0,
            ShotContext = sid =>
            {
                ReadOnlySpan<byte> raw = Pool.Slot(sid); Events.Add($"M:{sid}:1"); Events.Add($"C:{sid}");
                return [Get(raw, 14), Get(raw, 18), 0x42500000, 0x42540000, raw[10], 2, raw[Patch ? 34 : 32], Patch ? Get(raw, 36) : raw[34]];
            },
            Shoot = (context, target) => { Events.Add("H:" + string.Join(',', context) + ',' + target); return ShotResult; }, FiringInterval = _ => 2.25,
            Attach = (fields, parent) => { Events.Add($"T:34:{parent}:0:0"); Put(fields.Bytes, 44, 70); Put(fields.Bytes, 48, parent); },
        };
        /// <summary>입력이 지정한 실제 본체 진입점만 실행한다.</summary>
        internal uint Run(string[] row, RawGunProcess logic)
        {
            switch (N(row[1]))
            {
                case 0: logic.Construct(Fields, 50); break;
                case 1: logic.Frame(Fields); break;
                case 2: logic.ClearTarget(Fields, N(row[24]) != 0); break;
                case 3: logic.SetTarget(Fields, 61); break;
                case 4: RawGunProcess.Attached(Fields); break;
                case 5: logic.Notify(Fields, N(row[25]), 0x12345678, N(row[26])); break;
                case 6: logic.Fire(Fields); break;
                case 7: return RawGunProcess.WriteData(Fields, Packet, 70, N(row[29]), N(row[27]) != 0);
                case 8: return RawGunProcess.WriteProcess(Fields, Packet, N(row[28]) != 0, N(row[27]) != 0);
                case 9: logic.ReceiveUpdate(Fields, (byte)N(row[4]), N(row[29])); break;
                default: throw new InvalidOperationException("Gun fixture 진입점 오류");
            }
            return 0;
        }
        /// <summary>전체 raw/packet과 사건을 PE의 네 출력 열과 비교한다.</summary>
        internal void Compare(string[] expected, uint length)
        {
            Assert.Equal(expected[0], string.Join(';', Events)); Assert.Equal(Convert.FromHexString(expected[1]), Fields.Bytes);
            Assert.Equal(Convert.FromHexString(expected[2]), Packet); Assert.Equal(expected[3], length.ToString(System.Globalization.CultureInfo.InvariantCulture));
        }
    }

    /// <summary>두 판본의1498행을 fallback/실제 공유 권한 양쪽에서 전수 대조한다.</summary>
    [Theory]
    [InlineData(true, false)] [InlineData(false, false)] [InlineData(true, true)] [InlineData(false, true)]
    public void WholeNativeImagesMatchBothAuthorityPaths(bool patch, bool shared)
    {
        var scene = new Scene(patch); int count = 0;
        // 기존 PE의 전체 정상 입력을 그대로 실행한다.
        foreach (string[] row in Rows("gunprocess-x86.tsv"))
        {
            if (row[0] != (patch ? "originals" : "originalCD")) continue; Assert.Equal(34, row.Length); scene.Prepare(row);
            if (shared) { scene.State.SharedAuthority = new() { Boss = scene.State.Boss, Server = scene.State.Server }; scene.State.Boss = !scene.State.Boss; scene.State.Server = !scene.State.Server; }
            var logic = new RawGunProcess(scene.Pool, scene.Types, scene.State, scene.Hooks()); uint length = scene.Run(row, logic); scene.Compare(row[30..], length); count++;
        }
        Assert.Equal(1498, count);
    }

    /// <summary>현재 방향별81DWORD의 한 방향을 읽고 반대 방향/오래된 bool 사본은 쓰지 않는다.</summary>
    [Theory]
    [InlineData(true)] [InlineData(false)]
    public void CurrentDirectionalAllianceReplaysNativeFrame(bool patch)
    {
        var scene = new Scene(patch); int count = 0, reads = 0;
        // boss/표적/죽음의 짧은 평가까지 기존 Frame 관측으로 확인한다.
        foreach (string[] row in Rows("gunprocess-x86.tsv"))
        {
            if (row[0] != (patch ? "originals" : "originalCD") || row[1] != "1") continue;
            scene.Prepare(row); scene.State.Allies[1, 2] = N(row[16]) == 0;
            scene.State.AllianceRead = (owner, target) => { Assert.Equal(1u, owner); Assert.Equal(2u, target); reads++; return N(row[16]) != 0 ? 7u : 0u; };
            var logic = new RawGunProcess(scene.Pool, scene.Types, scene.State, scene.Hooks()); scene.Compare(row[30..], scene.Run(row, logic)); count++;
        }
        Assert.Equal(261, count); Assert.True(reads > 0);
    }

    /// <summary>checking 외부 owner와 진단 복귀 뒤 현재 단계/표적/부모 재조회904관측이다.</summary>
    [Fact]
    public void NotifyMatchesAllIndependentCheckingDiagnostics()
    {
        var scene = new Scene(true); int count = 0;
        // 통지 전용 PE 관측은 실제 공격/애니메이션 생성을 실행하지 않는다.
        foreach (string[] row in Rows("gun-notify-checking-x86.tsv"))
        {
            string[] input = row[0].Split(','), control = row[1].Split(','); string[] legacy = ["originals", .. input]; scene.Prepare(legacy);
            uint checking = N(control[0]); scene.State.Checking = new RawCheckingFlag(() => checking); GunProcessHooks hooks = scene.Hooks(); Func<ushort, ushort> animation = hooks.AnimationForm!;
            hooks.AnimationForm = parent => { if (N(control[1]) == 1) { checking = 0; Put(scene.Fields.Bytes, 28, 9); } return animation(parent); };
            hooks.Assertion = (expression, file, line) =>
            {
                scene.Events.Add($"X:{Convert.ToHexString(System.Text.Encoding.ASCII.GetBytes(expression)).ToLowerInvariant()}:{Convert.ToHexString(System.Text.Encoding.ASCII.GetBytes(file)).ToLowerInvariant()}:{line}");
                if (N(control[2]) == 0) throw new ArithmeticException("명시 진단 중단");
                uint mutation = N(control[1]);
                if (mutation is 2 or 3) { checking = 0; BinaryPrimitives.WriteUInt16LittleEndian(scene.Fields.Bytes.AsSpan(24), (ushort)N(legacy[26])); Put(scene.Fields.Bytes, 28, 2); scene.Pool.RawSlot(50)[11] = mutation == 2 ? (byte)2 : (byte)0; }
            };
            var logic = new RawGunProcess(scene.Pool, scene.Types, scene.State, hooks); string outcome = "return";
            try { logic.Notify(scene.Fields, N(legacy[25]), 0x12345678, N(legacy[26])); } catch (ArithmeticException) { outcome = "assertion"; }
            scene.Compare(row[2..6], 0); Assert.Equal(row[6], outcome); Assert.Equal(N(row[7]), checking); Assert.Equal(N(row[8]), scene.Pool.Slot(50)[11]); count++;
        }
        Assert.Equal(904, count);
    }

    /// <summary>선택되지 않은 효과는 요구하지 않고 부족한 writer는 어떤 바이트도 변경하지 않는다.</summary>
    [Fact]
    public void NaturalClearAndShortWriterPreserveSelectedBoundary()
    {
        var scene = new Scene(true); scene.Prepare(Rows("gunprocess-x86.tsv").First()); scene.State.SharedAuthority = new() { Boss = false, Server = false };
        var logic = new RawGunProcess(scene.Pool, scene.Types, scene.State, new()); byte[] before = scene.Fields.Bytes.ToArray();
        logic.ClearTarget(scene.Fields, true); Assert.Equal(before, scene.Fields.Bytes);
        Assert.Throws<InvalidOperationException>(() => logic.Construct(scene.Fields, 50)); Assert.Equal(before, scene.Fields.Bytes);
        // force의 최대31바이트보다 짧은 모든 폭은 dirty/padding/packet을 보존한다.
        for (int width = 0; width < 31; width++)
        {
            byte[] packet = Enumerable.Repeat((byte)0xa5, width).ToArray();
            Assert.Throws<ArgumentException>(() => RawGunProcess.WriteData(scene.Fields, packet, 70, 50, true)); Assert.Equal(before, scene.Fields.Bytes); Assert.All(packet, value => Assert.Equal(0xa5, value));
        }
    }

    /// <summary>역할 DWORD는 생성 시 사본이 아니라 각 회전 조건의 현재 owner에서 읽는다.</summary>
    [Fact]
    public void RotationUsesCurrentRoleBoxesWithoutSynchronizingFallbacks()
    {
        var scene = new Scene(true); scene.Prepare(Rows("gunprocess-x86.tsv").First()); scene.Types[128].Group = 0; scene.Pool.RawSlot(50)[10] = 128;
        Put(scene.Fields.Bytes, 32, 0); Put(scene.Fields.Bytes, 28, 2); BinaryPrimitives.WriteUInt16LittleEndian(scene.Fields.Bytes.AsSpan(24), 60); scene.State.Boss = false;
        var wind = new UIntBox { Value = 140 }; var thunder = new UIntBox { Value = 131 }; var rain = new UIntBox { Value = 129 };
        scene.State.SharedWindArcherType = wind; scene.State.SharedThunderCannonType = thunder; scene.State.SharedRainCannonType = rain;
        var logic = new RawGunProcess(scene.Pool, scene.Types, scene.State, scene.Hooks()); logic.Frame(scene.Fields); Assert.Equal(4u, scene.Fields.Stage());
        wind.Value = 128; Put(scene.Fields.Bytes, 28, 2); logic.Frame(scene.Fields); Assert.Equal(3u, scene.Fields.Stage()); Assert.Contains("S:2:70:60", scene.Events);
        thunder.Value = 128; Put(scene.Fields.Bytes, 28, 2); logic.Frame(scene.Fields); Assert.Equal(4u, scene.Fields.Stage());
        thunder.Value = 131; rain.Value = 128; Put(scene.Fields.Bytes, 28, 2); logic.Frame(scene.Fields); Assert.Equal(4u, scene.Fields.Stage()); Assert.Equal(140u, scene.State.WindArcherType);
    }

    /// <summary>flags0 부착/최초31BYTE/갱신13BYTE/실제 Kernel 실행과40삭제의 전체 수명을 확인한다.</summary>
    [Fact]
    public void ActualTypedHostOwnsGunAndClosesKernelFormAndDepths()
    {
        var scene = new Scene(true, true); scene.Prepare(Rows("gunprocess-x86.tsv").First()); scene.State.Now = 10; scene.State.Boss = false;
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(50)[6..], 0);
        var kernel = new Kernel(); var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, new RawSquidHash(), new byte[65536]), scene.Types);
        byte[] buffer = new byte[64]; var packets = new List<byte[]>(); var changed = new List<bool>();
        var transmit = new ProcessFormTransmission
        {
            WriteData = (process, parentChanged) => { changed.Add(parentChanged); Assert.True((scene.Pool.Slot(process.Form())[11] & 4) != 0); return Assert.IsType<GunProcess>(process).WritePacket(buffer, parentChanged, false); },
            Transmit = length => packets.Add(buffer.AsSpan(0, length).ToArray()),
        };
        var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, new() { Now = 10 }, transmit: transmit);
        var logic = new RawGunProcess(scene.Pool, scene.Types, scene.State, new());
        try
        {
            GunProcess gun = Assert.IsType<GunProcess>(RawGunProcess.CreateGunProcess(host, logic, 50)); int form = gun.Form();
            Assert.Same(gun, host.FromForm(form)); Assert.Equal(1, kernel.Size()); Assert.Equal(31, packets.Single().Length); Assert.True(changed.Single()); Assert.Equal(0, gun.Fields.Dirty());
            Assert.Equal(form, BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(50)[6..])); Assert.Equal(8, scene.Pool.Slot(form)[11] & 12);
            kernel.RunFrame(); Assert.Equal(1u, gun.Fields.Stage()); Put(gun.Fields.Bytes, 28, 3); gun.Fields.Bytes[8] = 8; host.RefreshForm(gun);
            Assert.Equal(3u, gun.Fields.Stage()); Assert.Equal(13, packets[1].Length); Assert.False(changed[1]);
            host.Kill(gun, 0x40); Assert.Equal(0, kernel.Size()); Assert.Null(host.FromForm(form)); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(50)[6..]));
            Assert.True((scene.Pool.Slot(form)[11] & 3) != 0); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }

    /// <summary>dirty는 observer 전에 잡고 packet 본문은 observer 뒤 현재 바이트로 읽으며 부분 실패도 보존한다.</summary>
    [Fact]
    public void ReceivePreservesObserverOrderRawBitsAndPartialFailure()
    {
        var scene = new Scene(true); scene.Prepare(Rows("gunprocess-x86.tsv").First()); Put(scene.Fields.Bytes, 28, 3); scene.Fields.Bytes[8] = 0x80;
        byte[] packet = new byte[31]; packet[8] = 0x1f; int calls = 0;
        var logic = new RawGunProcess(scene.Pool, scene.Types, scene.State, new()
        {
            AnimationForm = _ => { calls++; return 77; }, KillAnimation = (sid, flags) =>
            {
                Assert.Equal(77, sid); Assert.Equal(0x40u, flags); packet[8] = 0;
                Put(packet, 9, 0x12345678); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(13), 0x7ff8123456789abc); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(21), 61); Put(packet, 23, 6); Put(packet, 27, 1);
            },
        });
        logic.ReadData(scene.Fields, packet, out uint parent); Assert.Equal(2, calls); Assert.Equal(0x12345678u, parent); Assert.Equal(0x80, scene.Fields.Dirty());
        Assert.Equal(0x7ff8123456789abcul, BinaryPrimitives.ReadUInt64LittleEndian(scene.Fields.Bytes.AsSpan(16))); Assert.Equal(61, scene.Fields.Target()); Assert.Equal(6u, scene.Fields.Stage()); Assert.Equal(1, scene.Fields.Group());
        Put(scene.Fields.Bytes, 28, 0); packet[8] = 7; Put(packet, 9, 0x87654321); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(13), 0x8000000000000000);
        Assert.Throws<ArgumentException>(() => logic.ReadData(scene.Fields, packet.AsMemory(0, 21), out _)); Assert.Equal(0x8000000000000000ul, BinaryPrimitives.ReadUInt64LittleEndian(scene.Fields.Bytes.AsSpan(16))); Assert.Equal(61, scene.Fields.Target());
        byte[] before = scene.Fields.Bytes.ToArray(); logic.InitializeReceived(scene.Fields); Assert.Equal(before.AsSpan(32, 8).ToArray(), scene.Fields.Bytes.AsSpan(32, 8).ToArray()); Assert.Equal(before[26..28], scene.Fields.Bytes[26..28]); Assert.Equal(0xff, scene.Fields.Dirty()); Assert.Equal(0, scene.Fields.Form());
    }
}
