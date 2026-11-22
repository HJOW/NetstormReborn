using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 같은 실제 풀/현재 Deck의 동기화 순서를 원본과 대조한다.

/// <summary>원본98행과 current fullDeck/SID0·중단/현재 개수의 경계를 검증한다.</summary>
public sealed class ProductionSyncTests
{
    /// <summary>kind:flags 또는kind:power 명시 입력만 해석한다.</summary>
    private static int[][] Pairs(string text) => text == "-" ? [] : text.Split(';').Select(value => value.Split(':').Select(field => int.Parse(field, CultureInfo.InvariantCulture)).ToArray()).ToArray();
    /// <summary>실제 순회와 외부 효과의 원문 입력 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은24000슬롯 내용물 물리다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>실제188TYPE의 명시 flags2다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 같은 실제 Deck 바이트다.</summary>
        public readonly byte[] Deck = new byte[ProductionDeckLayout.Bytes];
        /// <summary>현재 실제 fullDeck DWORD다.</summary>
        public uint Full;
        /// <summary>명시 현재 부모 전환과 조회 횟수다.</summary>
        public uint Change, Getters, Query;
        /// <summary>명시 실제 신전 응답이다.</summary>
        public bool Temple;
        /// <summary>명시 검색 반환값이다.</summary>
        public uint[] Queries = [];
        /// <summary>원본의 모든 효과 순서다.</summary>
        public string Trace = "";
        /// <summary>실제 선택 장치다.</summary>
        public readonly ProductionSyncHooks Hooks;
        /// <summary>같은 실제 처리기다.</summary>
        public readonly RawProductionSync Sync;
        /// <summary>입력 바이트/외부 응답만 준비하고 분기는 실제 제품에 맡긴다.</summary>
        public Scene(string[] row)
        {
            var forms = Pairs(row[0]); var products = Pairs(row[1]); uint profile = U(row[2]); Temple = U(row[3]) != 0; Full = U(row[4]); Change = U(row[5]); Queries = Inputs(row[6]); BinaryPrimitives.WriteUInt16LittleEndian(Raw(Pool, 50)[6..], (ushort)(forms.Length == 0 ? 0 : 100));
            // 중복도 각각 별도 실제 raw Form/next로 작성한다.
            for (int i = 0; i < forms.Length; i++) { var raw = Raw(Pool, 100 + i); raw[10] = 6; BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], (ushort)(i + 1 < forms.Length ? 101 + i : 0)); BinaryPrimitives.WriteUInt16LittleEndian(raw[18..], unchecked((ushort)forms[i][0])); BinaryPrimitives.WriteUInt16LittleEndian(raw[20..], unchecked((ushort)forms[i][1])); }
            // 명시 TYPEbit4와5바이트 Deck의 signed power만 공급한다.
            for (int kind = 0; kind < Types.Length; kind++) Types[kind].Flags2 = profile == 1 || (profile == 0 && kind == 133) ? 4u : 0u;
            BinaryPrimitives.WriteUInt32LittleEndian(Deck.AsSpan(0x14c), (uint)products.Length);
            // 실제 물리 항목 순서를 보존한다.
            for (int i = 0; i < products.Length; i++) { Deck[i * 5] = unchecked((byte)products[i][0]); Deck[i * 5 + 2] = unchecked((byte)products[i][1]); }
            Hooks = new()
            {
                Deck = () => { Event("K"); return Deck; }, Technology = () => { uint parent = 50 + (Getters != 0 ? Change : 0); Getters++; Event("T," + parent); return parent; }, Temple = owner => { Assert.Equal(1u, owner); return Temple; }, CancelCursor = () => Event("X"), Destroy = (sid, flags) => Event("D," + sid + ',' + flags),
                FindKind = (parent, mask, kind, index) => { uint value = Queries[Math.Min((int)Query, Queries.Length - 1)]; Query++; Event("Q," + parent + ',' + mask + ',' + kind + ',' + index + ',' + value); return value; },
                Create = (parent, kind, amount, flags, attach, quality) => { Event("C," + parent + ',' + kind + ',' + amount + ',' + flags + ',' + attach + ',' + quality); return 20000; }, Assertion = text => Event("A," + Hex(text)),
            };
            Sync = new(Pool, Types, new(), new() { LocalOwner = () => 1, FullDeck = () => Full }, Hooks);
        }
        /// <summary>실제 이벤트 사이의 구분자만 누적한다.</summary>
        private void Event(string text) { if (Trace.Length != 0) Trace += ';'; Trace += text; }
    }
    /// <summary>BYTE 중복 카운터255/256/257도 포함해 원본98행과 풀 전체 불변을 대조한다.</summary>
    [Fact]
    public void Native98CurrentDeckParentRemovalCreationOrderAndPoolUnchanged()
    {
        int count = 0;
        // 입력과 관측을 구별해 모든 원문 행을 읽는다.
        foreach (var row in Rows("production-sync")) { Assert.Equal(8, row.Length); var scene = new Scene(row); byte[] before = All(scene.Pool).ToArray(); scene.Sync.Synchronize(); Assert.Equal(row[7], scene.Trace); Assert.True(before.AsSpan().SequenceEqual(All(scene.Pool))); count++; }
        Assert.Equal(98, count);
    }
    /// <summary>Deck getter 뒤 비영 fullDeck과 실제 SID0·빈 Deck도 같은 본체로 조회한다.</summary>
    [Fact]
    public void SharedWordAfterGetterAndZeroParentKeepRealTraversal()
    {
        var scene = new Scene(["71:8", "-", "2", "0", "0", "0", "0", ""]); uint destroyed = 0, current = 0x80000000; var old = scene.Hooks.Deck; scene.Hooks.Deck = () => { scene.Full = current; return old!(); }; scene.Hooks.Destroy = (sid, flags) => { Assert.Equal(100u, sid); Assert.Equal(0u, flags); destroyed++; };
        // 부모 목록은 유지하므로 현재 전역 변화만 관측한다.
        foreach (uint value in new[] { 0x80000000u, uint.MaxValue, 0u }) { current = value; scene.Sync.Synchronize(); Assert.Equal(value, scene.Full); Assert.Equal(value != 0 ? 0u : 1u, destroyed); }
        var zero = new Scene(["-", "-", "0", "0", "0", "0", "0", ""]); zero.Hooks.Technology = () => { zero.Getters++; return 0; }; zero.Hooks.FindKind = (_, _, _, _) => throw new InvalidOperationException("빈 Deck 조회 금지"); zero.Hooks.Create = (_, _, _, _, _, _) => throw new InvalidOperationException("빈 Deck 생성 금지"); byte[] before = All(zero.Pool).ToArray(); zero.Sync.Synchronize(); Assert.Equal(2u, zero.Getters); Assert.True(before.AsSpan().SequenceEqual(All(zero.Pool)));
    }
}
