using System.Text;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>Guide의 스크립트 소유·모드·설정 알림 순서를 실제 Config로 검사한다.</summary>
public sealed class GuideMissionScriptLifetimeTests
{
    /// <summary>콜백에서 관찰할 실제 현재 미션과 Native 모드다.</summary>
    private sealed class Owner
    {
        /// <summary>현재 미션 소유 슬롯이다.</summary>
        public MissionScript? Mission;
        /// <summary>현재 Native 미션 모드다.</summary>
        public bool Mode;
    }

    /// <summary>실제 설정 객체와 선택 가능한 스크립트 reader를 만든다.</summary>
    private static ConfigInterface MakeConfig()
    {
        var config = new ConfigInterface(path => path == "d\\setup.cfg" ? Encoding.Latin1.GetBytes("missionSpec=\"d/{local.1}.english\"\n[END]\n") : null);
        config.Startup(new ConfigInterface.StartupOptions());
        return config;
    }

    /// <summary>정상 반납 알림에서 스크립트 등록과 모드가 이미 반납됐고 빈 재반납은 무동작이다.</summary>
    [Fact]
    public void ReleasePublishesAfterDisposalAndIsIdempotent()
    {
        ConfigInterface config = MakeConfig();
        var owner = new Owner();
        owner.Mission = GuideMissionScriptLifetime.Create(config, "TEST", ref owner.Mode, _ => Encoding.Latin1.GetBytes("missionType=Tutorial\n"));
        Assert.True(owner.Mode);
        Assert.Equal(1, config.GetInt("global.inMission"));
        int calls = 0;
        config.OnChanged = () =>
        {
            calls++;
            Assert.Null(owner.Mission);
            Assert.Null(config.Registry.FindByName("mission"));
            Assert.False(owner.Mode);
            Assert.Equal(0, config.GetInt("global.inMission"));
        };
        GuideMissionScriptLifetime.Release(ref owner.Mission, ref owner.Mode, config);
        GuideMissionScriptLifetime.Release(ref owner.Mission, ref owner.Mode, config);
        Assert.Equal(1, calls);
    }

    /// <summary>반납 알림 실패가 앞선 해제를 되돌리지 않고 다시 반납해도 추가 알림을 내지 않는다.</summary>
    [Fact]
    public void ReleaseNotificationFailureKeepsCompletedEffects()
    {
        ConfigInterface config = MakeConfig();
        var owner = new Owner();
        owner.Mission = GuideMissionScriptLifetime.Create(config, "TEST", ref owner.Mode);
        int calls = 0;
        config.OnChanged = () => { calls++; throw new InvalidOperationException("notification"); };
        Assert.Throws<InvalidOperationException>(() => GuideMissionScriptLifetime.Release(ref owner.Mission, ref owner.Mode, config));
        Assert.Null(owner.Mission);
        Assert.Null(config.Registry.FindByName("mission"));
        Assert.False(owner.Mode);
        Assert.Equal(0, config.GetInt("global.inMission"));
        GuideMissionScriptLifetime.Release(ref owner.Mission, ref owner.Mode, config);
        Assert.Equal(1, calls);
    }

    /// <summary>파일 읽기 실패는 생성 중 등록을 해제하고 모드를 건드리지 않는다. 생성 알림 실패도 지역 스크립트를 해제한다.</summary>
    [Fact]
    public void CreationFailureReleasesRegistrationAtBothBoundaries()
    {
        ConfigInterface config = MakeConfig();
        var owner = new Owner();
        Assert.Throws<InvalidOperationException>(() => GuideMissionScriptLifetime.Create(config, "TEST", ref owner.Mode, _ => throw new InvalidOperationException("reader")));
        Assert.Null(config.Registry.FindByName("mission"));
        Assert.False(owner.Mode);
        Assert.Equal(0, config.GetInt("global.inMission"));
        config.OnChanged = () => throw new InvalidOperationException("notification");
        Assert.Throws<InvalidOperationException>(() => GuideMissionScriptLifetime.Create(config, "TEST", ref owner.Mode));
        Assert.Null(config.Registry.FindByName("mission"));
        Assert.True(owner.Mode);
        Assert.Equal(1, config.GetInt("global.inMission"));
    }

    /// <summary>빌린 이름은 기존 미션 해제 뒤 읽고 첫 NUL까지 공장에 전달한다.</summary>
    [Fact]
    public void BeginReadsBorrowedNameAfterRelease()
    {
        string borrowed = "before", created = "";
        var hooks = new GuideMissionTransitionHooks
        {
            Current = () => true,
            Finish = () => { },
            Release = () => borrowed = "after\0ignored",
            Create = name => created = name,
        };
        GuideMissionTransitions.Begin(() => borrowed, hooks);
        Assert.Equal("after", created);
    }
    /// <summary>실제 미션 설정의 fork 값을 해제 전에 읽어 다음 스크립트를 만들고 종료 시 등록을 반납한다.</summary>
    [Fact]
    public void ForkConnectsTransitionToActualScriptLifetime()
    {
        ConfigInterface config = MakeConfig();
        var owner = new Owner();
        Func<string, byte[]?> source = path => path switch
        {
            "d/first.english" => Encoding.Latin1.GetBytes("fork1=second\n[END]\n"),
            "d/second.english" => Encoding.Latin1.GetBytes("missionType=Tutorial\n[END]\n"),
            _ => null,
        };
        int finished = 0;
        var hooks = new GuideMissionTransitionHooks
        {
            Current = () => owner.Mission != null,
            Finish = () => finished++,
            Release = () => GuideMissionScriptLifetime.Release(ref owner.Mission, ref owner.Mode, config),
            Create = name => owner.Mission = GuideMissionScriptLifetime.Create(config, name, ref owner.Mode, source),
            Get = key => owner.Mission!.Get(key) ?? "",
        };
        GuideMissionTransitions.Begin("first", hooks);
        Assert.True(owner.Mission!.Loaded);
        GuideMissionTransitions.Fork(1, hooks);
        Assert.Equal("second", owner.Mission!.Name);
        Assert.Equal("Tutorial", owner.Mission.MissionType());
        Assert.True(owner.Mode);
        Assert.Equal(1, config.GetInt("global.inMission"));
        GuideMissionTransitions.End(hooks);
        Assert.Equal(2, finished);
        Assert.Null(owner.Mission);
        Assert.Null(config.Registry.FindByName("mission"));
        Assert.False(owner.Mode);
        Assert.Equal(0, config.GetInt("global.inMission"));
    }

}
