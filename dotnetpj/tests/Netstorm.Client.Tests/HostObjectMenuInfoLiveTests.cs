using System.Reflection;
using Netstorm.Assets;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 미션 TYPE과 현재 동맹을 객체 정보 본체에 연결한다.
namespace Netstorm.Client.Tests;

/// <summary>부분 적재 관문과 검사 장면의 현재 세션 조회를 구분한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>검사 전용 적재 표식에서 실제 정의와 동맹 변경을 다시 읽는다.</summary>
    [Fact]
    public void ObjectInformationUsesCurrentSessionAllianceAndRealClassTable()
    {
        var (session, _) = Scene();
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        uint type = checked((uint)(70 + Enumerable.Range(0, definitions.Count).First(index => string.IsNullOrEmpty(definitions[index].String("class")))));
        var headers = new List<string>();
        var hooks = new ObjectMenuInfoHooks
        {
            Description = number => "TYPE" + number, OwnerName = owner => "Player" + owner,
            Translate = text => text, Substitute = text => text,
            Print = (pattern, values) => string.Join('|', values.Select(value => value.IsText ? value.Text : value.Number.ToString())),
            Title = _ => { }, Header = headers.Add,
        };
        Assert.Throws<InvalidOperationException>(() => session.MakeObjectMenuInfo(definitions, hooks));
        // 완전한 제품 적재를 대신하지 않고 이 검사 장면에서만 해당 관문을 연다.
        FieldInfo loaded = typeof(HostSession).GetField("_nativeLoaded", BindingFlags.NonPublic | BindingFlags.Instance)!;
        loaded.SetValue(session, true);
        try
        {
            RawObjectMenuInfo info = session.MakeObjectMenuInfo(definitions, hooks);
            session.Alliances[2 * 9 + 1] = 1;
            info.Describe(type, 0, 2, uint.MaxValue);
            Assert.Contains("Ally|Player2", headers);
            headers.Clear(); session.Alliances[2 * 9 + 1] = 0;
            info.Describe(type, 0, 2, uint.MaxValue);
            Assert.Contains("Enemy|Player2", headers);
            Assert.Throws<InvalidOperationException>(() => info.Summary(1));
        }
        finally { loaded.SetValue(session, false); session.ReleasePlayerProcesses(); }
    }
}
