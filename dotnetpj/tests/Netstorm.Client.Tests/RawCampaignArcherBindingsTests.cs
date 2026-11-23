using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using SidAllocation = Netstorm.Core.Simulation.SidAllocation;

// 실제 World/Core/Host 공장 조립과 현재 시계/역할의 전달을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원문 파생 공장·동일 Pool·현재 역할·선택되지 않은 효과의 경계를 확인한다.</summary>
public sealed class RawCampaignArcherBindingsTests
{
    /// <summary>같은 실제 World/Core와 명시 TYPE/방향/프레임 입력을 소유한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 전체 물리 저장소다.</summary>
        internal RawSurfaceStorage Storage { get; }
        /// <summary>같은 Pool/Host를 실제로 대여한 월드다.</summary>
        internal RawSurfaceWorld World { get; }
        /// <summary>월드의 참조 보기이며 별도 풀을 만들지 않는다.</summary>
        internal RawWorldCore Core { get; }
        /// <summary>현재 아처 방향/클러스터 자료다.</summary>
        internal ArcherTypeData[] Directions { get; } = Enumerable.Range(0, 188).Select(_ => new ArcherTypeData(20, 80)).ToArray();
        /// <summary>실제 파생이 빌리는 명시 문자 프레임 표다.</summary>
        internal PriestPlainCanonType[] Frames { get; } = Enumerable.Range(0, 188).Select(_ => new PriestPlainCanonType()).ToArray();
        /// <summary>현재 미사일의 보충 자료다.</summary>
        internal MissileTypeData[] Missiles { get; } = Enumerable.Range(0, 188).Select(_ => new MissileTypeData()).ToArray();
        /// <summary>외부에서 빌려주는 아처 현재 시계다.</summary>
        internal ArcherAnimationState AnimationState { get; } = new();
        /// <summary>외부에서 빌려주는 미사일 현재 시계다.</summary>
        internal MissileState MissileState { get; } = new();
        /// <summary>같은 단일 게임 난수다.</summary>
        internal GameRandom Random { get; } = new(123);
        /// <summary>실제 표적 판정과 현재 중심을 제공한다.</summary>
        internal RawGunTarget Target { get; }
        /// <summary>생성 중 호출되지 않아야 하는 외부 효과와 실제 월드 자료를 준비한다.</summary>
        internal Scene()
        {
            OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i), FootX = 1, FootY = 1, Group = i is 128 or 129 or 131 ? 1 : 0 }).ToArray();
            RiftTypeFrames[] frames = types.Select(_ => new RiftTypeFrames([])).ToArray(); SquidDisplayShape[] shapes = types.Select(_ => new SquidDisplayShape()).ToArray();
            Storage = new(OriginalEdition.Patch1078, types, frames, shapes); World = new(OriginalEdition.Patch1078, Storage.Types, 97, Storage, frames); Core = World.Core();
            Target = new(Core.Pool, Core.Hash, Core.Types, types.Select(_ => new GunTargetType()).ToArray(), Core.Shapes, new());
        }
        /// <summary>같은 실제 자료와 외부 시계를 빌려 전투 공장을 만든다.</summary>
        internal RawCampaignArcherBindings Bind(RawCampaignArcherEffects? effects = null) => new(World, Target, Random, Directions, Frames, Missiles, AnimationState, MissileState, effects ?? new());
        /// <summary>검사할 파생 공장 앞에 실제 Host의 기존 Gun과 raw 부모를 부착한다.</summary>
        internal GunProcess Gun(byte type)
        {
            int parent = Core.Pool.Allocate(SidAllocation.Client); Span<byte> raw = Core.Pool.AllocatedBytes(parent); raw.Clear(); raw[10] = type; raw[34] = 1;
            BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x501ea0); BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 50); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 60);
            var logic = new RawGunProcess(Core.Pool, Core.Types, new(), new()); var fields = new GunProcessFields(); logic.Initialize(fields, (ushort)parent);
            return Assert.IsType<GunProcess>(Core.Processes.Attach(new GunProcess(logic, fields), 34, parent, 0x10));
        }
        /// <summary>실제 Kernel 슬롯을 번호순으로 읽어 Gun 외에 부착된 단일 파생을 찾는다.</summary>
        internal SquidProcess Other(GunProcess gun)
        {
            List<BaseProcess> others = [];
            // 공개된 실제 Kernel 슬롯만 조회하며 별도 예상 프로세스 표를 만들지 않는다.
            for (uint id = 1; id <= Core.Kernel.Capacity(); id++)
                if (Core.Kernel.Get(id) is { } process && !ReferenceEquals(process, gun)) others.Add(process);
            return Assert.IsAssignableFrom<SquidProcess>(Assert.Single(others));
        }
        /// <summary>현재 실제 Kernel 사용을 먼저 끝내고 World 대여를 반납한다.</summary>
        public void Dispose() { Core.Kernel.DrainOwnedForShutdown(); World.Dispose(); }
    }
    /// <summary>각 원문 TYPE/단계가 실제 같은 Host의 파생/시각/부모로 연결되는지 확인한다.</summary>
    [Theory]
    [InlineData(71,2,nameof(ArcherAnimation))] [InlineData(71,5,nameof(ArcherAnimation))]
    [InlineData(128,2,nameof(GunAnimation))] [InlineData(128,8,nameof(GunAnimation))] [InlineData(128,5,nameof(CannonFire))]
    [InlineData(131,2,nameof(GunAnimation))] [InlineData(131,5,nameof(CannonFire))]
    [InlineData(148,5,nameof(CannonFire))] [InlineData(140,2,nameof(ArcherAnimation))] [InlineData(140,5,nameof(CrossbowAnimation))] [InlineData(129,5,nameof(IceCannonFire))]
    public void ActualFactoriesAttachDerivedProcessesToSameHost(byte type, uint stage, string expected)
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); binding.PrepareFrame(new() { Game = 7.5, Delta = .125 });
        GunProcess gun = scene.Gun(type); GunProcessHooks hooks = binding.GunHooks(new()); hooks.StartAnimation!(stage, (ushort)gun.Form(), 60);
        SquidProcess animation = scene.Other(gun);
        Assert.Equal(expected, animation.GetType().Name); Assert.Equal(gun.Parent(), animation.Parent()); Assert.Same(animation, scene.Core.Processes.FromForm(animation.Form())); Assert.Equal(2, scene.Core.Kernel.Size());
        double time = animation switch { ArcherAnimation a => a.Fields.Time(), GunAnimation a => a.Fields.Time(), CannonFire a => a.Fields.Time(), CrossbowAnimation a => a.Fields.Time(), IceCannonFire a => a.Fields.Common.Time(), _ => throw new InvalidOperationException("예상 밖 실제 공장 파생") };
        Assert.Equal(7.5, time);
        if (animation is IceCannonFire ice) Assert.Equal(11.5, BinaryPrimitives.ReadDoubleLittleEndian(ice.Fields.Extra.AsSpan(8)));
        if (animation is ArcherAnimation archer && type == 71 && stage == 5) Assert.Equal(BitConverter.SingleToUInt32Bits(360), BinaryPrimitives.ReadUInt32LittleEndian(archer.Fields.Bytes.AsSpan(64)));
    }
    /// <summary>세 공유 역할의 실제 주소를 빌려 일반 준비 뒤에도 현재 변경을 소비한다.</summary>
    [Theory]
    [InlineData(0,72,nameof(ArcherAnimation))] [InlineData(1,127,nameof(GunAnimation))] [InlineData(2,139,nameof(ArcherAnimation))]
    public void CurrentSharedRoleSelectsNewParentAfterGeneralPreparation(int role, byte type, string expected)
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); RawGameAuxState aux = scene.Storage.GameAux();
        UIntBox current = role == 0 ? aux.SunArcherTypeBox : role == 1 ? aux.SunCannonTypeBox : aux.WindArcherTypeBox; current.Value = type; aux.ResetForGeneralWorld(); Assert.Equal((uint)type, current.Value);
        GunProcess gun = scene.Gun(type); binding.GunHooks(new()).StartAnimation!(2, (ushort)gun.Form(), 60);
        Assert.Equal(expected, scene.Other(gun).GetType().Name);
        Assert.Same(aux.SunArcherTypeBox, scene.AnimationState.CurrentType); Assert.Equal(role == 0 ? (uint)type : 71u, binding.Animation.Type());
    }
    /// <summary>선택되지 않은 효과/난수는 실행하지 않고 같은 외부 시계/식별자를 보존한다.</summary>
    [Fact]
    public void ConstructorAndClockPreserveRealOwnersAndUnselectedEffects()
    {
        using var scene = new Scene(); var effects = new RawCampaignArcherEffects
        {
            Animation = new() { Common = new() { Sound = (_, _) => throw new InvalidOperationException("생성 중 음향"), TargetPoint = (_, _) => throw new InvalidOperationException("생성 중 배리어") } },
            Missile = new() { Muzzle = _ => throw new InvalidOperationException("생성 중 미사일"), Damage = (_, _, _, _) => throw new InvalidOperationException("생성 중 피해") },
            IceCooling = _ => throw new InvalidOperationException("생성 중 냉각"),
        };
        RawCampaignArcherBindings binding = scene.Bind(effects); Assert.Same(scene.Core.Pool, binding.Pool); Assert.Same(scene.Core.Pool, binding.Missile.Pool); Assert.Same(scene.Core.Pool, binding.Animation.Pool);
        Assert.Equal(new GameRandom(123).Next(65536), scene.Random.Next(65536)); Assert.Null(effects.Beam.Random); Assert.Null(effects.Missile.Create); Assert.Null(effects.Animation.Common.Unpop);
        binding.PrepareFrame(new() { Game = 23.75, Delta = .25 }); Assert.Equal(23.75, scene.AnimationState.Now); Assert.Equal(.25, scene.AnimationState.Delta); Assert.Equal(23.75, scene.MissileState.Now);
        Assert.Throws<ArgumentException>(() => binding.PrepareFrame(new() { Game = double.NaN, Delta = 1 })); Assert.Throws<ArgumentException>(() => binding.PrepareFrame(new() { Game = 1, Delta = -.1 })); Assert.Equal(23.75, scene.MissileState.Now);
        var other = new RawSidPool(24000); var target = new RawGunTarget(other, new(), scene.Core.Types, Enumerable.Range(0,188).Select(_ => new GunTargetType()).ToArray(), scene.Core.Shapes, new());
        Assert.Throws<ArgumentException>(() => new RawCampaignArcherBindings(scene.World, target, scene.Random, scene.Directions, scene.Frames, scene.Missiles, scene.AnimationState, scene.MissileState, new()));
    }
    /// <summary>미지원 파생은 원래 함수 값으로 넘기고 새 공장 설치가 입력 함수를 바꾸지 않는다.</summary>
    [Fact]
    public void FallbackAndGeneralRegularKeepExactArguments()
    {
        using var scene = new Scene(); RawCampaignArcherBindings binding = scene.Bind(); GunProcess gun = scene.Gun(86); int calls = 0;
        var source = new GunProcessHooks { StartAnimation = (stage, form, target) => { Assert.Equal(9u, stage); Assert.Equal(gun.Form(), form); Assert.Equal((ushort)61, target); calls++; } };
        GunProcessHooks hooks = binding.GunHooks(source); hooks.StartAnimation!(9, (ushort)gun.Form(), 61); Assert.Equal(1, calls); Assert.NotSame(hooks, source);
        Assert.Throws<InvalidOperationException>(() => binding.GunHooks(new()).StartAnimation!(9, (ushort)gun.Form(), 61));
        float payload = BitConverter.UInt32BitsToSingle(0xffc12345); int parent = gun.Parent();
        float result = binding.Handler((sid, ev, count, value) => { Assert.Equal(parent, sid); Assert.Equal(77u, ev); Assert.Equal(9u, count); return value; })(parent, 77, 9, payload);
        Assert.Equal(0xffc12345u, BitConverter.SingleToUInt32Bits(result)); Assert.Throws<InvalidOperationException>(() => binding.Handler()(parent, 77, 9, payload));
    }
}
