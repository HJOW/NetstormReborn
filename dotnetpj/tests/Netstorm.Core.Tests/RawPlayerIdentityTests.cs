// Player 식별자의 초기화·검색 순서·네 getter를 원본 계약 그대로 대조하는 검사 클래스다.
// 근거: cpppj `src/o/RawPlayerIdentity.h`·`RawPlayerIdentity.cpp`(PATCH004841c0/CD00401610 계열). fixture 없이 직접 둔다.
// 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// PlayerFinder의 60바이트 레코드와 판본별 검색을 검증한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
public sealed class RawPlayerIdentityTests
{
    /// <summary>실제 Player·메타·채팅 저장소와 현재 전역 조회를 묶는 장면이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    private sealed class IdentityScene
    {
        /// <summary>실제 9×180바이트 Player 저장소다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        public byte[] Players = new byte[9 * 180];

        /// <summary>실제 40×460바이트 metamap 저장소다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        public byte[] Meta = new byte[40 * 460];

        /// <summary>실제 100×48바이트 채팅 저장소다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        public byte[] Chat = new byte[100 * 48];

        /// <summary>같은 현재 전역의 실제 조회다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        public PlayerIdentityReads Reads = new()
        {
            LocalOwner = () => 0u,
            Subscriber = () => 0u,
            Address = () => 0u,
            Name = () => string.Empty,
            Battle = () => false,
            Editor = () => false,
            Metamap = () => false,
            UnknownName = () => "SOMEONE",
        };

        /// <summary>값을 리틀 엔디언으로 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        /// <param name="bytes">쓸 물리다.</param>
        /// <param name="offset">쓸 바이트 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public static void Put(byte[] bytes, int offset, uint value)
        {
            // 낮은 바이트부터 4바이트를 쓰는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
            for (int index = 0; index < 4; ++index)
            {
                bytes[offset + index] = (byte)(value >> (8 * index));
            }
        }

        /// <summary>이름을 NUL까지만 쓰고 뒤 표식을 지우지 않는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        /// <param name="bytes">쓸 물리다.</param>
        /// <param name="offset">쓸 바이트 위치다.</param>
        /// <param name="text">쓸 이름이다.</param>
        public static void Text(byte[] bytes, int offset, string text)
        {
            // 원본 바이트 순서대로 복사하는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
            for (int index = 0; index < text.Length; ++index)
            {
                bytes[offset + index] = (byte)text[index];
            }

            bytes[offset + text.Length] = 0;
        }

        /// <summary>Player 행의 구독자와 이름만 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        /// <param name="owner">쓸 소유자 번호다.</param>
        /// <param name="subscriber">쓸 구독자 번호다.</param>
        /// <param name="name">쓸 이름이다.</param>
        public void WritePlayerRow(uint owner, uint subscriber, string name)
        {
            Put(Players, (int)owner * 180 + 44, subscriber);
            Text(Players, (int)owner * 180 + 12, name);
        }

        /// <summary>metamap 행의 소유자·구독자·이름·활성 flag만 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        /// <param name="index">쓸 행 번호다.</param>
        /// <param name="owner">쓸 소유자 번호다.</param>
        /// <param name="subscriber">쓸 구독자 번호다.</param>
        /// <param name="name">쓸 이름이다.</param>
        public void WriteMetaRow(int index, uint owner, uint subscriber, string name)
        {
            Put(Meta, index * 460, owner);
            Put(Meta, index * 460 + 4, subscriber);
            Text(Meta, index * 460 + 38, name);
            Meta[index * 460 + 100] = 1;
        }

        /// <summary>채팅 행의 이름·구독자만 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        /// <param name="index">쓸 행 번호다.</param>
        /// <param name="subscriber">쓸 구독자 번호다.</param>
        /// <param name="name">쓸 이름이다.</param>
        public void WriteChatRow(int index, uint subscriber, string name)
        {
            Text(Chat, index * 48, name);
            Put(Chat, index * 48 + 44, subscriber);
        }

        /// <summary>같은 저장소를 빌리는 조회기를 만든다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
        public RawPlayerIdentity Runtime() =>
            new(OriginalEdition.Patch1078, Players, Meta, Chat, Reads);
    }

