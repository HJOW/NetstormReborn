using Netstorm.Core.Original;

// 장기 저장소의 기본 장치가 외부 Kernel 결합과 실제 소비까지 지연되는지 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>Host 이전 장치 등록과 현재 World 우선 선택을 실제 저장소에서 검사한다.</summary>
public sealed class RawProcessStorageDefaultsTests
{
    /// <summary>같은 Pool/TYPE/공장/삭제기와 외부 Kernel을 갖는 검사 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 같은 슬롯을 소유한다.</summary>
        public RawSidPool Pool { get; } = new(32768, true);
        /// <summary>실제 원문 생성자 주소를 갖는 타입 표다.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        /// <summary>실제 장기 저장소다.</summary>
        public RawProcessHostStorage Storage { get; }
        /// <summary>같은 슬롯을 생성하는 공장이다.</summary>
        public RawSquidFactory Factory { get; }
        /// <summary>현재 삭제 부모로 빌릴 실제 객체다.</summary>
        public RawSquidDestroy Destroy { get; }
        /// <summary>이 장면에 필요한 물리만 구성하고 Host는 요구하지 않는다.</summary>
        public Scene()
        {
            Storage = new(Pool, Types, new RawAuthorityState(), null, new RawGameAuxState());
            Factory = new(Pool, Types);
            Destroy = new(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
        }
    }

    /// <summary>세 기본 장치를 보관한 뒤 같은 외부 Kernel/Frame을 연결하며 실제 getter는 Host 생성 때도 실행하지 않는다.</summary>
    [Fact]
    public void DefaultsWaitForExternalKernelAndConsumption()
    {
        var scene = new Scene(); int reads = 0;
        var assets = new SquidDestroyHooks();
        scene.Storage.BindLifecycleDefaults(() => { reads++; return scene.Destroy; }, (_, _, _, value) => value + 2, () => { reads++; return assets; });
        scene.Storage.BindFormFactoryDefaults(() => { reads++; return scene.Factory; }, () => { reads++; return 0; });
        scene.Storage.BindFormTransmissionDefaults(new ProcessFormTransmission
        {
            WriteData = (_, _) => { reads++; return 0; }, WriteFullData = (_, _) => { reads++; return 0; },
            Packet = _ => { reads++; return []; }, Transmit = _ => { reads++; },
        }, (_, _, _) => { reads++; });
        var kernel = new Kernel(16); var clock = new FrameTimeBox();
        scene.Storage.BindDevices(kernel, clock);
        SquidProcessHost host = scene.Storage.Host();
        Assert.Same(host, scene.Storage.Host()); Assert.Same(kernel, scene.Storage.Kernel()); Assert.Equal(0, reads);
        Assert.Equal(6f, host.Regular()!(0, 0, 0, 4)); Assert.Equal(0, reads);
        Assert.Same(scene.Destroy, host.Destroy()); Assert.Same(assets, host.Asset()); Assert.Equal(2, reads);
        Assert.Throws<InvalidOperationException>(() => host.Runtime());
    }

    /// <summary>기본 공장 조회를 실제 생성까지 늦추고 외부 Kernel과 같은 Pool의 실제 Form을 부착한다.</summary>
    [Fact]
    public void StoredFactoryCreatesActualFormAfterDeviceBinding()
    {
        var scene = new Scene(); int reads = 0;
        scene.Storage.BindFormFactoryDefaults(() => { reads++; return scene.Factory; }, () => 0);
        var kernel = new Kernel(16); scene.Storage.BindDevices(kernel, new FrameTimeBox());
        SquidProcessHost host = scene.Storage.Host(); Assert.Equal(0, reads);
        int form = host.CreateOrTakeForm(0x10, 0);
        Assert.Equal(1, reads); Assert.Equal(2, scene.Pool.Slot(form)[10]); Assert.Same(kernel, scene.Storage.Kernel());
    }

    /// <summary>잘못된 최초 등록은 후속 올바른 등록을 막지 않으며 중복과 Host 생성 뒤 등록을 거부한다.</summary>
    [Fact]
    public void InvalidDuplicateAndLateRegistrationPreserveContract()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentException>(() => scene.Storage.BindLifecycleDefaults(null!, (_, _, _, p) => p, () => new()));
        Assert.Throws<ArgumentException>(() => scene.Storage.BindFormFactoryDefaults(() => scene.Factory, null!));
        Assert.Throws<ArgumentException>(() => scene.Storage.BindFormTransmissionDefaults(new(), (_, _, _) => { }));
        scene.Storage.BindLifecycleDefaults(() => scene.Destroy, (_, _, _, p) => p, () => new());
        scene.Storage.BindFormFactoryDefaults(() => scene.Factory, () => 0);
        Assert.Throws<ArgumentException>(() => scene.Storage.BindLifecycleDefaults(() => scene.Destroy, (_, _, _, p) => p, () => new()));
        Assert.Throws<ArgumentException>(() => scene.Storage.BindFormFactoryDefaults(() => scene.Factory, () => 0));
        _ = scene.Storage.Host();
        Assert.Throws<ArgumentException>(() => scene.Storage.BindFormTransmissionDefaults(Transmission(), (_, _, _) => { }));
    }

    /// <summary>네 함수가 모두 연결된 값형 전송 입력을 제공한다.</summary>
    private static ProcessFormTransmission Transmission() => new()
    {
        WriteData = (_, _) => 4, WriteFullData = (_, _) => 7, Packet = _ => [1, 2], Transmit = _ => { },
    };

    /// <summary>등록 시 함수 슬롯을 복사하고 실제 현재 대여의 부분 장치를 기본값으로 채우지 않는다.</summary>
    [Fact]
    public void StoredTransmissionCopiesAndRespectsCurrentRuntime()
    {
        var scene = new Scene(); ProcessFormTransmission input = Transmission();
        scene.Storage.BindFormTransmissionDefaults(input, (_, _, _) => { }); input.WriteData = (_, _) => 99;
        Assert.Throws<ArgumentException>(() => scene.Storage.BindFormTransmissionDefaults(input, (_, _, _) => { }));
        SquidProcessHost host = scene.Storage.Host(); Assert.Equal(4, host.Transmission()!.WriteData!(new SquidProcess(), false));
        var runtime = new ProcessHostRuntimeBindings { Destroy = scene.Destroy, Transmit = new() { Packet = _ => [8] } };
        using (RawProcessHostStorage.Use lease = scene.Storage.Bind(runtime))
        {
            Assert.Null(host.Transmission()!.WriteData); Assert.Equal(new byte[] { 8 }, host.Transmission()!.Packet!(1));
        }
        Assert.Equal(4, host.Transmission()!.WriteData!(new SquidProcess(), false));
    }
}
