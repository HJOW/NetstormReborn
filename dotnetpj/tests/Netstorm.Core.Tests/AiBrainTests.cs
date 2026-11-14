using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using PlayerParticipationState = Netstorm.Core.Simulation.PlayerParticipationState;
using PlayerParticipationHooks = Netstorm.Core.Simulation.PlayerParticipationHooks;

// 실제 AI 생성·목록 소멸을 같은 원본 기계어 관측으로 검사한다.
namespace Netstorm.Tests;

/// <summary>cpppj/tests/AiBrainTests.cpp의160개 원본 입력·수명·현재 그래프 조회 검사다.</summary>
public sealed class AiBrainTests
{
    /// <summary>원본 독립 실행기의 첫 불투명 메모리 핸들이다.</summary>
    private const uint BrainBase = 0x13000000;
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);
    /// <summary>현재 원문 DWORD를 읽는다.</summary>
    private static uint Read(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));
    /// <summary>원본 fixture와 같은 소문자16진 표현이다.</summary>
    private static string Hex(byte[] raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>한 입력의 실제 메모리·Player·동맹·섬 몸체를 함께 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 원본 입력9개다.</summary>
        public readonly long[] Input;
        /// <summary>사제와 모든 부모가 공유하는 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);
        /// <summary>현재188TYPE이며 생성자에는 앞48개를 빌린다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>TYPE+a0의 별도 요구 에너지 문자열이다.</summary>
        private readonly string[] _energy = Enumerable.Repeat("", 188).ToArray();
        /// <summary>실제9개 Player의 동일 바이트다.</summary>
        public readonly byte[] Records = new byte[1620];
        /// <summary>실제81개 방향별 동맹이다.</summary>
        public readonly uint[] Allies = new uint[81];
        /// <summary>실제4개 섬을 담는100기록 원문이다.</summary>
        public readonly byte[] IslandBytes = new byte[3204];
        /// <summary>현재 Player전역이다.</summary>
        private readonly PlayerWorldState _playerState = new();
        /// <summary>현재 BYTE backing을 읽는 무효 그래프다.</summary>
        public byte InvalidGraph = 254;
        /// <summary>섬 소유자 변경의 현재 다시 그리기 전역이다.</summary>
        private uint _changed;
        /// <summary>섬 변경의 현재 지도 리비전이다.</summary>
        private uint _revision;
        /// <summary>할당별 독립 미초기화 패턴 번호다.</summary>
        private int _allocationIndex;
        /// <summary>실제 사제·지역·요구 에너지 조회 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>실제 Player월드 부모다.</summary>
        public readonly RawPlayerWorld Player;
        /// <summary>동맹·brain소유권을 공유하는 참여 부모다.</summary>
        public readonly PlayerParticipation Participation;
        /// <summary>실제 섬의 존재·소유권 부모다.</summary>
        public readonly RawIslandRuntime Islands;
        /// <summary>본체와17개 목록의 실제 소유자다.</summary>
        public readonly RawAiBrainMemory Memory;
        /// <summary>같은 자료에 연결된 실제 생성자·소멸자다.</summary>
        public readonly RawAiBrainConstructor Runtime;
        /// <summary>600바이트의 독립 입력 설정이다.</summary>
        public readonly byte[] Profile = new byte[600];

        /// <summary>기대 출력을 계산하지 않고 cpp검사와 같은 독립 입력 패턴을 구성한다.</summary>
        public Scene(string input)
        {
            Input = input.Split(',').Select(X86Fixture.Long).ToArray();
            Assert.Equal(9, Input.Length);
            _playerState.BattleOwner = (uint)Input[1];
            Put(Records, (int)Input[0] * 180 + 0x78, unchecked((uint)Input[2]));
            // 원본 입력의600바이트와 명시 설정 필드만 준비한다.
            for (int index = 0; index < Profile.Length; index++) Profile[index] = unchecked((byte)(index * 37 + Input[8]));
            Put(Profile, 0x23c, (uint)Input[4]); Put(Profile, 0x240, 0); Put(Profile, 0x244, 0); Put(Profile, 0x248, (uint)Input[3]);
            // 모든 방향을 독립적인7/0으로 채워 대칭 동맹으로 잘못 고치는 경우를 검출한다.
            for (int owner = 0; owner < 9; owner++)
                // 대상 방향마다 별도의 입력을 쓴다.
                for (int target = 0; target < 9; target++) Allies[owner * 9 + target] = owner == target ? 1u : (owner + target) % 3 == 0 ? 7u : 0u;
            var post = new RawPostPop(Pool, Types, new RawPostPopState());
            Player = new RawPlayerWorld(Pool, post, Records, _playerState, new PlayerWorldHooks());
            Participation = new PlayerParticipation(Records, Allies, new byte[9],
                new PlayerParticipationState { LocalOwner = _playerState.BattleOwner },
                new PlayerParticipationHooks
                {
                    DestructBrain = token => Runtime!.Destruct(token),
                    FreeBrain = token => Runtime!.FreeBody(token),
                });
            Put(IslandBytes, 0, 4);
            // 존재하는3섬과 존재하지 않는1섬에 유효 bounds만 준다.
            for (int index = 0; index < 4; index++)
            {
                int start = 4 + index * 32;
                Put(IslandBytes, start + 4, index < 3 ? 1u : 0u); Put(IslandBytes, start + 8, 4);
                Put(IslandBytes, start + 16, BitConverter.SingleToUInt32Bits(1)); Put(IslandBytes, start + 20, BitConverter.SingleToUInt32Bits(1));
                Put(IslandBytes, start + 24, BitConverter.SingleToUInt32Bits(10)); Put(IslandBytes, start + 28, BitConverter.SingleToUInt32Bits(10));
            }
            Islands = new RawIslandRuntime(Pool, new RawSquidHash(), IslandBytes, new FortFinishOwnerState(), new RawPostPopList(),
                () => _changed, value => _changed = value, () => _revision, value => _revision = value,
                new IslandRuntimeHooks
                {
                    SetResidenceOwner = (_, _) => throw new InvalidOperationException("선택하지 않은 거주 구역 효과"),
                    Assertion = _ => throw new InvalidOperationException("선택하지 않은 섬 진단"),
                });
            Islands.BindPlayerRecords(Records);
            if (Input[6] > 0 && Input[6] < 32768)
            {
                // 원본 하네스처럼 할당 상태를 바꾸지 않고 명시 입력SID의 좌표만 쓴다.
                Span<byte> raw = Pool.RawSlot((int)Input[6]);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(5.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(6.25f));
            }
            if (Input[5] == 1)
            {
                Types[3].Flags2 = 0x40800; _energy[3] = "ws";
                Types[4].Flags2 = 0x800; _energy[4] = "Ss";
                Types[5].Flags2 = 0x40000; _energy[5] = "ss";
            }
            else if (Input[5] == 2) { Types[0].Flags2 = 0x40800; _energy[0] = "ss"; }
            Memory = new RawAiBrainMemory(BrainBase, raw =>
            {
                // 해당 할당의 caller입력 바이트다. 생성자의 기대 결과는 사용하지 않는다.
                for (int index = 0; index < raw.Length; index++) raw[index] = unchecked((byte)(index * 29 + Input[8] + _allocationIndex * 17));
                _allocationIndex++;
            });
            Runtime = new RawAiBrainConstructor(Memory, Player, Participation, Islands, Allies, Types.Take(48).ToArray(), Globals(), Hooks());
        }

        /// <summary>각 쓰기 위치에서 BYTE와 현재 Player전역을 다시 조회한다.</summary>
        public AiBrainConstructorGlobals Globals(Func<uint>? graph = null) => new()
        { LocalOwner = () => _playerState.BattleOwner, NoRegion = () => 127, InvalidGraph = graph ?? (() => InvalidGraph) };

        /// <summary>실제 요청 순서만 기록하고 독립 입력의 사제·지역·에너지를 공급한다.</summary>
        public AiBrainConstructorHooks Hooks() => new()
        {
            Priest = owner => { Trace.Add("P," + owner); return (uint)Input[6]; },
            RegionAt = (x, y) => { Trace.Add($"R,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)}"); return (int)Input[7]; },
            RequiredEnergy = type => { Trace.Add("E," + type); return _energy[type]; },
        };

        /// <summary>방향별81DWORD를 원문 바이트 순서로 표현한다.</summary>
        public string AlliesHex()
        {
            var raw = new byte[324];
            // 표의 모든 방향을 순서대로 복사한다.
            for (int index = 0; index < Allies.Length; index++) Put(raw, index * 4, Allies[index]);
            return Hex(raw);
        }

        /// <summary>실제 각 할당의 토큰·크기·전체 바이트Adler를 원본 형식으로 쓴다.</summary>
        public string Blocks() => string.Join(';', Memory.Allocations.Select(block => $"{block.Token},{block.Size},{X86Fixture.Adler32(Memory.Bytes(block.Token))}"));
    }

    /// <summary>원본160입력의 본체·Player·방향별 동맹·섬·18할당과17목록 역순 해제를 전수 대조한다.</summary>
    [Fact]
    public void Native160()
    {
        string[][] rows = X86Fixture.Read("ai-brain").Where(row => row[0] != "input").ToArray();
        Assert.Equal(160, rows.Length);
        Assert.Equal(160, rows.Select(row => row[0]).Distinct().Count());
        // 기대 출력은 실행 뒤에만 읽으며 모든 독립 입력을 끝까지 대조한다.
        X86Fixture.CheckRows(rows, row =>
        {
            var scene = new Scene(row[0]);
            uint token = scene.Memory.Allocate(RawAiBrainConstructor.BytesCount);
            scene.Runtime.Initialize(token, (uint)scene.Input[0], scene.Profile);
            bool equal = Hex(scene.Runtime.Bytes(token)) == row[1] && Hex(scene.Records) == row[2]
                && scene.AlliesHex() == row[3] && Hex(scene.IslandBytes) == row[4] && scene.Blocks() == row[5]
                && (scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace)) == row[7];
            scene.Runtime.Destruct(token);
            equal &= string.Join(',', scene.Memory.Frees) == row[6] && scene.Memory.Contains(token) && Hex(scene.Runtime.Bytes(token)) == row[1];
            scene.Runtime.FreeBody(token);
            return equal && !scene.Memory.Contains(token);
        });
    }

    /// <summary>초기 갱신 미연결과 생성 도중 실패는 가짜brain부착·목록 누수를 만들지 않는다.</summary>
    [Fact]
    public void OwnedLifetimeAndMissingInitialUpdate()
    {
        var scene = new Scene("1,8,5,0,256,1,70,2,19");
        var wrapper = new RawAiCreate(OriginalEdition.Patch1078, scene.Participation,
            new AiCreateHooks { Construct = (owner, settings, bytes) => scene.Runtime.Construct(owner, settings.Span, bytes) });
        Assert.Throws<InvalidOperationException>(() => wrapper.Create(1, scene.Profile));
        Assert.Empty(scene.Memory.Allocations);
        uint token = scene.Runtime.Construct(1, scene.Profile);
        Assert.Equal(18, scene.Memory.Allocations.Count);
        Assert.Equal(0u, scene.Participation.Brain(1));
        scene.Participation.AttachBrain(1, token);
        scene.Participation.RemoveBrain(1);
        Assert.False(scene.Memory.Contains(token));
        Assert.Equal(0u, scene.Participation.Brain(1));
        Assert.NotEqual(0, scene.Records[180 + 0x50] & 0x20);
        Assert.Equal(18, scene.Memory.Frees.Count);
        var missing = new Scene("1,8,5,1,256,1,70,2,19");
        var incomplete = new RawAiBrainConstructor(missing.Memory, missing.Player, missing.Participation, missing.Islands,
            missing.Allies, missing.Types.Take(48).ToArray(), missing.Globals(), new AiBrainConstructorHooks());
        Assert.Throws<InvalidOperationException>(() => incomplete.Construct(1, missing.Profile));
        Assert.Equal(18, missing.Memory.Allocations.Count);
        Assert.Equal(18, missing.Memory.Frees.Count);
        Assert.Equal(0u, missing.Participation.Brain(1));
    }

    /// <summary>두 무효 그래프 쓰기 사이의 현재BYTE변화와 먼저 완료한 초기화를 검증한다.</summary>
    [Fact]
    public void ReadsCurrentByteAtBothGraphWrites()
    {
        var scene = new Scene("1,8,5,0,256,1,70,2,19");
        uint token = scene.Memory.Allocate(RawAiBrainConstructor.BytesCount);
        int calls = 0;
        var globals = scene.Globals(() =>
        {
            calls++;
            uint value = scene.InvalidGraph;
            if (calls == 1) scene.InvalidGraph = 17;
            else { Assert.Equal(254u, Read(scene.Memory.Bytes(token), 0x298)); Assert.Equal(0u, Read(scene.Memory.Bytes(token), 0x2a8)); }
            return value;
        });
        var runtime = new RawAiBrainConstructor(scene.Memory, scene.Player, scene.Participation, scene.Islands,
            scene.Allies, scene.Types.Take(48).ToArray(), globals, scene.Hooks());
        runtime.Initialize(token, 1, scene.Profile);
        Assert.Equal(2, calls);
        Assert.Equal(254u, Read(runtime.Bytes(token), 0x298));
        Assert.Equal(17u, Read(runtime.Bytes(token), 0x2ac));
        runtime.Destruct(token); runtime.FreeBody(token);
        Assert.False(scene.Memory.Contains(token));
    }

    /// <summary>현재 그래프 조회가 없으면 본체 할당 전에 연결 오류로 거부한다.</summary>
    [Fact]
    public void RejectsMissingCurrentGraphGetter()
    {
        var scene = new Scene("1,8,5,0,256,1,70,2,19");
        Assert.Throws<ArgumentException>(() => new RawAiBrainConstructor(scene.Memory, scene.Player, scene.Participation,
            scene.Islands, scene.Allies, scene.Types.Take(48).ToArray(),
            new AiBrainConstructorGlobals { LocalOwner = () => 8, NoRegion = () => 127 }, scene.Hooks()));
        Assert.Empty(scene.Memory.Allocations);
    }
}
