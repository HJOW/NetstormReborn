// 논리 요새 이름과 실제 파일 경로가 로그인 지역 읽기/서버 최초 설치에서 분리되는지 실제 파일/Session으로 검사한다.
using Netstorm.Assets;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using MissionScript = Netstorm.Client.Mission.MissionScript;

namespace Netstorm.Client.Tests;

// R4-7c3b 미룬 세션 검사 중 미션 Fort 경로 대조다. cpppj tests/CampaignMissionFortPathTests.inc의
// CampaignMissionFortPath_ConfigMutationDuringReadCannotRenameThatImage를 옮겼다. 같은 장면의 다른 검사와
// 서버 최초 설치 검사는 세션 LoadLocalConnection 제품 작업 뒤에 잇는다. 1.003에서 추가됐다.
public sealed class CampaignMissionFortPathTests
{
    /// <summary>I/O가 다음 fortSpec을 바꿔도 이번 이미지의 이름은 읽기에 사용한 경로다.</summary>
    [Fact]
    public void ConfigMutationDuringReadCannotRenameThatImage()
    {
        string? directory = GameDataLocator.FindDataDirectory();
        Assert.NotNull(directory);
        GameFileSystem assets = GameFileSystem.Open(directory!);
        using var scene = new ReloadSourceScene(assets, "stable-path-486");
        File.WriteAllBytes(Path.Combine(scene.Root, "other", "WorkingReload483.fort"), scene.Changed);
        using MissionScript mission = scene.Mission();
        Assert.True(mission.Loaded && mission.FileSource() is not null);
        MissionFortFileReader reader = mission.FortFileReader(path => scene.Absolute(path));
        Func<string, byte[]?>? source = mission.FileSource();
        MissionFortSourceImage first = reader("WorkingReload483");
        Assert.True(scene.Reads.Count > 0 && first.Image.SequenceEqual(scene.Original));
        Assert.Equal(scene.Absolute("maps/WorkingReload483.fort"), first.FileName);
        MissionFortSourceImage next = reader("WorkingReload483");
        Assert.True(next.Image.SequenceEqual(scene.Changed));
        Assert.Equal(scene.Absolute("other/WorkingReload483.fort"), next.FileName);
        bool mode = false;
        MissionScript? replacement = GuideMissionScriptLifetime.Create(scene.Config, "WorkingReload483", ref mode, source);
        try
        {
            Assert.NotNull(replacement);
            Assert.True(replacement.Loaded && replacement.FileSource() is not null);
        }
        finally
        {
            GuideMissionScriptLifetime.Release(ref replacement, ref mode, scene.Config);
        }

        var defaults = new ConfigInterface(path => assets.TryReadAllBytes(path));
        defaults.Configuration.Assign("missionSpec=\"d/{local.1}.english\"\nfortSpec=\"d/{local.1}.fort\"\n[END]\n");
        using var ordinary = new MissionScript(defaults, "TheWarBegins");
        // 기본 자산 경계는 Guide 교체에서 명시 작업 source로 오인하지 않는다.
        Assert.True(ordinary.Loaded && ordinary.FileSource() is null);
    }
}
