using System.Buffers.Binary;
using System.Text;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// 실제 보호 Fort(TEST01)의 기존 manager·Subscriber를 쓰고 helper·ID 쓰기 뒤 header가 바뀌어도 선택 stream을 유지한다.
/// cpppj <c>FortSubscriberWriteTests</c>의 두 번째 TEST_CASE를 옮겼다. (F2c-6, 2026-11-03)
/// Core의 90행 fixture 재생(<c>Netstorm.Core.Tests.FortSubscriberWriteTests</c>)과 달리
/// 이 검사는 Client 저장소(<see cref="CampaignTemplateStorage"/>)에 연결하므로 Client 프로젝트에 둔다.
/// </summary>
public sealed class FortSubscriberWriteStorageTests
{
    /// <summary>실제 보호 Fort의 기존 manager·Subscriber로 저장하고 캡처한 물리 이름을 유지한다.</summary>
    [Fact]
    public void RealStorageKeepsChosenManagerAndCapturedPhysicalName()
    {
        byte[] original = RealFortBytes("TEST01");
        FortTemplate parsed = RealFort("TEST01");
        CampaignTemplateStorage storage = new();
        storage.InstallFile(0, parsed, original, "first.fort");
        storage.InstallFile(2, RealFort("TEST01"), original, "second.fort");
        storage.Borrow(1, 0);
        string first = storage.ReadSubscriberName(0);
        uint subscriber = 0xffffffffu;
        uint writes = 0;
        TemplateWriteStream selected = default;
        byte[] name = new byte[64];
        Encoding.Latin1.GetBytes("Original\0").CopyTo(name, 0);
        FortSubscriberWriteHooks hooks = new()
        {
            Manager = slot => Header(storage, (int)slot * 20),
            Assertion = _ => throw new InvalidOperationException("실제 Subscriber 저장 예상 밖 진단"),
            RegisterSubscriber = () =>
            {
                FortSubscriberLoadHooks registration = new()
                {
                    ReadRegistered = (string key, ref uint target) =>
                    {
                        target = 7;
                        return true;
                    },
                };
                RawFortSubscriberLoad.FinishFortSubscriberRegistration(
                    new FortSubscriberLoadGlobals { GetSubscriber = () => subscriber, SetSubscriber = value => subscriber = value },
                    registration);
                storage.Borrow(1, 2);
                Encoding.Latin1.GetBytes("Helper\0").CopyTo(name, 0);
            },
            Create = (manager, section) =>
            {
                selected = storage.CreateManagerSection(manager, section);
                return 1u;
            },
            Write = (stream, bytes) =>
            {
                Assert.Equal(1u, stream);
                storage.WriteSection(selected, bytes);
                if (++writes == 1)
                {
                    storage.Borrow(1, 0);
                    Encoding.Latin1.GetBytes("AfterID\0").CopyTo(name, 0);
                }
            },
        };
        RawFortSubscriberWrite.SaveFortSubscriber(1, 0xffffffffu, name, hooks);
        Assert.Equal(7u, subscriber);
        Assert.Equal(3u, writes);
        Assert.Equal("AfterID", storage.ReadSubscriberName(2));
        Assert.Equal(first, storage.ReadSubscriberName(0));
        Assert.Equal(0xffffffffu, BinaryPrimitives.ReadUInt32LittleEndian(storage.Template(2).Section("Subscriber")));
        Assert.Equal(original, RealFortBytes("TEST01"));
    }
}
