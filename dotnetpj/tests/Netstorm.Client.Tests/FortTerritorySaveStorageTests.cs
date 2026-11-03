using Netstorm.Core.Original;
using Netstorm.Client.Host;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// Territory 섹션의 현재 쓰기 참조·State 전환·파일 적재·빌림 수명을 실제 저장소에서 대조한다.
/// cpppj <c>FortTerritorySaveTests</c>의 네 번째 TEST_CASE 중 저장소 수명 부분을 옮겼다.
/// (맵·seed 부분은 Core의 <c>FortTerritorySaveTests.MapRecordAndSeedBoundaries</c>에 있다.) (F2c-7, 2026-11-03)
/// </summary>
public sealed class FortTerritorySaveStorageTests
{
    /// <summary>물리 손상이나 누락된 실제 장치가 성공으로 바뀌지 않는지 확인한다.</summary>
    /// <param name="operation">실패해야 하는 동작이다.</param>
    private static bool Rejects(Action operation)
    {
        try
        {
            operation();
        }
        catch (Exception)
        {
            return true;
        }
        return false;
    }

    /// <summary>원본 16바이트 stream 필드의 DWORD를 읽는다.</summary>
    /// <param name="bytes">읽을 필드다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 지정 4바이트만 합친다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 새 마지막 State를 만들 때 빈 Territory의 할당이 0으로 압축되므로 참조 유효성과 쓰기 용량을 구별한다.
    /// </summary>
    [Fact]
    public void CurrentWriteStreamAndTerritoryLifetime()
    {
        FortTemplate fort = EmptyFort();
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "territory.fort");
        Assert.True(Rejects(() => storage.CurrentWriteStream(1)));
        TemplateWriteStream created = storage.CreateSection(1, "Territory");
        TemplateWriteStream selected = storage.CurrentWriteStream(1);
        Assert.Equal(created, selected);
        byte[] state = new byte[28];
        storage.InitializeState(1, state);
        TemplateWriteStream stateStream = storage.CurrentWriteStream(1);
        Assert.NotEqual(created.Section, stateStream.Section);
        byte[] record = [7, 9];
        storage.AccessSection(selected, (fields, _) =>
        {
            Assert.Equal(0u, Read(fields, 4));
            Assert.Equal(0u, Read(fields, 8));
            Assert.Equal(0u, Read(fields, 12));
        });
        Assert.True(Rejects(() => storage.WriteSection(selected, record)));
        Assert.Equal("State", storage.CurrentSection(1));
        storage.WriteSection(stateStream, record);
        storage.CreateSection(1, "Territory");
        Assert.True(Rejects(() => storage.WriteSection(selected, record)));
        storage.Clear(1);
        Assert.True(Rejects(() => storage.WriteSection(stateStream, record)));
        // 보관한 참조도 이후 실제 읽기 부모가 진행한 현재 위치에 쓴다
        FortTemplate loaded = EmptyFort();
        int at = IndexOf("Territory");
        loaded.Sections[at].Resize(120);
        loaded.Sections[at].Reserve(128);
        CampaignTemplateStorage fromFile = new();
        fromFile.Install(1, loaded, "loaded.fort");
        fromFile.SelectTerritory(1).Position = 120;
        TemplateWriteStream loadedStream = fromFile.CurrentWriteStream(1);
        fromFile.WriteSection(loadedStream, record);
        Assert.Equal(122, fromFile.Template(1).Sections[at].Count);
        Assert.Equal(7, fromFile.Template(1).Sections[at].Span[120]);
        Assert.Equal(122u, fromFile.Territory(1).Position);
        fromFile.SelectTerritory(1).Position = 6;
        fromFile.WriteSection(loadedStream, record);
        Assert.Equal(7, fromFile.Template(1).Sections[at].Span[6]);
        Assert.Equal(9, fromFile.Template(1).Sections[at].Span[7]);
        Assert.Equal(8u, fromFile.Territory(1).Position);
    }

    /// <summary>
    /// 조회는 바깥 위치를 보존하며 중첩 쓰기 거부 뒤에도 같은 빌림에서 계속 쓸 수 있다.
    /// </summary>
    [Fact]
    public void AccessSectionKeepsPositionAcrossNestedReject()
    {
        FortTemplate nested = EmptyFort();
        CampaignTemplateStorage borrowing = new();
        borrowing.Install(1, nested, "borrowed.fort");
        borrowing.InitializeState(1, new byte[28]);
        TemplateWriteStream outer = borrowing.CurrentWriteStream(1);
        borrowing.AccessSection(outer, (fields, memory) =>
        {
            RawMemStream.Write(fields, new byte[] { 7, 9 }, 2, memory);
            Assert.Equal(30u, Read(fields, 4));
            TemplateWriteStream same = borrowing.CurrentWriteStream(1);
            Assert.Equal(outer.Section, same.Section);
            Assert.Equal(outer.Generation, same.Generation);
            Assert.Equal(30u, Read(fields, 4));
            Assert.True(Rejects(() => borrowing.AccessSection(same, (_, _) => { })));
            Assert.Equal(30u, Read(fields, 4));
            RawMemStream.Write(fields, new byte[] { 7, 9 }, 2, memory);
        });
        Assert.Equal(32u, borrowing.State(1).Position);
        Assert.Equal(32u, borrowing.State(1).End);
    }
}
