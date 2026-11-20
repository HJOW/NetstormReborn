using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원본 x86에서 동결한 플레어의 슬롯·난수·호출 순서와 반환 비트를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>같은 원문139행·주문 방향60행을 typed와 Native 계약에 각각 적용한다.</summary>
public sealed class X86RawFlareTests
{
    /// <summary>관측 실행기에 공급했던 독립 입력과 실제 효과 인자 기록이다.</summary>
    private sealed class Scene
    {
        /// <summary>부모와 생성 후보가 공유하는 PATCH 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>원문과 같은 현재 genus 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>각 방향7프레임의 명시 코드 입력이다.</summary>
        public readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames(
            Enumerable.Range(0, 112).Select(i => new RiftFrameCode((byte)('A' + i / 7), (byte)'A', (byte)i, 0)))).ToArray();
        /// <summary>원본 난수 상태다.</summary>
        public readonly GameRandom Random = new();
        /// <summary>실제 호출한 하위 인자를 기록한다.</summary>
        public readonly FlareHooks Hooks = new();
        /// <summary>현재 생성 억제 시각이다.</summary>
        public double Suppressed;
        /// <summary>현재 로딩 깊이다.</summary>
        public uint Loading;
        /// <summary>호출 순서의 전체 기록이다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>명시 Kernel 프로세스 수다.</summary>
        private readonly int _kernel;
        /// <summary>실제 SHP 추가 헤더의 signed 기준점이다.</summary>
        private readonly (short X, short Y) _hotspot;
        /// <summary>독립 예약 성공 비트 입력이다.</summary>
        private readonly uint _allocations;
        /// <summary>이미 선택한 예약의 개수다.</summary>
        private int _reserved;
        /// <summary>생성한 후보 개수다.</summary>
        private int _created;

        /// <summary>같은 물리 슬롯과 보존 패턴을 준비하고 효과 기록 장치를 연결한다.</summary>
        public Scene(uint[] input, bool spawn)
        {
            // 원본 실행기의 client SID 순서와 동일하게 실제 슬롯을 할당한다.
            for (int sid = 5; sid < 110; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            // 부모와 생성 후보11개의 전체50바이트를 동일한 패턴으로 채운다.
            foreach (int sid in new[] { 50, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109 })
            {
                Span<byte> raw = Pool.AllocatedBytes(sid);
                raw.Fill(0xa5); Put(sid, 0, 0x5096a0); raw[10] = 106; raw[11] = 0;
                raw[34] = 8; raw[35] = 99; raw[40] = 32; Put(sid, 36, 77);
            }
            Span<byte> parent = Pool.AllocatedBytes(50);
            if (spawn)
            {
                Suppressed = BitConverter.UInt64BitsToDouble(input[2] | ((ulong)input[3] << 32));
                Loading = input[4]; _kernel = unchecked((int)input[5]); Types[85].Flags2 = input[6];
                parent[10] = 85; Put(50, 14, input[7]); Put(50, 18, input[8]); parent[35] = (byte)input[9];
                Put(50, 36, 0); _hotspot = (unchecked((short)input[10]), unchecked((short)input[11]));
                Random.SetState(input[12]); _allocations = input[13];
            }
            else
            {
                _kernel = unchecked((int)input[3]); Put(50, 14, input[4]); Put(50, 18, input[5]);
                Put(50, 36, input[0] * 7); Random.SetState(input[6]); _allocations = input[7];
            }
            Hooks.ProcessCount = () => { Trace.Add("K," + _kernel.ToString(CultureInfo.InvariantCulture)); return _kernel; };
            Hooks.Create = (type, flags) => { int sid = 99 + ++_created; Add("C", type, flags, (uint)sid); return sid; };
            Hooks.SourceHotspot = sid => { Assert.Equal(50, sid); Add("H"); return _hotspot; };
            Hooks.Owner = (sid, owner) => { Add("O", (uint)sid, owner); Pool.AllocatedBytes(sid)[34] = (byte)owner; };
            Hooks.Pop = (sid, x, y, flags) =>
            {
                Add("P", (uint)sid, Bits(x), Bits(y), flags); Put(sid, 14, Bits(x)); Put(sid, 18, Bits(y));
            };
            Hooks.ReserveRegular = () => { uint success = (_allocations >> _reserved++) & 1u; Add("R", success); return success != 0; };
            Hooks.AddRegular = (sid, ev, payload) => Add("A", (uint)sid, ev, Bits(payload));
            Hooks.Frame = (sid, frame, flags) => { Add("F", (uint)sid, unchecked((uint)frame), flags); Put(sid, 36, unchecked((uint)frame)); };
            Hooks.Update = sid => Add("U", (uint)sid);
            Hooks.Destroy = (sid, flags) => Add("D", (uint)sid, flags);
            Hooks.Sound = (x, y, sound, flags) => { Assert.Equal("powerup.wav", sound); Add("S", Bits(x), Bits(y), flags); };
        }

        /// <summary>현재 입력 getter와 원문 계약을 같은 본체에 연결한다.</summary>
        public RawFlare Flare(bool native) => new(Pool, Types, Frames, Random,
            new FlareState { Native = native, SuppressedTime = () => Suppressed, LoadingDepth = () => Loading, Type = () => 106 }, Hooks);
        /// <summary>전체 관측 슬롯을 원문 순서대로 직렬화한다.</summary>
        public string Raw()
        {
            var bytes = new List<byte>();
            // 방문되지 않은 후보까지 모두 읽어 부분 쓰기 보존을 확인한다.
            foreach (int sid in new[] { 50, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109 }) bytes.AddRange(Pool.Slot(sid).ToArray());
            return Convert.ToHexString(bytes.ToArray()).ToLowerInvariant();
        }
        /// <summary>입력 패턴의 free 비트와 무관하게 관측 실행기처럼 물리 DWORD만 쓴다.</summary>
        public void Put(int sid, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(sid).Slice(offset, 4), value);
        /// <summary>효과 인자를 부호 없는 DWORD 십진수로 기록한다.</summary>
        private void Add(string label, params uint[] values) => Trace.Add(label + string.Concat(values.Select(value => "," + value.ToString(CultureInfo.InvariantCulture))));
    }

