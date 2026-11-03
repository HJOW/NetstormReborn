using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Subscriber 변경(<see cref="RawFortSubscriberChange"/>)의 저장소 부재·복사 순서·물리 범위 계약 3종을 직접 검사한다.
/// 전용 x86 fixture가 없으므로 원본 strcpy·재조회 순서를 계약으로 적는다. (F2c-6, 2026-11-03)
/// </summary>
public sealed class FortSubscriberChangeTests
{
    /// <summary>Template이 없으면 fortMode조차 읽지 않고 My·handle·설정·제목을 건드리지 않는다.</summary>
    [Fact]
    public void TemplateMissingSkipsFortMode()
    {
        uint my = 0x12345678u;
        List<string> calls = [];
        FortSubscriberChangeHooks hooks = new()
        {
            Owner = () =>
            {
                calls.Add("owner");
                return 3u;
            },
            Manager = slot =>
            {
                calls.Add("manager:" + slot);
                return 0u;
            },
            FortMode = () =>
            {
                calls.Add("fortMode");
                return 1u;
            },
            Handle = () =>
            {
                calls.Add("handle");
                return new byte[8].AsSpan();
            },
            Configure = (_, _) => calls.Add("configure"),
            Title = () => calls.Add("title"),
            Subscriber = (_, _, _) => calls.Add("subscriber"),
            Notify = _ => calls.Add("notify"),
        };
        RawFortSubscriberChange.ChangeFortSubscriber(9u, null, ref my, hooks);
        Assert.Equal(0x12345678u, my);
        Assert.Equal(new[] { "owner", "manager:3" }, calls);
    }

    /// <summary>이름을 복사한 뒤 설정·제목·My·writer·알림을 원본 순서대로 호출한다.</summary>
    [Fact]
    public void CopiesNameAndNotifiesInOrder()
    {
        byte[] buffer = new byte[16];
        Array.Fill(buffer, (byte)0xee);
        Encoding.Latin1.GetBytes("Old\0").CopyTo(buffer, 0);
        byte[] name = [(byte)'N', (byte)'e', (byte)'w', 0];
        List<string> calls = [];
        byte[]? stored = null;
        uint my = 0;
        FortSubscriberChangeHooks hooks = new()
        {
            Owner = () =>
            {
                calls.Add("owner");
                return 5u;
            },
            Manager = slot =>
            {
                calls.Add("manager:" + slot);
                Assert.Equal(5u, slot);
                return 11u;
            },
            FortMode = () =>
            {
                calls.Add("fortMode");
                return 1u;
            },
            Handle = () =>
            {
                Span<byte> current = buffer.AsSpan();
                calls.Add("handle:" + (char)current[0]);
                return current;
            },
            Configure = (key, value) =>
            {
                Assert.Equal("fort.mySubHandle", key);
                Assert.Equal("New", value);
                calls.Add("configure:" + key + ':' + value);
            },
            Title = () => calls.Add("title"),
            Subscriber = (owner, id, current) =>
            {
                stored = current.ToArray();
                int nul = Array.IndexOf(stored, (byte)0);
                calls.Add($"subscriber:{owner}:{id}:{Encoding.Latin1.GetString(stored, 0, nul)}");
            },
            Notify = slot => calls.Add("notify:" + slot),
        };
        RawFortSubscriberChange.ChangeFortSubscriber(7u, name, ref my, hooks);
        Assert.Equal(7u, my);
        Assert.Equal("owner;manager:5;fortMode;handle:O;handle:N;configure:fort.mySubHandle:New;title;handle:N;owner;subscriber:5:7:New;owner;notify:5", string.Join(';', calls));
        Assert.Equal(new byte[] { (byte)'N', (byte)'e', (byte)'w', 0 }, buffer[..4]);
        Assert.Equal(0xee, buffer[4]);
        Assert.NotNull(stored);
        Assert.Equal(buffer, stored);
    }

    /// <summary>NUL 없는 이름과 짧은 목적지는 거부하고 앞선 바이트는 유지한다.</summary>
    [Fact]
    public void MissingNulAndShortBufferRejects()
    {
        byte[] roomy = new byte[16];
        byte[] noNul = [(byte)'A', (byte)'B'];
        uint my = 0xaaaa5555u;
        bool configured = false, titled = false, subscribed = false, notified = false;
        FortSubscriberChangeHooks hooks = new()
        {
            Owner = () => 1u,
            Manager = _ => 11u,
            FortMode = () => 1u,
            Handle = () => roomy.AsSpan(),
            Configure = (_, _) => configured = true,
            Title = () => titled = true,
            Subscriber = (_, _, _) => subscribed = true,
            Notify = _ => notified = true,
        };
        // NUL 없는 이름은 두 바이트를 쓴 뒤 물리 범위를 벗어나 거부한다
        Assert.Throws<ArgumentOutOfRangeException>(() => RawFortSubscriberChange.ChangeFortSubscriber(4u, noNul, ref my, hooks));
        Assert.Equal(0xaaaa5555u, my);
        Assert.Equal((byte)'A', roomy[0]);
        Assert.Equal((byte)'B', roomy[1]);
        Assert.Equal(0, roomy[2]);
        Assert.False(configured || titled || subscribed || notified);
        // 짧은 목적지는 두 바이트를 쓴 뒤 세 번째에서 거부하고 앞선 바이트는 유지한다
        byte[] shortRoom = new byte[2];
        byte[] hooked = [(byte)'N', (byte)'e', (byte)'w', 0];
        FortSubscriberChangeHooks small = new()
        {
            Owner = () => 1u,
            Manager = _ => 11u,
            FortMode = () => 1u,
            Handle = () => shortRoom.AsSpan(),
            Configure = (_, _) => configured = true,
            Title = () => titled = true,
            Subscriber = (_, _, _) => subscribed = true,
            Notify = _ => notified = true,
        };
        Assert.Throws<ArgumentOutOfRangeException>(() => RawFortSubscriberChange.ChangeFortSubscriber(6u, hooked, ref my, small));
        Assert.Equal(new byte[] { (byte)'N', (byte)'e' }, shortRoom);
        Assert.False(configured || titled || subscribed || notified);
    }
}
