using System.Buffers.Binary;
using System.Text;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 원본 동작과 미션 설정 연결을 검증한다.
namespace Netstorm.Tests;

/// <summary>실제 미션 스크립트 설정과 원본 AI 설정 준비를 연결한다.</summary>
public sealed class MissionAiScriptIntegrationTests
{
    /// <summary>공통·소유자 설정을 실제 mission Config에서 순서대로 읽어 AI 저장소에 누적한다.</summary>
    [Fact]
    public void MissionConfigFeedsCommonAndOwnerAiSettings()
    {
        var config = new ConfigInterface(path => path == "d/demo.english"
            ? Encoding.Latin1.GetBytes("[Header]\naiName=Common\naiAllyList=1;4\naiColor=red\nai2Name=Enemy\nai2AllyList=8\nai2StartMoney=1234\n[END]\n") : null);
        config.Configuration.Assign("missionSpec=\"d/{local.1}.english\"\n[END]\n");
        using var mission = new MissionScript(config, "demo");
        Assert.True(mission.Loaded);
        var settings = new RawMissionAiSettings(OriginalEdition.Patch1078, new MissionAiSettingsState(), new MissionAiSettingsHooks { Get = key => mission.Get(key) ?? "" });
        var raw = new byte[RawMissionAiSettings.Size];
        settings.Prepare(raw, 2);
        Assert.Equal("Enemy", Encoding.Latin1.GetString(raw, 0, Array.IndexOf(raw, (byte)0)));
        Assert.Equal(1234u, BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x22c)));
        Assert.Equal((1u << 1) | (1u << 4) | (1u << 8), BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x23c)));
        Assert.Equal(2u, BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x250)));
    }
}