    /// <summary>생성의 double 억제 입력만 두 DWORD로 나누며 나머지는 하위32비트를 보존한다.</summary>
    private static uint[] Inputs(string text, bool spawn)
    {
        var result = new List<uint>();
        string[] words = text.Split(',');
        // TSV의 음수와 unsigned64를 같은 물리 비트로 읽는다.
        for (int index = 0; index < words.Length; index++)
        {
            ulong value = words[index].StartsWith('-') ? unchecked((ulong)X86Fixture.Long(words[index])) : X86Fixture.ULong(words[index]);
            result.Add(unchecked((uint)value));
            if (spawn && index == 2) result.Add((uint)(value >> 32));
        }
        return result.ToArray();
    }
    /// <summary>단정밀도 반환·좌표·예약 비트를 손실 없이 비교한다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);

    /// <summary>원문139행의 반환·난수·550바이트·모든 효과 인자를 양쪽 계약에서 대조한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void Original139(bool native)
    {
        string[][] rows = X86Fixture.Read("flare");
        Assert.Equal(30, rows.Count(row => row[0] == "Spawn"));
        Assert.Equal(109, rows.Count(row => row[0] == "Regular"));
        X86Fixture.CheckRows(rows, row =>
        {
            bool spawn = row[0] == "Spawn";
            uint[] input = Inputs(row[1], spawn);
            var scene = new Scene(input, spawn); RawFlare flare = scene.Flare(native);
            uint result = 0;
            if (spawn) flare.Spawn(50, unchecked((int)input[0]), input[1]);
            else result = Bits(flare.Handle(50, input[1], input[2], BitConverter.UInt32BitsToSingle(input[8])));
            Assert.Equal(X86Fixture.UInt(row[2]), result); Assert.Equal(X86Fixture.UInt(row[3]), scene.Random.State());
            Assert.Equal(row[4], scene.Raw()); Assert.Equal(row[5], scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace));
            return true;
        });
    }

    /// <summary>C/D 방향60행도 원본의 전체 슬롯·난수·호출 기록과 대조한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void OriginalDirections60(bool native)
    {
        string[][] rows = X86Fixture.Read("bomb-status-flare"); Assert.Equal(60, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] input = Inputs(row[1], true); var scene = new Scene(input, true);
            scene.Flare(native).SpawnDirection(50, unchecked((int)input[0]), input[1], 106, X86Fixture.UInt(row[0]));
            Assert.Equal(0u, X86Fixture.UInt(row[2])); Assert.Equal(X86Fixture.UInt(row[3]), scene.Random.State());
            Assert.Equal(row[4], scene.Raw()); Assert.Equal(row[5], scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace));
            return true;
        });
    }

    /// <summary>Native의 후속 단계는 원문에 없는 가상표·억제 시각·난수 접근을 요구하지 않는다.</summary>
    [Fact]
    public void LaterNativeCountReadsOnlySelectedInputs()
    {
        var scene = new Scene(Inputs("0,1,1,0,0,0,1,0,1045220557", false), false);
        scene.Put(50, 0, 0x12345678);
        var flare = new RawFlare(scene.Pool, scene.Types, scene.Frames, (Func<GameRandom>?)null, new FlareState { Native = true }, scene.Hooks);
        Assert.Equal(1045220557u, Bits(flare.Handle(50, 1, 1, BitConverter.UInt32BitsToSingle(1045220557))));
        Assert.Equal("F,50,1,0", Assert.Single(scene.Trace));
    }
}