    /// <summary>초기화는 원본이 쓴 필드만 두고 이름 뒤와 마지막 DWORD 표식을 보존한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void InitializePreservesUnwrittenBytes()
    {
        byte[] record = new byte[60];
        Array.Fill(record, (byte)0xa5);

        RawPlayerIdentity.Initialize(record, 7u);

        Assert.Equal(0u, BitConverter.ToUInt32(record, 0));
        Assert.Equal(0u, BitConverter.ToUInt32(record, 4));
        Assert.Equal(0, record[8]);
        Assert.Equal(0u, BitConverter.ToUInt32(record, 48));
        Assert.Equal(7u, BitConverter.ToUInt32(record, 52));
        Assert.Equal(0xa5, record[9]);
        Assert.Equal(0xa5, record[47]);
        Assert.Equal(0xa5, record[56]);
        Assert.Equal(0xa5, record[59]);
    }

    /// <summary>공유 저장소 크기가 어긋나면 생성 시 거부한다. 빈 metamap은 허용한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ConstructorRejectsBadStorageSizes()
    {
        var scene = new IdentityScene();

        Assert.Throws<ArgumentException>(() => new RawPlayerIdentity(OriginalEdition.Patch1078, new byte[10], scene.Meta, scene.Chat, scene.Reads));
        Assert.Throws<ArgumentException>(() => new RawPlayerIdentity(OriginalEdition.Patch1078, scene.Players, scene.Meta, new byte[10], scene.Reads));
        Assert.Throws<ArgumentException>(() => new RawPlayerIdentity(OriginalEdition.Patch1078, scene.Players, new byte[10], scene.Chat, scene.Reads));

        var emptyMeta = new RawPlayerIdentity(OriginalEdition.Patch1078, scene.Players, [], scene.Chat, scene.Reads);
        Assert.Empty(emptyMeta.MetaRecords());
    }

    /// <summary>로컬 소유자 일치는 현재 이름·주소·구독자·소유자로 덮어쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ResolveLocalOwnerCopiesCurrent()
    {
        var scene = new IdentityScene();
        scene.Reads.LocalOwner = () => 2u;
        scene.Reads.Subscriber = () => 0x80000002u;
        scene.Reads.Address = () => 0x11u;
        scene.Reads.Name = () => "LocalUser";
        byte[] record = new byte[60];
        Array.Fill(record, (byte)0xa5);
        IdentityScene.Put(record, 0, 2u);
        IdentityScene.Put(record, 52, 0xdeadbeefu);

        Assert.True(scene.Runtime().Resolve(record));
        Assert.Equal(2u, BitConverter.ToUInt32(record, 0));
        Assert.Equal(0x80000002u, BitConverter.ToUInt32(record, 4));
        Assert.Equal("LocalUser", Encoding.Latin1.GetString(record, 8, 9));
        Assert.Equal(0u, record[17]);
        Assert.Equal(0x11u, BitConverter.ToUInt32(record, 48));
        Assert.Equal(0xa5, record[56]);
    }

    /// <summary>전투 분기는 Player 1..8의 첫 소유자 일치를 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ResolveBattleOwnerFillsFromPlayerRow()
    {
        var scene = new IdentityScene();
        scene.Reads.Battle = () => true;
        scene.WritePlayerRow(2u, 0x80000002u, "Alpha");
        byte[] record = new byte[60];
        RawPlayerIdentity.Initialize(record, 2u);
        IdentityScene.Put(record, 0, 2u);

        Assert.True(scene.Runtime().Resolve(record));
        Assert.Equal(2u, BitConverter.ToUInt32(record, 0));
        Assert.Equal(0x80000002u, BitConverter.ToUInt32(record, 4));
        Assert.Equal("Alpha", Encoding.Latin1.GetString(record, 8, 5));
        Assert.Equal(0u, BitConverter.ToUInt32(record, 48));
    }

