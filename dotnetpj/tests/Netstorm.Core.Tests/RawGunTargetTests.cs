using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

// 독립 원본 관측의 모든 판단/사정선/진단 사건을 같은 실제 Finder로 재생한다.
namespace Netstorm.Core.Tests;

/// <summary>두 PE의3454관측과 현재 Player/실제 Gun Frame의 연결을 검사한다.</summary>
public sealed class RawGunTargetTests
{
    /// <summary>원본 DWORD 십진수를 읽는다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>입력 DWORD를 float 비트로 해석한다.</summary>
    private static float F(uint bits) => BitConverter.UInt32BitsToSingle(bits);
    /// <summary>정렬되지 않은 실제 DWORD만 기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>사각형의 실제 네 float 비트를 왼쪽/위/오른쪽/아래 순서로 쓴다.</summary>
    private static string RectBits(GunTargetRect rect) => string.Join(',', new[] { rect.Left, rect.Top, rect.Right, rect.Bottom }.Select(BitConverter.SingleToUInt32Bits));
    /// <summary>원본에 제공한 동일 raw 입력을 현재 Pool/Hash/TYPE/SHP에 옮기는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 판본의 실제 물리 슬롯이다. 고정 번호 입력은 RawSlot으로 명시하며 할당 수명 검사는 아니다.</summary>
        internal RawSidPool Pool { get; }
        /// <summary>실제 네 단계 공간 해시다.</summary>
        internal RawSquidHash Hash { get; } = new();
        /// <summary>현재 HP/group/발자국/핫스폿 표다.</summary>
        internal OriginalTypeRecord[] Types { get; }
        /// <summary>현재 사거리/테마/방향/높이 표다.</summary>
        internal GunTargetType[] Targets { get; }
        /// <summary>현재 실제 포구 헤더의 번호별 표다.</summary>
        internal SquidDisplayShape[] Shapes { get; }
        /// <summary>매 준비 후 사용하는 독립 현재 전역이다.</summary>
        internal GunTargetState State { get; private set; } = new();
        /// <summary>실제 그리기 요청 순서와 비트를 기록한다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>같은 두 PE의 자료 폭만 준비하며 출력 판단은 만들지 않는다.</summary>
        internal Scene(bool patch)
        {
            Pool = new(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            Types = new OriginalTypeRecord[patch ? 188 : 171]; Targets = new GunTargetType[Types.Length]; Shapes = new SquidDisplayShape[Types.Length];
        }
        /// <summary>고정 native 입력을 실제 슬롯/해시 체인의 머리에 같은 순서로 넣는다.</summary>
        private void Object(int sid, byte type, byte extra, uint xb, uint yb, int width, int height, int extension, int level)
        {
            Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); raw[10] = type; raw[Pool.Edition == OriginalEdition.Patch1078 ? 40 : 35] = extra;
            Put(raw, 14, xb); Put(raw, 18, yb); Types[type].FootX = width; Types[type].FootY = height; Targets[type].HeightExtension = extension;
            int scale = level == 0 ? 1 : level == 1 ? 2 : level == 2 ? 4 : 16;
            ref ushort head = ref Hash.Cell(level, (int)F(xb) / scale, (int)F(yb) / scale);
            BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
        }
        /// <summary>기대 열을 읽지 않고18개의 독립 입력 칸만 옮긴다.</summary>
        internal void Prepare(uint[] input)
        {
            Assert.Equal(18, input.Length); Events.Clear(); Hash.Reset(); State = new();
            // 매 입력마다 현재 배열의 객체를 바꿔 이전 관측 상태를 남기지 않는다.
            for (int i = 0; i < Types.Length; i++) { Types[i] = new(); Targets[i] = new(); Shapes[i] = new(); }
            // 원본 하네스의 비등록 후보도 이전 타입/상태를 남기지 않는다.
            foreach (int sid in new[] { 60, 5999, 6000, 14999, 15000, 15050, 15060, 15070 }) Pool.RawSlot(sid).Clear();
            uint scene = input[15]; int target = (int)input[17], width = scene != 0 ? 3 : 1, height = scene != 0 ? 2 : 1, extension = scene == 2 ? 3 : 0;
            if (input[0] == 0 && scene == 3)
            {
                Object(15070, 88, 0, BitConverter.SingleToUInt32Bits(50.75f), BitConverter.SingleToUInt32Bits(56.5f), 2, 2, 1, 2);
                Object(15060, 87, 0, BitConverter.SingleToUInt32Bits(55.75f), BitConverter.SingleToUInt32Bits(52.5f), 1, 1, 0, 1);
                Object(target, 86, (byte)input[7], input[5], input[6], width, height, extension, 1);
            }
            else
            {
                Object(target, 86, (byte)input[7], input[5], input[6], width, height, extension, 1);
                if (input[0] == 0)
                {
                    Object(15060, 87, 0, BitConverter.SingleToUInt32Bits(53.75f), BitConverter.SingleToUInt32Bits(52.5f), 1, 1, 0, 1);
                    Object(15070, 88, 0, BitConverter.SingleToUInt32Bits(50.75f), BitConverter.SingleToUInt32Bits(56.5f), 2, 2, 1, 2);
                }
            }
            // 실제 source 타입별 group만 구분하고 같은 독립 metadata 입력을 공급한다.
            foreach (int number in new[] { 71, 128, 129, 140, 148, 83 })
            {
                OriginalTypeRecord type = Types[number]; type.Group = number is 128 or 129 ? 1 : number == 83 ? 3 : 0; type.FootX = type.FootY = 1;
                type.HotspotX = scene != 0 ? 2 : 0; type.HotspotY = scene != 0 ? -3 : 0;
                Targets[number].Range = F(input[4]); Targets[number].Theme = unchecked((int)input[3]); Targets[number].DirectionFrames = [0, 1, 2, 3];
                SquidDisplayShape shape = Shapes[number]; shape.Loaded = true; shape.FrameCount = 4;
                // header+20/+22의 signed short 포구를 화면 기준점과 구분한다.
                for (int i = 0; i < 4; i++) shape.Frames.Add(new() { MuzzleX = (short)(scene != 0 ? 3 * i - 4 : 0), MuzzleY = (short)(scene != 0 ? i - 2 : 0) });
            }
            // 수량/소유자는 raw 현재값, 후보 플래그는 현재 TYPE 표다.
            foreach (int sid in new[] { target, 15060, 15070 })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw[Pool.Edition == OriginalEdition.Patch1078 ? 34 : 32] = (byte)input[11];
                if (Pool.Edition == OriginalEdition.Patch1078) Put(raw, 26, input[10]); else BinaryPrimitives.WriteUInt16LittleEndian(raw[26..], (ushort)input[10]);
                Types[raw[10]].Flags1 = input[8]; Types[raw[10]].Flags2 = sid == target ? input[9] : sid == 15060 ? 0x100000u : 0;
            }
            State.FriendlyFire = new(input[13] != 0); State.ShowRanges = input[16] != 0; State.AllyMasks[1] = (1u << 1) | (input[12] != 0 ? 1u << (int)input[11] : 0);
        }
        /// <summary>계산을 대체하지 않고 실제 출력 경계의 요청만 기록한다.</summary>
        internal GunTargetHooks Hooks() => new() { DrawRange = (rect, target) => Events.Add($"D:{(target ? 1 : 0)}:{RectBits(rect)}") };
        /// <summary>같은 현재 자료를 빌리는 실제 검색기다.</summary>
        internal RawGunTarget Logic(GunTargetHooks? hooks = null) => new(Pool, Hash, Types, Targets, Shapes, State, hooks);
    }
    /// <summary>두 판본을 원문 전체 입력과 현재 Player 조회기의 두 경로로 재생한다. 반복을 추가 관측으로 세지 않는다.</summary>
    [Theory]
    [InlineData(true, false)] [InlineData(false, false)] [InlineData(true, true)] [InlineData(false, true)]
    public void MatchesAllNativeJudgementsRaysAndEvents(bool patch, bool currentPlayers)
    {
        var scene = new Scene(patch); int count = 0, maskReads = 0, flagReads = 0;
        // 각 고유 원문 입력은 두 PE의 실제 반환/비트/출력 순서를 기대값으로 유지한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "guntarget-x86.tsv")))
        {
            if (line.Length == 0 || line[0] == '#') continue; string[] row = line.Split('\t');
            if (row[0] != (patch ? "originals" : "originalCD")) continue;
            uint[] input = row[1].Split(',').Select(N).ToArray(); scene.Prepare(input);
            if (currentPlayers)
            {
                uint[] masks = (uint[])scene.State.AllyMasks.Clone(); byte[] flags = (byte[])scene.State.PlayerFlags.Clone();
                scene.State.AllyMaskRead = owner => { maskReads++; return masks[owner]; }; scene.State.PlayerFlagsRead = owner => { flagReads++; return flags[owner]; };
                Array.Fill(scene.State.AllyMasks, uint.MaxValue); Array.Fill(scene.State.PlayerFlags, (byte)0x20);
            }
            RawGunTarget logic = scene.Logic(scene.Hooks());
            var query = new GunTargetQuery { X = 50.75f, Y = 52.5f, CenterX = 50.25f, CenterY = 52f, Type = input[1], Direction = unchecked((int)input[2]), Owner = 1, Frame = unchecked((int)input[14]) };
            int target = (int)input[17]; uint result = 0; GunTargetRect rect = default;
            switch (input[0])
            {
                case 0: result = logic.FindTarget(query.X, query.Y, query.Type, query.Direction, query.Owner); break;
                case 1: result = logic.MayTarget(query, target) ? 1u : 0; break;
                case 2: result = logic.MayShoot(query, target) ? 1u : 0; break;
                case 3: result = logic.MayFire(query, target) ? 1u : 0; break;
                case 4: rect = logic.CannonRay(query, query.Direction); break;
                case 5: rect = logic.Footprint(target); break;
                default: throw new InvalidOperationException("독립 원문 입력 종류 오류");
            }
            Assert.Equal($"{row[1]}\t{row[2]}\t{row[3]}\t{row[4]}", $"{row[1]}\t{result}\t{RectBits(rect)}\t{(scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events))}"); count++;
        }
        Assert.Equal(1727, count);
        if (currentPlayers) { Assert.True(maskReads > 0); Assert.True(flagReads > 0); }
    }
    /// <summary>진단 누락/Player bit20·함수 값 복사·Pool 혼용 경계를 실제 사정선으로 확인한다.</summary>
    [Fact]
    public void SelectedDiagnosticsAndAdapterKeepOriginalBoundaries()
    {
        var scene = new Scene(true); scene.Prepare([4,128,0,0,BitConverter.SingleToUInt32Bits(10.5f),BitConverter.SingleToUInt32Bits(55.75f),BitConverter.SingleToUInt32Bits(52.5f),0,0x10,0,2,2,0,0,1,0,1,15050]);
        var query = new GunTargetQuery { X = 50.75f, Y = 52.5f, CenterX = 50.25f, CenterY = 52, Type = 128, Owner = 1, Frame = 1 };
        RawGunTarget missing = scene.Logic(); Assert.Throws<InvalidOperationException>(() => missing.CannonRay(query, 0));
        GunTargetHooks hooks = scene.Hooks(); RawGunTarget logic = scene.Logic(hooks); hooks.DrawRange = (_, _) => throw new InvalidOperationException("복사 뒤 변경한 함수");
        _ = logic.CannonRay(query, 0); Assert.Single(scene.Events); scene.Events.Clear(); scene.State.PlayerFlags[1] = 0x20; _ = logic.CannonRay(query, 0); Assert.Empty(scene.Events);
        bool preserved = false; var input = new GunProcessHooks { RefreshRange = _ => preserved = true }; GunProcessHooks adapted = RawGunTarget.MakeGunTargetHooks(scene.Pool, logic, input); Assert.Null(input.FindTarget); Assert.NotNull(adapted.FindTarget); Assert.NotNull(adapted.Shoot); adapted.RefreshRange!(15050); Assert.True(preserved);
        Assert.Throws<ArgumentException>(() => RawGunTarget.MakeGunTargetHooks(new(24000), logic, new()));
    }
    /// <summary>실제 Gun.Frame이 현재 raw 발사 문맥과 MayFire를 소비해 수락/거부 상태를 바꾼다.</summary>
    [Fact]
    public void ActualGunFrameUsesCurrentTargetAndFrame()
    {
        var scene = new Scene(true); scene.Prepare([3,128,0,0,BitConverter.SingleToUInt32Bits(10.5f),BitConverter.SingleToUInt32Bits(55.75f),BitConverter.SingleToUInt32Bits(52.5f),0,0x10,0,2,2,0,0,3,0,0,15050]);
        Span<byte> raw = scene.Pool.RawSlot(15000); raw.Clear(); raw[10] = 128; raw[34] = 1; Put(raw, 14, BitConverter.SingleToUInt32Bits(50.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(52.5f)); Put(raw, 36, 3);
        RawGunTarget target = scene.Logic(scene.Hooks()); var state = new GunProcessState { Now = 10 }; List<string> events = [];
        var boundaries = new GunProcessHooks
        {
            ParentBlocked = sid => (scene.Pool.Slot(sid)[40] & 0x20) != 0,
            ShotContext = sid =>
            {
                ReadOnlySpan<byte> image = scene.Pool.Slot(sid); var center = target.Center(sid);
                return [BinaryPrimitives.ReadUInt32LittleEndian(image[14..]), BinaryPrimitives.ReadUInt32LittleEndian(image[18..]), BitConverter.SingleToUInt32Bits(center.X), BitConverter.SingleToUInt32Bits(center.Y), image[10], 0, image[34], BinaryPrimitives.ReadUInt32LittleEndian(image[36..])];
            },
            FiringInterval = _ => 2.25, RemoveDependency = (form, sid) => events.Add($"D:{form}:{sid}"), RepopForm = _ => events.Add("P"),
        };
        var gun = new RawGunProcess(scene.Pool, scene.Types, state, RawGunTarget.MakeGunTargetHooks(scene.Pool, target, boundaries)); var fields = new GunProcessFields(); gun.Initialize(fields, 15000);
        Put(fields.Bytes, 44, 5); Put(fields.Bytes, 48, 15000); BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(24), 15050); Put(fields.Bytes, 28, 7);
        gun.Frame(fields); Assert.Equal(2u, fields.Stage()); Assert.Equal((ushort)15050, fields.Target()); Assert.Equal(12.25, fields.Time()); Assert.Empty(events);
        Put(raw, 36, 1); Put(fields.Bytes, 28, 7); BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(16), state.Now);
        gun.Frame(fields); Assert.Equal(1u, fields.Stage()); Assert.Equal((ushort)0, fields.Target()); Assert.Equal(12.25, fields.Time()); Assert.Equal(new[] { "D:5:15050", "P" }, events);
    }
    /// <summary>같은 현재 표의 교체/검사 플래그와 빈 SID의 원문 경계를 확인한다.</summary>
    [Fact]
    public void CurrentTablesCheckingAndEmptySidKeepSourceContract()
    {
        var scene = new Scene(true); scene.Prepare([2,83,0,0,BitConverter.SingleToUInt32Bits(10.5f),BitConverter.SingleToUInt32Bits(55.75f),BitConverter.SingleToUInt32Bits(52.5f),0,0x10,0,2,2,0,0,1,0,0,15050]);
        RawGunTarget logic = scene.Logic(); var query = new GunTargetQuery { X = 50.75f, Y = 52.5f, CenterX = 50.25f, CenterY = 52, Type = 83, Owner = 1 };
        Assert.True(logic.MayTarget(query, 15050)); scene.Pool.RawSlot(15050)[11] = 3; Assert.True(logic.MayTarget(query, 15050)); scene.Types[86] = new() { Flags1 = 0 }; Assert.False(logic.MayTarget(query, 15050)); scene.Types[0].Flags1 = 0; Assert.False(logic.MayTarget(query, 0));
        scene.Types[83].Group = 2; scene.State.Checking = new(true); Assert.Throws<InvalidOperationException>(() => logic.MayShoot(query, 15050)); scene.Types[83].Flags2 = 0x100000; Assert.True(logic.MayShoot(query, 15050));
        Assert.Throws<ArgumentException>(() => new RawGunTarget(scene.Pool, scene.Hash, scene.Types[..171], scene.Targets, scene.Shapes, scene.State)); Assert.Throws<ArgumentException>(() => GunTargetQuery.FromWords([1]));
    }

    /// <summary>현재 전역 조회기 복귀 뒤의 TYPE 읽기는 같은 원문 레코드 주소의 변경을 반영한다.</summary>
    [Fact]
    public void TypeReplacementDuringCurrentReadsRemainsVisible()
    {
        var scene = new Scene(true); scene.Prepare([2,83,0,0,BitConverter.SingleToUInt32Bits(10.5f),BitConverter.SingleToUInt32Bits(55.75f),BitConverter.SingleToUInt32Bits(52.5f),0,0x10,0,1,2,0,0,1,0,0,15050]);
        RawGunTarget logic = scene.Logic(); var query = new GunTargetQuery { X = 50.75f, Y = 52.5f, CenterX = 50.25f, CenterY = 52, Type = 83, Owner = 1 };
        scene.State.AllyMaskRead = _ => { scene.Types[86] = new() { Flags1 = 0x10, Flags2 = 0x200000 }; return 2; };
        Assert.False(logic.MayTarget(query, 15050));
        scene.Types[83].Group = 2; scene.State.Checking = new(() => { scene.Types[83] = new() { Group = 2, Flags2 = 0x100000 }; return 1u; });
        Assert.True(logic.MayShoot(query, 15050));
    }
}
