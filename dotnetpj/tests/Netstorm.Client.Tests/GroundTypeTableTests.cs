// isle.type 파일 바이트에서 DN 스택으로 읽은 테이블이 L2 증거와 같은 J-clock을 가지는지 확인한다.
using System.Linq;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 지면 타입 테이블이 실제 자산 바이트와 같은지 고정하는 클래스다. 2026-11-18 0.616에서 R4-2 지면 프레임 원인 좁히기용으로 추가했다.
// 근거: `extracted/dotnet-l2-0614` 3경로 증거와 `assets/game-data/netstorm.tarc` 내 `isle.type` 317클러스터·첫 J-clock 172(JJ00)의 독립 파싱 결과다.
public sealed class GroundTypeTableTests
{
    /// <summary>isle.type(지면 타입 97=70+27)의 DN 테이블이 파일 바이트와 같은지 확인한다.</summary>
    [Fact]
    public void IsleTableMatchesFileBytes()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        IReadOnlyList<string> order = RiftTypeLoadOrder.Get(OriginalEdition.Patch1078);
        int isle = order.Select((name, index) => (name, index)).First(pair => pair.name == "isle").index;
        Assert.Equal(27, isle);
        RiftTypeFrames table = definitions[isle].FrameTable();
        Assert.Equal(317, table.Codes.Count);
        // 파일 순서대로 첫 J-clock 코드를 찾는다. tarc 독립 파싱 결과 JJ00이 172번이다.
        int firstJ = -1;
        // 모든 코드를 순서대로 훑어 첫 J-clock을 찾는다.
        for (int i = 0; i < table.Codes.Count; i++)
        {
            if (table.Codes[i].Side == (byte)'J')
            {
                firstJ = i;
                break;
            }
        }
        Assert.Equal(172, firstJ);
        Assert.Equal(172, table.Run((byte)'J').First);
    }
}