    /// <summary>이름 비교는 ASCII 대소문자를 접는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ResolveBattleNameMatchesCaseInsensitive()
    {
        var scene = new IdentityScene();
        scene.Reads.Battle = () => true;
        scene.WritePlayerRow(3u, 0x80000003u, "ALPHA");
        byte[] record = new byte[60];
        RawPlayerIdentity.Initialize(record, 2u);
        IdentityScene.Text(record, 8, "alpha");

        Assert.True(scene.Runtime().Resolve(record));
        Assert.Equal(3u, BitConverter.ToUInt32(record, 0));
        Assert.Equal(0x80000003u, BitConverter.ToUInt32(record, 4));
        Assert.Equal("ALPHA", Encoding.Latin1.GetString(record, 8, 5));
    }

    /// <summary>metamap 분기는 활성 기록의 구독자 일치를 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ResolveMetamapActiveSubscriber()
    {
        var scene = new IdentityScene();
        scene.Reads.Metamap = () => true;
        scene.WriteMetaRow(0, 6u, 0x80000002u, "MetaFirst");
        byte[] record = new byte[60];
        RawPlayerIdentity.Initialize(record, 2u);
        IdentityScene.Put(record, 4, 0x80000002u);

        Assert.True(scene.Runtime().Resolve(record));
        Assert.Equal(6u, BitConverter.ToUInt32(record, 0));
        Assert.Equal(0x80000002u, BitConverter.ToUInt32(record, 4));
        Assert.Equal("MetaFirst", Encoding.Latin1.GetString(record, 8, 9));
    }

    /// <summary>metamap 진입에 기록이 없으면 명시 오류로 멈춘다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ResolveMetamapMissingStorageThrows()
    {
        var scene = new IdentityScene();
        scene.Reads.Metamap = () => true;
        var identity = new RawPlayerIdentity(OriginalEdition.Patch1078, scene.Players, [], scene.Chat, scene.Reads);
        byte[] record = new byte[60];
        RawPlayerIdentity.Initialize(record, 2u);
        IdentityScene.Put(record, 4, 0x80000002u);

        Assert.Throws<InvalidOperationException>(() => identity.Resolve(record));
    }

    /// <summary>어느 표에도 없으면 채팅 첫 일치가 소유자 0으로 남는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ResolveChatSubscriberFallback()
    {
        var scene = new IdentityScene();
        scene.WriteChatRow(2, 0x80000002u, "ChatSecond");
        byte[] record = new byte[60];
        RawPlayerIdentity.Initialize(record, 2u);
        IdentityScene.Put(record, 4, 0x80000002u);

        Assert.True(scene.Runtime().Resolve(record));
        Assert.Equal(0u, BitConverter.ToUInt32(record, 0));
        Assert.Equal(0x80000002u, BitConverter.ToUInt32(record, 4));
        Assert.Equal("ChatSecond", Encoding.Latin1.GetString(record, 8, 10));
    }

    /// <summary>네 getter는 같은 Resolve 몸체를 쓰고 실패 이름은 대체 이름을 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void IdentityGettersReturnResolvedValues()
    {
        var scene = new IdentityScene();
        scene.Reads.Battle = () => true;
        scene.WritePlayerRow(2u, 0x80000002u, "Alpha");
        var identity = scene.Runtime();

        Assert.Equal(0x80000002u, identity.SubscriberForOwner(2u));
        Assert.Equal(2u, identity.OwnerForSubscriber(0x80000002u));
        Assert.Equal(0u, identity.AddressForSubscriber(0x80000002u));
        Assert.Equal("Alpha", identity.NameForSubscriber(0x80000002u));
        Assert.Equal("Alpha", identity.NameForOwner(2u));
        Assert.Equal("SOMEONE", identity.NameForSubscriber(0x1234u));
        Assert.Equal(OriginalEdition.Patch1078, identity.Edition());
        Assert.Same(scene.Players, identity.PlayerRecords());
    }

    /// <summary>같은 조회기는 현재 저장소 변경을 그대로 읽는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void IdentityReadsLiveStorage()
    {
        var scene = new IdentityScene();
        scene.Reads.Battle = () => true;
        scene.WritePlayerRow(2u, 0x80000002u, "Alpha");
        var identity = scene.Runtime();

        Assert.Equal(0x80000002u, identity.SubscriberForOwner(2u));
        IdentityScene.Put(scene.Players, 2 * 180 + 44, 0xabcd9911u);
        Assert.Equal(0xabcd9911u, identity.SubscriberForOwner(2u));
    }
}
